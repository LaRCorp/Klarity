#include "PackageManager.h"
#include "core/LogModel.h"
#include "core/PkexecRunner.h"
#include "core/ProcessRunner.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QThread>
#include <KDesktopFile>
#include <KConfigGroup>

bool PackageManager::s_hasStaticCache = false;
QVector<PackageInfo> PackageManager::s_staticPackages;
int PackageManager::s_staticGuiCount = 0;
int PackageManager::s_staticExplicitCount = 0;
int PackageManager::s_staticPacmanCount = 0;
int PackageManager::s_staticAurCount = 0;
int PackageManager::s_staticFlatpakCount = 0;
int PackageManager::s_staticSnapCount = 0;

void PackageManager::updateStaticCache(const QVector<PackageInfo> &pkgs, int gCount, int eCount, int pCount, int aCount, int fCount, int sCount)
{
    s_staticPackages = pkgs;
    s_staticGuiCount = gCount;
    s_staticExplicitCount = eCount;
    s_staticPacmanCount = pCount;
    s_staticAurCount = aCount;
    s_staticFlatpakCount = fCount;
    s_staticSnapCount = sCount;
    s_hasStaticCache = true;
}

static QString getPackagesCacheFilePath()
{
    const QString cacheDir = QDir::homePath() + QStringLiteral("/.cache/klarity");
    QDir().mkpath(cacheDir);
    return cacheDir + QStringLiteral("/packages_cache.json");
}

PackageManager::PackageManager(QObject *parent)
    : QObject(parent)
{
    if (s_hasStaticCache) {
        m_packages = s_staticPackages;
        m_guiCount = s_staticGuiCount;
        m_explicitCount = s_staticExplicitCount;
        m_pacmanCount = s_staticPacmanCount;
        m_aurCount = s_staticAurCount;
        m_flatpakCount = s_staticFlatpakCount;
        m_snapCount = s_staticSnapCount;
        m_isLoading = false;
    } else if (loadFromDiskCache()) {
        m_isLoading = false;
    } else {
        loadPackages(false);
    }
}

QVariantList PackageManager::packages() const
{
    QVariantList list;
    list.reserve(m_packages.size());
    for (const auto &pkg : m_packages) {
        list.append(pkg.toMap());
    }
    return list;
}

bool PackageManager::loadFromDiskCache()
{
    const QString path = getPackagesCacheFilePath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        return false;
    }

    const QJsonObject root = doc.object();
    m_guiCount = root.value(QStringLiteral("guiCount")).toInt();
    m_explicitCount = root.value(QStringLiteral("explicitCount")).toInt();
    m_pacmanCount = root.value(QStringLiteral("pacmanCount")).toInt();
    m_aurCount = root.value(QStringLiteral("aurCount")).toInt();
    m_flatpakCount = root.value(QStringLiteral("flatpakCount")).toInt();
    m_snapCount = root.value(QStringLiteral("snapCount")).toInt();

    const QJsonArray arr = root.value(QStringLiteral("packages")).toArray();
    m_packages.clear();
    m_packages.reserve(arr.size());

    for (const auto &val : arr) {
        const QJsonObject obj = val.toObject();
        PackageInfo pkg;
        pkg.id = obj.value(QStringLiteral("id")).toString();
        pkg.name = obj.value(QStringLiteral("name")).toString();
        pkg.version = obj.value(QStringLiteral("version")).toString();
        pkg.source = obj.value(QStringLiteral("source")).toString();
        pkg.size = obj.value(QStringLiteral("size")).toString();
        pkg.sizeBytes = static_cast<qint64>(obj.value(QStringLiteral("sizeBytes")).toDouble());
        pkg.description = obj.value(QStringLiteral("description")).toString();
        pkg.icon = obj.value(QStringLiteral("icon")).toString();
        pkg.isGui = obj.value(QStringLiteral("isGui")).toBool();
        pkg.isExplicit = obj.value(QStringLiteral("isExplicit")).toBool(true);
        pkg.hasUpdate = obj.value(QStringLiteral("hasUpdate")).toBool();
        pkg.exec = obj.value(QStringLiteral("exec")).toString();
        pkg.category = obj.value(QStringLiteral("category")).toString(QStringLiteral("Utilities"));
        m_packages.append(pkg);
    }

    updateStaticCache(m_packages, m_guiCount, m_explicitCount, m_pacmanCount, m_aurCount, m_flatpakCount, m_snapCount);
    emit packagesChanged();
    return true;
}

