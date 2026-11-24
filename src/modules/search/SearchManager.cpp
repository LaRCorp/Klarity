#include "SearchManager.h"
#include "core/LogModel.h"

#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeType>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <KLocalizedString>

SearchManager::SearchManager(QObject *parent)
    : QObject(parent)
{
    m_hasFd = !QStandardPaths::findExecutable(QStringLiteral("fd")).isEmpty();
    m_hasRg = !QStandardPaths::findExecutable(QStringLiteral("rg")).isEmpty();

    m_batchTimer = new QTimer(this);
    m_batchTimer->setInterval(40);
    connect(m_batchTimer, &QTimer::timeout, this, &SearchManager::flushPendingResults);

    m_historyTimer = new QTimer(this);
    m_historyTimer->setSingleShot(true);
    m_historyTimer->setInterval(5000); // 5-second debounce for search history
    connect(m_historyTimer, &QTimer::timeout, this, [this]() {
        if (!m_pendingHistoryQuery.isEmpty()) {
            saveHistory(m_pendingHistoryQuery);
            m_pendingHistoryQuery.clear();
        }
    });

    loadHistory();
}

SearchManager::~SearchManager()
{
    cancelSearch();
}

void SearchManager::search(const QString &query,
                           const QString &scope,
                           const QString &typeFilter,
                           int maxDepth,
                           bool searchContent,
                           bool includeHidden)
{
    const QString cleanQuery = query.trimmed();
    if (cleanQuery.isEmpty()) {
        cancelSearch();
        m_results.clear();
        m_pendingResults.clear();
        emit resultsChanged();
        emit resultCountChanged();
        m_statusMessage = i18n("Enter a search term or file pattern...");
        emit statusMessageChanged();
        return;
    }

    cancelSearch();

    m_results.clear();
    m_pendingResults.clear();
    m_outputBuffer.clear();
    emit resultsChanged();
    emit resultCountChanged();

    if (cleanQuery.trimmed().length() >= 2) {
        m_pendingHistoryQuery = cleanQuery.trimmed();
        m_historyTimer->start(5000); // Save only after 5 seconds of stable text
    }

    parseAndExecuteQuery(cleanQuery, scope, typeFilter, maxDepth, searchContent, includeHidden);
}

void SearchManager::cancelSearch()
{
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
        delete m_process;
        m_process = nullptr;
    }

    if (m_batchTimer && m_batchTimer->isActive()) {
        m_batchTimer->stop();
    }

    flushPendingResults();

    if (m_isSearching) {
        m_isSearching = false;
        emit isSearchingChanged();
        m_statusMessage = i18n("Search stopped. Found %1 item(s).", m_results.size());
        emit statusMessageChanged();
    }
}

