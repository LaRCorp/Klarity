#include "AliasManager.h"
#include "core/ProcessRunner.h"
#include "core/LogModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QTextStream>

AliasManager::AliasManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QVariantList AliasManager::entries() const
{
    QVariantList list;
    list.reserve(m_items.size());
    for (const auto &item : m_items) {
        list.append(item.toMap());
    }
    return list;
}

void AliasManager::load()
{
    m_isLoading = true;
    emit isLoadingChanged();

    m_items.clear();

    loadFish();
    loadBash();
    loadZsh();

    m_isLoading = false;
    emit isLoadingChanged();
    emit entriesChanged();
}

void AliasManager::loadFish()
{
    const QString fishFuncDir = QDir::homePath() + QStringLiteral("/.config/fish/functions");
    const QString fishConfigFile = QDir::homePath() + QStringLiteral("/.config/fish/config.fish");

    // 1. Scan fish function files in ~/.config/fish/functions/*.fish
    QDir dir(fishFuncDir);
    if (dir.exists()) {
        const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.fish")}, QDir::Files | QDir::Readable);
        for (const auto &fi : files) {
            QFile file(fi.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }

            const QString content = QString::fromUtf8(file.readAll());
            file.close();

            // Extract body between 'function <name> ...' and 'end'
            QString body = content.trimmed();
            QRegularExpression reFunc(QStringLiteral("function\\s+([a-zA-Z0-9_.-]+)[^\\n]*\\n([\\s\\S]*?)\\n\\s*end"),
                                      QRegularExpression::CaseInsensitiveOption);
            auto match = reFunc.match(content);
            if (match.hasMatch()) {
                body = match.captured(2).trimmed();
            }

            AliasItem item;
            item.id = QStringLiteral("fish:func:") + fi.fileName();
            item.name = fi.completeBaseName();
            item.command = body;
            item.shell = QStringLiteral("Fish");
            item.type = QStringLiteral("Function");
            item.filePath = fi.absoluteFilePath();
            item.lineNumber = -1;
            item.description = QStringLiteral("Fish function in %1").arg(fi.fileName());

            m_items.append(item);
        }
    }

    // 2. Scan fish aliases / abbreviations in config.fish
    QFile configFile(fishConfigFile);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        int lineNum = 0;
        QRegularExpression reAlias(QStringLiteral("^\\s*alias\\s+([a-zA-Z0-9_.-]+)(?:=|[\\s]+)(?:'([^']*)'|\"([^\"]*)\"|(\\S+))"));
        QRegularExpression reAbbr(QStringLiteral("^\\s*abbr\\s+(?:-a\\s+)?([a-zA-Z0-9_.-]+)[\\s]+(?:'([^']*)'|\"([^\"]*)\"|(\\S+))"));

        while (!in.atEnd()) {
            lineNum++;
            const QString line = in.readLine();
            auto match = reAlias.match(line);
            if (match.hasMatch()) {
                const QString name = match.captured(1);
                QString val = match.captured(2);
                if (val.isEmpty()) val = match.captured(3);
                if (val.isEmpty()) val = match.captured(4);

                AliasItem item;
                item.id = QStringLiteral("fish:alias:%1:%2").arg(name, QString::number(lineNum));
                item.name = name;
                item.command = val;
                item.shell = QStringLiteral("Fish");
                item.type = QStringLiteral("Alias");
                item.filePath = fishConfigFile;
                item.lineNumber = lineNum;
                item.description = QStringLiteral("Fish alias in config.fish");
                m_items.append(item);
                continue;
            }

            match = reAbbr.match(line);
            if (match.hasMatch()) {
                const QString name = match.captured(1);
                QString val = match.captured(2);
                if (val.isEmpty()) val = match.captured(3);
                if (val.isEmpty()) val = match.captured(4);

                AliasItem item;
                item.id = QStringLiteral("fish:abbr:%1:%2").arg(name, QString::number(lineNum));
                item.name = name;
                item.command = val;
                item.shell = QStringLiteral("Fish");
                item.type = QStringLiteral("Alias");
                item.filePath = fishConfigFile;
                item.lineNumber = lineNum;
                item.description = QStringLiteral("Fish abbreviation in config.fish");
                m_items.append(item);
            }
        }
        configFile.close();
    }
}