void PackageManager::saveToDiskCache() const
{
    const QString path = getPackagesCacheFilePath();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("guiCount"), m_guiCount);
    root.insert(QStringLiteral("explicitCount"), m_explicitCount);
    root.insert(QStringLiteral("pacmanCount"), m_pacmanCount);
    root.insert(QStringLiteral("aurCount"), m_aurCount);
    root.insert(QStringLiteral("flatpakCount"), m_flatpakCount);
    root.insert(QStringLiteral("snapCount"), m_snapCount);

    QJsonArray arr;
    for (const auto &pkg : m_packages) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), pkg.id);
        obj.insert(QStringLiteral("name"), pkg.name);
        obj.insert(QStringLiteral("version"), pkg.version);
        obj.insert(QStringLiteral("source"), pkg.source);
        obj.insert(QStringLiteral("size"), pkg.size);
        obj.insert(QStringLiteral("sizeBytes"), pkg.sizeBytes);
        obj.insert(QStringLiteral("description"), pkg.description);
        obj.insert(QStringLiteral("icon"), pkg.icon);
        obj.insert(QStringLiteral("isGui"), pkg.isGui);
        obj.insert(QStringLiteral("isExplicit"), pkg.isExplicit);
        obj.insert(QStringLiteral("hasUpdate"), pkg.hasUpdate);
        obj.insert(QStringLiteral("exec"), pkg.exec);
        obj.insert(QStringLiteral("category"), pkg.category);
        arr.append(obj);
    }

    root.insert(QStringLiteral("packages"), arr);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.close();
}

QHash<QString, PackageManager::DesktopMeta> PackageManager::collectDesktopMap()
{
    QHash<QString, DesktopMeta> map;
    const QStringList appDirs = {
        QStringLiteral("/usr/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/applications"),
        QStringLiteral("/var/lib/flatpak/exports/share/applications"),
        QDir::homePath() + QStringLiteral("/.local/share/flatpak/exports/share/applications")
    };

    for (const auto &dirPath : appDirs) {
        QDir dir(dirPath);
        const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files | QDir::Readable);

        for (const auto &fi : files) {
            KDesktopFile df(fi.absoluteFilePath());
            const QString name = df.readName();
            const QString icon = df.readIcon();
            const QString exec = df.desktopGroup().readEntry(QStringLiteral("Exec"), QString());
            const bool noDisplay = df.noDisplay();
            const QStringList catList = df.desktopGroup().readEntry(QStringLiteral("Categories"), QString()).split(QLatin1Char(';'), Qt::SkipEmptyParts);

            QString category = QStringLiteral("Utilities");
            for (const auto &c : catList) {
                const QString trimmed = c.trimmed();
                if (trimmed == QLatin1String("Network") || trimmed == QLatin1String("WebBrowser") || trimmed == QLatin1String("Email") || trimmed == QLatin1String("Chat")) {
                    category = QStringLiteral("Internet");
                    break;
                }
                if (trimmed == QLatin1String("Development") || trimmed == QLatin1String("IDE")) {
                    category = QStringLiteral("Development");
                    break;
                }
                if (trimmed == QLatin1String("Graphics") || trimmed == QLatin1String("Photography") || trimmed == QLatin1String("2DGraphics") || trimmed == QLatin1String("RasterGraphics")) {
                    category = QStringLiteral("Graphics");
                    break;
                }
                if (trimmed == QLatin1String("AudioVideo") || trimmed == QLatin1String("Audio") || trimmed == QLatin1String("Video") || trimmed == QLatin1String("Player") || trimmed == QLatin1String("Music")) {
                    category = QStringLiteral("Multimedia");
                    break;
                }
                if (trimmed == QLatin1String("Office") || trimmed == QLatin1String("WordProcessor") || trimmed == QLatin1String("Spreadsheet")) {
                    category = QStringLiteral("Office");
                    break;
                }
                if (trimmed == QLatin1String("Game")) {
                    category = QStringLiteral("Games");
                    break;
                }
                if (trimmed == QLatin1String("System") || trimmed == QLatin1String("Settings") || trimmed == QLatin1String("PackageManager") || trimmed == QLatin1String("Monitor")) {
                    category = QStringLiteral("System");
                    break;
                }
            }

            if (!name.isEmpty()) {
                DesktopMeta meta{name, icon.isEmpty() ? QStringLiteral("application-x-executable") : icon, exec, category, noDisplay};
                const QString baseName = fi.baseName().toLower();
                map.insert(baseName, meta);

                if (!exec.isEmpty()) {
                    QString exeName = exec.split(QLatin1Char(' '), Qt::SkipEmptyParts).first();
                    exeName = QFileInfo(exeName).fileName().toLower();
                    if (!exeName.isEmpty() && exeName != QStringLiteral("bash") && exeName != QStringLiteral("sh") && exeName != QStringLiteral("env") && exeName != QStringLiteral("python") && exeName != QStringLiteral("perl") && exeName != QStringLiteral("ruby")) {
                        map.insert(exeName, meta);
                    }
                }
            }
        }
    }
    return map;
}