void SearchManager::parseAndExecuteQuery(const QString &query,
                                        const QString &scope,
                                        const QString &typeFilter,
                                        int maxDepth,
                                        bool searchContent,
                                        bool includeHidden)
{
    // Determine target search directories
    QStringList targetPaths;
    if (scope == QStringLiteral("all")) {
        targetPaths << QStringLiteral("/");
    } else if (scope == QStringLiteral("home")) {
        targetPaths << QDir::homePath();
    } else if (scope == QStringLiteral("config")) {
        targetPaths << QStringLiteral("/etc") << (QDir::homePath() + QStringLiteral("/.config"));
    } else if (scope == QStringLiteral("apps")) {
        targetPaths << QStringLiteral("/usr/share/applications")
                    << (QDir::homePath() + QStringLiteral("/.local/share/applications"));
    } else if (scope == QStringLiteral("logs")) {
        targetPaths << QStringLiteral("/var/log");
    } else {
        targetPaths << (scope.isEmpty() ? QDir::homePath() : scope);
    }

    // Parse query syntax
    QString parsedExt;
    QString parsedSize;
    QString parsedType = typeFilter;
    QString textPattern = query;

    // 1. Detect extension pattern: *.ext or .ext
    static const QRegularExpression extRe(QStringLiteral(R"((\*\.|\.)([a-zA-Z0-9_\-]+))"));
    const auto extMatch = extRe.match(textPattern);
    if (extMatch.hasMatch() && textPattern.startsWith(extMatch.captured(0))) {
        parsedExt = extMatch.captured(2);
        textPattern = textPattern.mid(extMatch.capturedLength(0)).trimmed();
    }

    // 2. Detect size filter: > 10M, >10M, < 1M, <1M, size:>10M
    static const QRegularExpression sizeRe(QStringLiteral(R"((?:size:\s*)?([><])\s*(\d+[kKmMgGtT]?b?))"), QRegularExpression::CaseInsensitiveOption);
    const auto sizeMatch = sizeRe.match(textPattern);
    if (sizeMatch.hasMatch()) {
        const QString op = sizeMatch.captured(1);
        const QString sz = sizeMatch.captured(2);
        parsedSize = (op == QStringLiteral(">") ? QStringLiteral("+") : QStringLiteral("-")) + sz;
        textPattern.remove(sizeMatch.capturedStart(0), sizeMatch.capturedLength(0));
        textPattern = textPattern.trimmed();
    }

    // 3. Detect type filter: type:file, type:dir, type:link, type:exec
    static const QRegularExpression typeRe(QStringLiteral(R"(type:(file|dir|link|symlink|exec))"), QRegularExpression::CaseInsensitiveOption);
    const auto typeMatch = typeRe.match(textPattern);
    if (typeMatch.hasMatch()) {
        const QString t = typeMatch.captured(1).toLower();
        if (t == QStringLiteral("file")) parsedType = QStringLiteral("file");
        else if (t == QStringLiteral("dir")) parsedType = QStringLiteral("dir");
        else if (t == QStringLiteral("link") || t == QStringLiteral("symlink")) parsedType = QStringLiteral("symlink");
        else if (t == QStringLiteral("exec")) parsedType = QStringLiteral("exec");
        textPattern.remove(typeMatch.capturedStart(0), typeMatch.capturedLength(0));
        textPattern = textPattern.trimmed();
    }

    textPattern = textPattern.trimmed();

    QString program;
    QStringList args;

    if (searchContent && m_hasRg) {
        // Ripgrep search mode (search inside file contents)
        program = QStringLiteral("rg");
        args << QStringLiteral("--files-with-matches")
             << QStringLiteral("--smart-case");

        if (maxDepth > 0) {
            args << QStringLiteral("--max-depth") << QString::number(maxDepth);
        }
        if (includeHidden) {
            args << QStringLiteral("--hidden");
        }

        // Exclusions for virtual filesystems
        args << QStringLiteral("-g") << QStringLiteral("!/proc/**")
             << QStringLiteral("-g") << QStringLiteral("!/sys/**")
             << QStringLiteral("-g") << QStringLiteral("!/dev/**")
             << QStringLiteral("-g") << QStringLiteral("!/run/**");

        if (!parsedExt.isEmpty()) {
            args << QStringLiteral("-g") << (QStringLiteral("*.") + parsedExt);
        }

        args << (textPattern.isEmpty() ? query : textPattern);
        args.append(targetPaths);
    } else if (m_hasFd) {
        // fd search mode
        program = QStringLiteral("fd");
        args << QStringLiteral("--color=never");

        if (includeHidden) {
            args << QStringLiteral("--hidden");
        }

        // Virtual filesystem exclusions
        args << QStringLiteral("--exclude") << QStringLiteral("/proc")
             << QStringLiteral("--exclude") << QStringLiteral("/sys")
             << QStringLiteral("--exclude") << QStringLiteral("/dev")
             << QStringLiteral("--exclude") << QStringLiteral("/run")
             << QStringLiteral("--exclude") << QStringLiteral("/mnt")
             << QStringLiteral("--exclude") << QStringLiteral("/media");

        if (maxDepth > 0) {
            args << QStringLiteral("--max-depth") << QString::number(maxDepth);
        }

        if (!parsedExt.isEmpty()) {
            args << QStringLiteral("-e") << parsedExt;
        }

        if (!parsedSize.isEmpty()) {
            args << QStringLiteral("-S") << parsedSize;
        }

        if (parsedType == QStringLiteral("file")) {
            args << QStringLiteral("-t") << QStringLiteral("f");
        } else if (parsedType == QStringLiteral("dir")) {
            args << QStringLiteral("-t") << QStringLiteral("d");
        } else if (parsedType == QStringLiteral("symlink")) {
            args << QStringLiteral("-t") << QStringLiteral("l");
        } else if (parsedType == QStringLiteral("exec")) {
            args << QStringLiteral("-t") << QStringLiteral("x");
        }

        if (!textPattern.isEmpty()) {
            args << textPattern;
        }

        args.append(targetPaths);
    } else {
        // find fallback
        program = QStringLiteral("find");
        args.append(targetPaths);

        if (maxDepth > 0) {
            args << QStringLiteral("-maxdepth") << QString::number(maxDepth);
        }

        if (parsedType == QStringLiteral("file")) {
            args << QStringLiteral("-type") << QStringLiteral("f");
        } else if (parsedType == QStringLiteral("dir")) {
            args << QStringLiteral("-type") << QStringLiteral("d");
        } else if (parsedType == QStringLiteral("symlink")) {
            args << QStringLiteral("-type") << QStringLiteral("l");
        }

        if (!parsedExt.isEmpty()) {
            args << QStringLiteral("-name") << (QStringLiteral("*.") + parsedExt);
        } else if (!textPattern.isEmpty()) {
            args << QStringLiteral("-iname") << (QStringLiteral("*") + textPattern + QStringLiteral("*"));
        }
    }

    m_isSearching = true;
    emit isSearchingChanged();
    m_statusMessage = i18n("Searching...");
    emit statusMessageChanged();

    m_process = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &SearchManager::onReadyReadStandardOutput);
    connect(m_process, &QProcess::finished, this, &SearchManager::onProcessFinished);

    LogModel::instance()->append(
        QStringLiteral("SearchManager"),
        QStringLiteral("info"),
        QStringLiteral("Executing: %1 %2").arg(program, args.join(QLatin1Char(' ')))
    );

    m_batchTimer->start();
    m_process->start(program, args);
}