void AliasManager::loadBash()
{
    const QStringList bashFiles = {
        QDir::homePath() + QStringLiteral("/.bash_aliases"),
        QDir::homePath() + QStringLiteral("/.bashrc")
    };

    QRegularExpression reAlias(QStringLiteral("^\\s*alias\\s+([a-zA-Z0-9_.-]+)=(?:'([^']*)'|\"([^\"]*)\"|(\\S+))"));

    for (const auto &filePath : bashFiles) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream in(&file);
        int lineNum = 0;
        while (!in.atEnd()) {
            lineNum++;
            const QString line = in.readLine();
            auto match = reAlias.match(line);
            if (match.hasMatch()) {
                const QString name = match.captured(1);
                QString val = match.captured(2);
                if (val.isEmpty()) val = match.captured(3);
                if (val.isEmpty()) val = match.captured(4);

                AliasItem item;
                item.id = QStringLiteral("bash:alias:%1:%2:%3").arg(QFileInfo(filePath).fileName(), name, QString::number(lineNum));
                item.name = name;
                item.command = val;
                item.shell = QStringLiteral("Bash");
                item.type = QStringLiteral("Alias");
                item.filePath = filePath;
                item.lineNumber = lineNum;
                item.description = QStringLiteral("Bash alias in %1").arg(QFileInfo(filePath).fileName());
                m_items.append(item);
            }
        }
        file.close();
    }
}