QHash<QString, QString> PackageManager::collectPacmanGuiMap()
{
    QHash<QString, QString> guiMap; // packageName -> desktopBaseName
    const QString localDbPath = QStringLiteral("/var/lib/pacman/local");
    QDir dir(localDbPath);
    if (!dir.exists()) {
        return guiMap;
    }

    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        const QString filesPath = localDbPath + QLatin1Char('/') + entry + QStringLiteral("/files");
        QFile file(filesPath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const QByteArray content = file.readAll();
        file.close();

        if (content.contains("usr/share/applications/")) {
            // Read package name from desc file
            const QString descPath = localDbPath + QLatin1Char('/') + entry + QStringLiteral("/desc");
            QFile descFile(descPath);
            if (descFile.open(QIODevice::ReadOnly)) {
                QTextStream in(&descFile);
                QString line;
                QString pkgName;
                while (!in.atEnd()) {
                    line = in.readLine();
                    if (line == QStringLiteral("%NAME%")) {
                        pkgName = in.readLine().trimmed();
                        break;
                    }
                }
                descFile.close();

                if (!pkgName.isEmpty()) {
                    // Extract desktop base name from content
                    const QStringList lines = QString::fromUtf8(content).split(QLatin1Char('\n'));
                    for (const auto &l : lines) {
                        if (l.contains(QStringLiteral("usr/share/applications/")) && l.endsWith(QStringLiteral(".desktop"))) {
                            const QString base = QFileInfo(l).baseName().toLower();
                            guiMap.insert(pkgName.toLower(), base);
                            break;
                        }
                    }
                }
            }
        }
    }

    return guiMap;
}