void SearchManager::onReadyReadStandardOutput()
{
    if (!m_process) return;

    m_outputBuffer.append(m_process->readAllStandardOutput());

    while (true) {
        const int newlineIdx = m_outputBuffer.indexOf('\n');
        if (newlineIdx == -1) break;

        const QByteArray lineBytes = m_outputBuffer.left(newlineIdx);
        m_outputBuffer.remove(0, newlineIdx + 1);

        const QString line = QString::fromUtf8(lineBytes).trimmed();
        if (!line.isEmpty()) {
            addResultPath(line);
        }

        if (m_results.size() + m_pendingResults.size() >= m_maxResultsLimit) {
            cancelSearch();
            m_statusMessage = i18n("Search reached maximum limit (%1 results).", m_maxResultsLimit);
            emit statusMessageChanged();
            break;
        }
    }
}

void SearchManager::addResultPath(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists() && !fi.isSymLink()) return;

    const bool isDir = fi.isDir();
    const bool isSym = fi.isSymLink();
    const bool isExec = fi.isExecutable() && !isDir;

    QString iconName;
    if (isDir) {
        iconName = QStringLiteral("folder");
    } else if (isSym) {
        iconName = QStringLiteral("emblem-symbolic-link");
    } else {
        const QMimeType mime = m_mimeDb.mimeTypeForFile(fi);
        iconName = mime.iconName();
        if (iconName.isEmpty() || iconName == QStringLiteral("unknown")) {
            iconName = isExec ? QStringLiteral("application-x-executable") : QStringLiteral("text-plain");
        }
    }

    const QString typeName = isDir ? i18n("Directory") : (isSym ? i18n("Symlink") : (isExec ? i18n("Executable") : i18n("File")));

    m_pendingResults.append(QVariantMap{
        {QStringLiteral("name"), fi.fileName().isEmpty() ? path : fi.fileName()},
        {QStringLiteral("path"), fi.absoluteFilePath()},
        {QStringLiteral("dir"), fi.absolutePath()},
        {QStringLiteral("size"), isDir ? QStringLiteral("-") : formatSize(fi.size())},
        {QStringLiteral("rawSize"), (qint64)(isDir ? 0 : fi.size())},
        {QStringLiteral("modified"), fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"))},
        {QStringLiteral("icon"), iconName},
        {QStringLiteral("isDir"), isDir},
        {QStringLiteral("isSymLink"), isSym},
        {QStringLiteral("isExecutable"), isExec},
        {QStringLiteral("typeName"), typeName},
        {QStringLiteral("isHidden"), fi.isHidden()}
    });
}

void SearchManager::flushPendingResults()
{
    if (m_pendingResults.isEmpty()) return;

    m_results.append(m_pendingResults);
    m_pendingResults.clear();

    emit resultsChanged();
    emit resultCountChanged();

    if (m_isSearching) {
        m_statusMessage = i18n("Searching... Found %1 item(s)", m_results.size());
        emit statusMessageChanged();
    }
}

void SearchManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    if (!m_outputBuffer.isEmpty()) {
        const QString remaining = QString::fromUtf8(m_outputBuffer).trimmed();
        if (!remaining.isEmpty()) {
            const QStringList lines = remaining.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (const QString &l : lines) {
                addResultPath(l.trimmed());
            }
        }
        m_outputBuffer.clear();
    }

    flushPendingResults();

    if (m_batchTimer && m_batchTimer->isActive()) {
        m_batchTimer->stop();
    }

    m_isSearching = false;
    emit isSearchingChanged();

    m_statusMessage = i18n("Search completed. Found %1 item(s).", m_results.size());
    emit statusMessageChanged();

    LogModel::instance()->append(
        QStringLiteral("SearchManager"),
        QStringLiteral("info"),
        m_statusMessage
    );
}

void SearchManager::openFile(const QString &path)
{
    if (path.isEmpty()) return;
    const QUrl url = QUrl::fromLocalFile(path);
    QDesktopServices::openUrl(url);

    LogModel::instance()->append(
        QStringLiteral("SearchManager"),
        QStringLiteral("info"),
        QStringLiteral("Opened file: %1").arg(path)
    );
}