void AliasManager::loadZsh()
{
    const QString zshFile = QDir::homePath() + QStringLiteral("/.zshrc");
    QFile file(zshFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QRegularExpression reAlias(QStringLiteral("^\\s*alias\\s+([a-zA-Z0-9_.-]+)=(?:'([^']*)'|\"([^\"]*)\"|(\\S+))"));

    QTextStream in(&file);
    int lineNum = 0;
    while (!in.atEnd()) {
        lineNum++;
        const QString line = in.readLine();
        auto match = reAlias.match(line);
        if (match.hasMatch()) {
            const QString name = match.captured(1);
            QString val = match.captured(2);
            if (val.isEmpty()) val = match.captured(3);
            if (val.isEmpty()) val = match.captured(4);

            AliasItem item;
            item.id = QStringLiteral("zsh:alias:%1:%2").arg(name, QString::number(lineNum));
            item.name = name;
            item.command = val;
            item.shell = QStringLiteral("Zsh");
            item.type = QStringLiteral("Alias");
            item.filePath = zshFile;
            item.lineNumber = lineNum;
            item.description = QStringLiteral("Zsh alias in .zshrc");
            m_items.append(item);
        }
    }
    file.close();
}

QVariantMap AliasManager::validateSyntax(const QString &shell, const QString &type, const QString &name, const QString &command)
{
    QVariantMap res;
    res[QStringLiteral("valid")] = false;

    if (name.trimmed().isEmpty()) {
        res[QStringLiteral("error")] = QStringLiteral("Name cannot be empty.");
        return res;
    }

    if (!name.contains(QRegularExpression(QStringLiteral("^[a-zA-Z0-9_.-]+$")))) {
        res[QStringLiteral("error")] = QStringLiteral("Name contains invalid characters.");
        return res;
    }

    if (command.trimmed().isEmpty()) {
        res[QStringLiteral("error")] = QStringLiteral("Command cannot be empty.");
        return res;
    }

    // Syntax validation via shell -n
    if (shell == QStringLiteral("Fish")) {
        QString snippet;
        if (type == QStringLiteral("Function")) {
            snippet = QStringLiteral("function %1\n%2\nend").arg(name, command);
        } else {
            snippet = QStringLiteral("alias %1 '%2'").arg(name, command);
        }

        QTemporaryFile tempFile;
        if (tempFile.open()) {
            tempFile.write(snippet.toUtf8());
            tempFile.flush();
            const QString path = tempFile.fileName();

            const ProcessResult testRes = ProcessRunner::run(QStringLiteral("fish"), {QStringLiteral("-n"), path});
            if (testRes.exitCode == 0) {
                res[QStringLiteral("valid")] = true;
                res[QStringLiteral("error")] = QString();
            } else {
                res[QStringLiteral("error")] = testRes.stderrStr.trimmed();
            }
        }
    } else {
        // Bash or Zsh validation via bash -n
        QString snippet;
        if (type == QStringLiteral("Function")) {
            snippet = QStringLiteral("%1() {\n%2\n}").arg(name, command);
        } else {
            snippet = QStringLiteral("alias %1='%2'").arg(name, command);
        }

        QTemporaryFile tempFile;
        if (tempFile.open()) {
            tempFile.write(snippet.toUtf8());
            tempFile.flush();
            const QString path = tempFile.fileName();

            const ProcessResult testRes = ProcessRunner::run(QStringLiteral("bash"), {QStringLiteral("-n"), path});
            if (testRes.exitCode == 0) {
                res[QStringLiteral("valid")] = true;
                res[QStringLiteral("error")] = QString();
            } else {
                res[QStringLiteral("error")] = testRes.stderrStr.trimmed();
            }
        }
    }

    return res;
}

bool AliasManager::createEntry(const QString &shell, const QString &type, const QString &name, const QString &command)
{
    const QVariantMap val = validateSyntax(shell, type, name, command);
    if (!val.value(QStringLiteral("valid")).toBool()) {
        emit operationFinished(false, val.value(QStringLiteral("error")).toString());
        return false;
    }

    if (shell == QStringLiteral("Fish")) {
        if (type == QStringLiteral("Function")) {
            const QString dirPath = QDir::homePath() + QStringLiteral("/.config/fish/functions");
            QDir().mkpath(dirPath);
            const QString filePath = QStringLiteral("%1/%2.fish").arg(dirPath, name.trimmed());

            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                emit operationFinished(false, QStringLiteral("Cannot open file for writing: %1").arg(filePath));
                return false;
            }

            QTextStream out(&file);
            out << QStringLiteral("function %1\n    %2\nend\n").arg(name.trimmed(), command.trimmed());
            file.close();

            LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                        QStringLiteral("Created fish function file: %1").arg(filePath));
        } else {
            const QString configFile = QDir::homePath() + QStringLiteral("/.config/fish/config.fish");
            QFile file(configFile);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                emit operationFinished(false, QStringLiteral("Cannot open config.fish for appending."));
                return false;
            }

            QTextStream out(&file);
            out << QStringLiteral("\nalias %1 '%2'\n").arg(name.trimmed(), command.trimmed());
            file.close();

            LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                        QStringLiteral("Appended fish alias %1 to config.fish").arg(name.trimmed()));
        }
    } else if (shell == QStringLiteral("Bash")) {
        QString targetFile = QDir::homePath() + QStringLiteral("/.bash_aliases");
        if (!QFile::exists(targetFile)) {
            targetFile = QDir::homePath() + QStringLiteral("/.bashrc");
        }

        QFile file(targetFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            emit operationFinished(false, QStringLiteral("Cannot open %1 for appending.").arg(targetFile));
            return false;
        }

        QTextStream out(&file);
        if (type == QStringLiteral("Function")) {
            out << QStringLiteral("\nfunction %1() {\n    %2\n}\n").arg(name.trimmed(), command.trimmed());
        } else {
            out << QStringLiteral("\nalias %1='%2'\n").arg(name.trimmed(), command.trimmed());
        }
        file.close();

        LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                    QStringLiteral("Appended %1 to %2").arg(name.trimmed(), targetFile));
    } else {
        // Zsh
        const QString targetFile = QDir::homePath() + QStringLiteral("/.zshrc");
        QFile file(targetFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            emit operationFinished(false, QStringLiteral("Cannot open .zshrc for appending."));
            return false;
        }

        QTextStream out(&file);
        if (type == QStringLiteral("Function")) {
            out << QStringLiteral("\n%1() {\n    %2\n}\n").arg(name.trimmed(), command.trimmed());
        } else {
            out << QStringLiteral("\nalias %1='%2'\n").arg(name.trimmed(), command.trimmed());
        }
        file.close();

        LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                    QStringLiteral("Appended %1 to .zshrc").arg(name.trimmed()));
    }

    load();
    emit operationFinished(true, QStringLiteral("Entry created successfully."));
    return true;
}