void PackageManager::collectPacmanAndAur(const QHash<QString, DesktopMeta> &desktopMap,
                                         const QHash<QString, QString> &guiMap,
                                         QVector<PackageInfo> &pkgs,
                                         int &pCount, int &aCount, int &gCount, int &eCount)
{
    // 1. Get foreign/AUR packages list
    QSet<QString> aurSet;
    QProcess qmProc;
    qmProc.start(QStringLiteral("pacman"), {QStringLiteral("-Qm")});
    qmProc.waitForFinished(10000);

    if (qmProc.exitCode() == 0) {
        const QString qmOut = QString::fromUtf8(qmProc.readAllStandardOutput());
        const QStringList lines = qmOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            const QString pkgName = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).first().trimmed();
            if (!pkgName.isEmpty()) {
                aurSet.insert(pkgName);
            }
        }
    }

    // 2. Get explicitly installed packages list
    QSet<QString> explicitSet;
    QProcess qeProc;
    qeProc.start(QStringLiteral("pacman"), {QStringLiteral("-Qe")});
    qeProc.waitForFinished(10000);

    if (qeProc.exitCode() == 0) {
        const QString qeOut = QString::fromUtf8(qeProc.readAllStandardOutput());
        const QStringList lines = qeOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            const QString pkgName = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).first().trimmed();
            if (!pkgName.isEmpty()) {
                explicitSet.insert(pkgName);
            }
        }
    }

    // 3. Get all installed packages
    QProcess qProc;
    qProc.start(QStringLiteral("pacman"), {QStringLiteral("-Q")});
    qProc.waitForFinished(15000);

    if (qProc.exitCode() == 0) {
        const QString qOut = QString::fromUtf8(qProc.readAllStandardOutput());
        const QStringList lines = qOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        for (const auto &line : lines) {
            const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (parts.size() < 2) continue;

            const QString name = parts[0].trimmed();
            const QString version = parts[1].trimmed();
            const bool isAur = aurSet.contains(name);
            const bool isExplicit = explicitSet.contains(name);
            const QString source = isAur ? QStringLiteral("AUR") : QStringLiteral("Pacman");

            PackageInfo pkg;
            pkg.id = QStringLiteral("%1:%2").arg(source.toLower(), name);
            pkg.name = name;
            pkg.version = version;
            pkg.source = source;
            pkg.size = QStringLiteral("-");
            pkg.isExplicit = isExplicit;

            if (isExplicit) {
                eCount++;
            }

            const QString lowerName = name.toLower();
            QString desktopKey;
            if (guiMap.contains(lowerName)) {
                desktopKey = guiMap.value(lowerName);
            } else if (desktopMap.contains(lowerName) && isExplicit) {
                desktopKey = lowerName;
            }

            if (!desktopKey.isEmpty() && desktopMap.contains(desktopKey)) {
                const auto &meta = desktopMap.value(desktopKey);
                pkg.isGui = !meta.noDisplay;
                pkg.icon = meta.icon;
                pkg.description = meta.displayName;
                pkg.exec = meta.exec;
                pkg.category = meta.category;
                if (pkg.isGui) {
                    gCount++;
                }
            } else {
                pkg.isGui = false;
                pkg.icon = QStringLiteral("package-x-generic");
                pkg.description = QString();
                pkg.category = QStringLiteral("System");
            }

            if (isAur) {
                aCount++;
            } else {
                pCount++;
            }
            pkgs.append(pkg);
        }
    }
}

void PackageManager::collectFlatpak(const QHash<QString, DesktopMeta> &desktopMap, QVector<PackageInfo> &pkgs, int &fCount, int &gCount)
{
    if (QStandardPaths::findExecutable(QStringLiteral("flatpak")).isEmpty()) {
        return;
    }

    QProcess proc;
    proc.start(QStringLiteral("flatpak"), {
        QStringLiteral("list"),
        QStringLiteral("--app"),
        QStringLiteral("--columns=application,name,version,size,description")
    });
    proc.waitForFinished(10000);

    if (proc.exitCode() != 0) {
        return;
    }

    const QString out = QString::fromUtf8(proc.readAllStandardOutput());
    const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const auto &line : lines) {
        const QStringList cols = line.split(QLatin1Char('\t'));
        if (cols.size() < 2) continue;

        const QString appId = cols[0].trimmed();
        const QString name = cols.size() >= 2 ? cols[1].trimmed() : appId;
        const QString version = cols.size() >= 3 ? cols[2].trimmed() : QStringLiteral("-");
        const QString size = cols.size() >= 4 ? cols[3].trimmed() : QStringLiteral("-");
        const QString desc = cols.size() >= 5 ? cols[4].trimmed() : QString();

        PackageInfo pkg;
        pkg.id = QStringLiteral("flatpak:%1").arg(appId);
        pkg.name = name.isEmpty() ? appId : name;
        pkg.version = version.isEmpty() ? QStringLiteral("-") : version;
        pkg.source = QStringLiteral("Flatpak");
        pkg.size = size.isEmpty() ? QStringLiteral("-") : size;
        pkg.description = desc.isEmpty() ? appId : desc;
        pkg.icon = appId;
        pkg.isGui = true;
        pkg.isExplicit = true;
        pkg.exec = QStringLiteral("flatpak run %1").arg(appId);
        pkg.category = QStringLiteral("Utilities");

        const QString lowerId = appId.toLower();
        if (desktopMap.contains(lowerId)) {
            const auto &meta = desktopMap.value(lowerId);
            pkg.category = meta.category;
            if (!meta.icon.isEmpty()) {
                pkg.icon = meta.icon;
            }
        }

        fCount++;
        gCount++;
        pkgs.append(pkg);
    }
}