void SearchManager::openContainingFolder(const QString &path)
{
    if (path.isEmpty()) return;

    // Prefer dolphin --select <path> if available
    if (!QStandardPaths::findExecutable(QStringLiteral("dolphin")).isEmpty()) {
        QProcess::startDetached(QStringLiteral("dolphin"), {QStringLiteral("--select"), path});
    } else {
        const QFileInfo fi(path);
        const QString dirPath = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
    }

    LogModel::instance()->append(
        QStringLiteral("SearchManager"),
        QStringLiteral("info"),
        QStringLiteral("Opened folder for: %1").arg(path)
    );
}

void SearchManager::copyToClipboard(const QString &text)
{
    if (auto *cb = QGuiApplication::clipboard()) {
        cb->setText(text);
        emit operationFinished(true, i18n("Path copied to clipboard: %1", text));
    }
}

QStringList SearchManager::getSuggestions(const QString &partialQuery) const
{
    const QString clean = partialQuery.trimmed().toLower();

    // 1. Extension suggestions
    if (clean.startsWith(QLatin1String("*.")) || clean == QLatin1String("*")) {
        const QStringList exts{
            QStringLiteral("*.service"),
            QStringLiteral("*.conf"),
            QStringLiteral("*.desktop"),
            QStringLiteral("*.log"),
            QStringLiteral("*.sh"),
            QStringLiteral("*.json"),
            QStringLiteral("*.yaml"),
            QStringLiteral("*.qml"),
            QStringLiteral("*.cpp"),
            QStringLiteral("*.h"),
            QStringLiteral("*.py"),
            QStringLiteral("*.tar.gz")
        };
        if (clean == QLatin1String("*") || clean == QLatin1String("*.")) {
            return exts;
        }
        QStringList matched;
        for (const auto &e : exts) {
            if (e.toLower().startsWith(clean)) matched.append(e);
        }
        return matched.isEmpty() ? exts : matched;
    }

    // 2. Size suggestions
    if (clean.startsWith(QLatin1Char('>'))) {
        return QStringList{
            QStringLiteral("> 10M"),
            QStringLiteral("> 50M"),
            QStringLiteral("> 100M"),
            QStringLiteral("> 500M"),
            QStringLiteral("> 1G")
        };
    }
    if (clean.startsWith(QLatin1Char('<'))) {
        return QStringList{
            QStringLiteral("< 10K"),
            QStringLiteral("< 100K"),
            QStringLiteral("< 1M"),
            QStringLiteral("< 10M")
        };
    }

    // 3. Type suggestions
    if (clean.startsWith(QLatin1String("type:"))) {
        return QStringList{
            QStringLiteral("type:file"),
            QStringLiteral("type:dir"),
            QStringLiteral("type:link"),
            QStringLiteral("type:exec")
        };
    }

    // 4. Default: return recent search history matching query or first 5 history items
    if (clean.isEmpty()) {
        return m_searchHistory;
    }

    QStringList historyMatches;
    for (const auto &h : m_searchHistory) {
        if (h.toLower().contains(clean)) {
            historyMatches.append(h);
        }
    }
    return historyMatches;
}

void SearchManager::commitSearchHistory(const QString &query)
{
    if (m_historyTimer && m_historyTimer->isActive()) {
        m_historyTimer->stop();
    }
    m_pendingHistoryQuery.clear();
    const QString trimmed = query.trimmed();
    if (trimmed.length() >= 2) {
        saveHistory(trimmed);
    }
}

void SearchManager::saveHistory(const QString &query)
{
    if (query.trimmed().isEmpty()) return;

    m_searchHistory.removeAll(query);
    m_searchHistory.prepend(query);
    while (m_searchHistory.size() > 10) {
        m_searchHistory.removeLast();
    }
    emit searchHistoryChanged();

    QSettings settings(QDir::homePath() + QStringLiteral("/.config/klarity/search_history.ini"), QSettings::IniFormat);
    settings.setValue(QStringLiteral("history"), m_searchHistory);
}

void SearchManager::loadHistory()
{
    QSettings settings(QDir::homePath() + QStringLiteral("/.config/klarity/search_history.ini"), QSettings::IniFormat);
    m_searchHistory = settings.value(QStringLiteral("history")).toStringList();
    emit searchHistoryChanged();
}

void SearchManager::clearHistory()
{
    m_searchHistory.clear();
    emit searchHistoryChanged();

    QSettings settings(QDir::homePath() + QStringLiteral("/.config/klarity/search_history.ini"), QSettings::IniFormat);
    settings.remove(QStringLiteral("history"));
}

QString SearchManager::formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString::number(bytes) + QStringLiteral(" B");
    }
    if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 1) + QStringLiteral(" KB");
    }
    if (bytes < 1024 * 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + QStringLiteral(" MB");
    }
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + QStringLiteral(" GB");
}