bool AliasManager::saveEntry(const QString &id, const QString &name, const QString &command)
{
    // Find item by ID
    AliasItem target;
    bool found = false;
    for (const auto &item : m_items) {
        if (item.id == id) {
            target = item;
            found = true;
            break;
        }
    }

    if (!found) {
        emit operationFinished(false, QStringLiteral("Entry not found."));
        return false;
    }

    const QVariantMap val = validateSyntax(target.shell, target.type, name, command);
    if (!val.value(QStringLiteral("valid")).toBool()) {
        emit operationFinished(false, val.value(QStringLiteral("error")).toString());
        return false;
    }

    // 1. Separate file (Fish function)
    if (target.lineNumber == -1) {
        QFile file(target.filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            emit operationFinished(false, QStringLiteral("Cannot write to %1").arg(target.filePath));
            return false;
        }

        QTextStream out(&file);
        out << QStringLiteral("function %1\n    %2\nend\n").arg(name.trimmed(), command.trimmed());
        file.close();

        // If name changed, rename file
        const QString expectedPath = QFileInfo(target.filePath).dir().absoluteFilePath(name.trimmed() + QStringLiteral(".fish"));
        if (target.filePath != expectedPath) {
            QFile::rename(target.filePath, expectedPath);
        }

        LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                    QStringLiteral("Updated function in %1").arg(target.filePath));
        load();
        emit operationFinished(true, QStringLiteral("Function updated successfully."));
        return true;
    }

    // 2. Line in shared file (.bashrc, .bash_aliases, config.fish, .zshrc)
    QFile file(target.filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit operationFinished(false, QStringLiteral("Cannot read %1").arg(target.filePath));
        return false;
    }

    QStringList allLines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        allLines.append(in.readLine());
    }
    file.close();

    const int idx = target.lineNumber - 1;
    if (idx >= 0 && idx < allLines.size()) {
        QString newLine;
        if (target.shell == QStringLiteral("Fish")) {
            newLine = QStringLiteral("alias %1 '%2'").arg(name.trimmed(), command.trimmed());
        } else {
            newLine = QStringLiteral("alias %1='%2'").arg(name.trimmed(), command.trimmed());
        }
        allLines[idx] = newLine;

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            emit operationFinished(false, QStringLiteral("Cannot write to %1").arg(target.filePath));
            return false;
        }

        QTextStream out(&file);
        for (const auto &l : allLines) {
            out << l << QLatin1Char('\n');
        }
        file.close();

        LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                    QStringLiteral("Updated alias at line %1 in %2").arg(QString::number(target.lineNumber), target.filePath));
        load();
        emit operationFinished(true, QStringLiteral("Alias updated successfully."));
        return true;
    }

    emit operationFinished(false, QStringLiteral("Line index out of bounds."));
    return false;
}

bool AliasManager::deleteEntry(const QString &id)
{
    AliasItem target;
    bool found = false;
    for (const auto &item : m_items) {
        if (item.id == id) {
            target = item;
            found = true;
            break;
        }
    }

    if (!found) {
        emit operationFinished(false, QStringLiteral("Entry not found."));
        return false;
    }

    // Separate file
    if (target.lineNumber == -1) {
        const bool ok = QFile::remove(target.filePath);
        LogModel::instance()->append(
            QStringLiteral("Aliases"),
            ok ? QStringLiteral("info") : QStringLiteral("error"),
            QStringLiteral("Deleted function file: %1").arg(target.filePath)
        );
        load();
        emit operationFinished(ok, ok ? QStringLiteral("File deleted.") : QStringLiteral("Failed to delete file."));
        return ok;
    }

    // Line in file
    QFile file(target.filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit operationFinished(false, QStringLiteral("Cannot read file."));
        return false;
    }

    QStringList allLines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        allLines.append(in.readLine());
    }
    file.close();

    const int idx = target.lineNumber - 1;
    if (idx >= 0 && idx < allLines.size()) {
        allLines.removeAt(idx);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            emit operationFinished(false, QStringLiteral("Cannot write file."));
            return false;
        }

        QTextStream out(&file);
        for (const auto &l : allLines) {
            out << l << QLatin1Char('\n');
        }
        file.close();

        LogModel::instance()->append(QStringLiteral("Aliases"), QStringLiteral("info"),
                                    QStringLiteral("Removed line %1 from %2").arg(QString::number(target.lineNumber), target.filePath));
        load();
        emit operationFinished(true, QStringLiteral("Entry removed successfully."));
        return true;
    }

    emit operationFinished(false, QStringLiteral("Line not found."));
    return false;
}