void PackageManager::collectSnap(QVector<PackageInfo> &pkgs, int &sCount, int &gCount)
{
    if (QStandardPaths::findExecutable(QStringLiteral("snap")).isEmpty()) {
        return;
    }

    QProcess proc;
    proc.start(QStringLiteral("snap"), {QStringLiteral("list")});
    proc.waitForFinished(8000);

    if (proc.exitCode() != 0) {
        return;
    }

    const QString out = QString::fromUtf8(proc.readAllStandardOutput());
    const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (int i = 1; i < lines.size(); ++i) {
        const QStringList parts = lines[i].split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        const QString name = parts[0].trimmed();
        const QString version = parts[1].trimmed();

        PackageInfo pkg;
        pkg.id = QStringLiteral("snap:%1").arg(name);
        pkg.name = name;
        pkg.version = version;
        pkg.source = QStringLiteral("Snap");
        pkg.size = QStringLiteral("-");
        pkg.description = QStringLiteral("Snap Package");
        pkg.icon = QStringLiteral("package-x-generic");
        pkg.isGui = false;
        pkg.isExplicit = true;

        sCount++;
        pkgs.append(pkg);
    }
}

void PackageManager::loadPackagesSync()
{
    m_isLoading = true;
    emit isLoadingChanged();

    QVector<PackageInfo> pkgs;
    int pCount = 0;
    int aCount = 0;
    int fCount = 0;
    int sCount = 0;
    int gCount = 0;
    int eCount = 0;

    auto desktopMap = collectDesktopMap();
    auto guiMap = collectPacmanGuiMap();
    collectPacmanAndAur(desktopMap, guiMap, pkgs, pCount, aCount, gCount, eCount);
    collectFlatpak(desktopMap, pkgs, fCount, gCount);
    collectSnap(pkgs, sCount, gCount);

    m_packages = pkgs;
    m_guiCount = gCount;
    m_explicitCount = eCount;
    m_pacmanCount = pCount;
    m_aurCount = aCount;
    m_flatpakCount = fCount;
    m_snapCount = sCount;
    m_isLoading = false;

    updateStaticCache(m_packages, m_guiCount, m_explicitCount, m_pacmanCount, m_aurCount, m_flatpakCount, m_snapCount);
    saveToDiskCache();

    emit isLoadingChanged();
    emit packagesChanged();
}

void PackageManager::loadPackages(bool forceRefresh)
{
    if (!forceRefresh && s_hasStaticCache) {
        m_packages = s_staticPackages;
        m_guiCount = s_staticGuiCount;
        m_explicitCount = s_staticExplicitCount;
        m_pacmanCount = s_staticPacmanCount;
        m_aurCount = s_staticAurCount;
        m_flatpakCount = s_staticFlatpakCount;
        m_snapCount = s_staticSnapCount;
        emit packagesChanged();
        return;
    }

    if (m_isLoading) return;

    m_isLoading = true;
    emit isLoadingChanged();

    QThread *thread = QThread::create([this]() {
        QVector<PackageInfo> pkgs;
        int pCount = 0;
        int aCount = 0;
        int fCount = 0;
        int sCount = 0;
        int gCount = 0;
        int eCount = 0;

        auto desktopMap = collectDesktopMap();
        auto guiMap = collectPacmanGuiMap();
        collectPacmanAndAur(desktopMap, guiMap, pkgs, pCount, aCount, gCount, eCount);
        collectFlatpak(desktopMap, pkgs, fCount, gCount);
        collectSnap(pkgs, sCount, gCount);

        QMetaObject::invokeMethod(this, [this, pkgs, gCount, eCount, pCount, aCount, fCount, sCount]() {
            m_packages = pkgs;
            m_guiCount = gCount;
            m_explicitCount = eCount;
            m_pacmanCount = pCount;
            m_aurCount = aCount;
            m_flatpakCount = fCount;
            m_snapCount = sCount;
            m_isLoading = false;

            updateStaticCache(m_packages, m_guiCount, m_explicitCount, m_pacmanCount, m_aurCount, m_flatpakCount, m_snapCount);
            saveToDiskCache();

            emit isLoadingChanged();
            emit packagesChanged();

            LogModel::instance()->append(
                QStringLiteral("Packages"),
                QStringLiteral("info"),
                QStringLiteral("Loaded %1 packages: GUI=%2, Explicit=%3, Pacman=%4, AUR=%5, Flatpak=%6")
                    .arg(m_packages.size())
                    .arg(m_guiCount)
                    .arg(m_explicitCount)
                    .arg(m_pacmanCount)
                    .arg(m_aurCount)
                    .arg(m_flatpakCount)
            );
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void PackageManager::checkUpdates()
{
    if (QStandardPaths::findExecutable(QStringLiteral("checkupdates")).isEmpty()) {
        return;
    }

    auto runner = new ProcessRunner(this);
    connect(runner, &ProcessRunner::finished, this, [this](int exitCode, const QString &output) {
        if (exitCode == 0 || exitCode == 2) {
            const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            QSet<QString> upgradable;
            for (const auto &line : lines) {
                const QString pkgName = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).first().trimmed();
                if (!pkgName.isEmpty()) upgradable.insert(pkgName);
            }

            m_updateCount = upgradable.size();
            for (auto &pkg : m_packages) {
                if (pkg.source == QStringLiteral("Pacman") && upgradable.contains(pkg.name)) {
                    pkg.hasUpdate = true;
                }
            }
            emit updatesChecked();
            emit packagesChanged();

            LogModel::instance()->append(
                QStringLiteral("Packages"),
                QStringLiteral("info"),
                QStringLiteral("Found %1 available package update(s)").arg(m_updateCount)
            );
        }
    });

    runner->runAsync(QStringLiteral("checkupdates"));
}

void PackageManager::removePackage(const QString &source, const QString &name)
{
    if (name.isEmpty()) return;

    m_isLoading = true;
    emit isLoadingChanged();

    if (source == QStringLiteral("Pacman") || source == QStringLiteral("AUR")) {
        auto runner = new PkexecRunner(this);
        connect(runner, &PkexecRunner::finished, this, [this, name](int exitCode, const QString &output) {
            m_isLoading = false;
            emit isLoadingChanged();

            if (exitCode == 0) {
                LogModel::instance()->append(QStringLiteral("Packages"), QStringLiteral("info"), QStringLiteral("Removed package: %1").arg(name));
                emit operationFinished(true, QStringLiteral("Package %1 removed successfully.").arg(name));
                loadPackages(true);
            } else if (exitCode == 126 || exitCode == 127) {
                emit operationFinished(false, QStringLiteral("Package removal cancelled."));
            } else {
                emit operationFinished(false, QStringLiteral("Failed to remove package: %1").arg(output));
            }
        });

        runner->runRoot(QStringLiteral("pacman"), {QStringLiteral("-Rns"), QStringLiteral("--noconfirm"), name});
    } else if (source == QStringLiteral("Flatpak")) {
        auto runner = new ProcessRunner(this);
        connect(runner, &ProcessRunner::finished, this, [this, name](int exitCode, const QString &output) {
            m_isLoading = false;
            emit isLoadingChanged();

            if (exitCode == 0) {
                LogModel::instance()->append(QStringLiteral("Packages"), QStringLiteral("info"), QStringLiteral("Uninstalled Flatpak app: %1").arg(name));
                emit operationFinished(true, QStringLiteral("Flatpak %1 uninstalled successfully.").arg(name));
                loadPackages(true);
            } else {
                emit operationFinished(false, QStringLiteral("Failed to uninstall Flatpak: %1").arg(output));
            }
        });

        runner->runAsync(QStringLiteral("flatpak"), {QStringLiteral("uninstall"), QStringLiteral("-y"), name});
    } else if (source == QStringLiteral("Snap")) {
        auto runner = new PkexecRunner(this);
        connect(runner, &PkexecRunner::finished, this, [this, name](int exitCode, const QString &output) {
            m_isLoading = false;
            emit isLoadingChanged();

            if (exitCode == 0) {
                emit operationFinished(true, QStringLiteral("Snap %1 removed successfully.").arg(name));
                loadPackages(true);
            } else {
                emit operationFinished(false, QStringLiteral("Failed to remove Snap: %1").arg(output));
            }
        });

        runner->runRoot(QStringLiteral("snap"), {QStringLiteral("remove"), name});
    }
}

QVariantMap PackageManager::getPackageDetails(const QString &source, const QString &name)
{
    QVariantMap details;
    details.insert(QStringLiteral("name"), name);
    details.insert(QStringLiteral("source"), source);

    if (source == QStringLiteral("Pacman") || source == QStringLiteral("AUR")) {
        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
        proc.setProcessEnvironment(env);
        proc.start(QStringLiteral("pacman"), {QStringLiteral("-Qi"), name});
        proc.waitForFinished(5000);

        if (proc.exitCode() == 0) {
            const QString out = QString::fromUtf8(proc.readAllStandardOutput());
            const QStringList lines = out.split(QLatin1Char('\n'));

            for (const auto &line : lines) {
                const int col = line.indexOf(QLatin1Char(':'));
                if (col != -1) {
                    const QString key = line.left(col).trimmed();
                    const QString val = line.mid(col + 1).trimmed();

                    if (key == QStringLiteral("Version") || key == QStringLiteral("Версия")) details.insert(QStringLiteral("version"), val);
                    else if (key == QStringLiteral("Description") || key == QStringLiteral("Описание")) details.insert(QStringLiteral("description"), val);
                    else if (key == QStringLiteral("URL")) details.insert(QStringLiteral("url"), val);
                    else if (key == QStringLiteral("Licenses") || key == QStringLiteral("Лицензии")) details.insert(QStringLiteral("license"), val);
                    else if (key == QStringLiteral("Installed Size") || key == QStringLiteral("Установленный размер")) details.insert(QStringLiteral("size"), val);
                    else if (key == QStringLiteral("Packager") || key == QStringLiteral("Сборщик")) details.insert(QStringLiteral("packager"), val);
                    else if (key == QStringLiteral("Build Date") || key == QStringLiteral("Дата сборки")) details.insert(QStringLiteral("buildDate"), val);
                    else if (key == QStringLiteral("Depends On") || key == QStringLiteral("Зависит от")) details.insert(QStringLiteral("dependsOn"), val);
                    else if (key == QStringLiteral("Required By") || key == QStringLiteral("Требуется")) details.insert(QStringLiteral("requiredBy"), val);
                }
            }
        }
    } else if (source == QStringLiteral("Flatpak")) {
        QProcess proc;
        proc.start(QStringLiteral("flatpak"), {QStringLiteral("info"), name});
        proc.waitForFinished(5000);

        if (proc.exitCode() == 0) {
            const QString out = QString::fromUtf8(proc.readAllStandardOutput());
            details.insert(QStringLiteral("description"), out);
        }
    }

    return details;
}

bool PackageManager::launchPackage(const QString &source, const QString &name)
{
    if (source == QStringLiteral("Flatpak")) {
        return QProcess::startDetached(QStringLiteral("flatpak"), {QStringLiteral("run"), name});
    }

    for (const auto &pkg : m_packages) {
        if (pkg.name == name) {
            if (!pkg.exec.isEmpty()) {
                QString cmd = pkg.exec;
                // Remove field codes like %u, %U, %f, %F, %i, %c, %k
                cmd.remove(QRegularExpression(QStringLiteral("%[uUfFick]")));
                cmd = cmd.trimmed();

                const QStringList parts = cmd.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                if (!parts.isEmpty()) {
                    const QString prog = parts.first();
                    const QStringList args = parts.mid(1);
                    return QProcess::startDetached(prog, args);
                }
            }
            break;
        }
    }

    // Fallback: try gtk-launch with package name
    return QProcess::startDetached(QStringLiteral("gtk-launch"), {name});
}
