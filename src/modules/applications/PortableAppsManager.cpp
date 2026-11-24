#include "PortableAppsManager.h"
#include "core/LogModel.h"
#include "core/ProcessRunner.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <KDesktopFile>
#include <KConfigGroup>

bool PortableAppsManager::s_hasStaticCache = false;
QVector<PortableAppInfo> PortableAppsManager::s_staticApps;

void PortableAppsManager::updateStaticCache(const QVector<PortableAppInfo> &apps)
{
    s_staticApps = apps;
    s_hasStaticCache = true;
}

static QString getPortableCacheFilePath()
{
    const QString cacheDir = QDir::homePath() + QStringLiteral("/.cache/klarity");
    QDir().mkpath(cacheDir);
    return cacheDir + QStringLiteral("/portable_apps_cache.json");
}

PortableAppsManager::PortableAppsManager(QObject *parent)
    : QObject(parent)
{
    if (s_hasStaticCache) {
        m_apps = s_staticApps;
        m_isScanning = false;
    } else if (loadFromDiskCache()) {
        m_isScanning = false;
    } else {
        scanPortableApps(false);
    }
}

QVariantList PortableAppsManager::apps() const
{
    QVariantList list;
    list.reserve(m_apps.size());
    for (const auto &app : m_apps) {
        list.append(app.toMap());
    }
    return list;
}

QString PortableAppsManager::formatSize(qint64 bytes)
{
    if (bytes <= 0) return QStringLiteral("0 B");
    const double b = static_cast<double>(bytes);
    if (b < 1024.0) return QStringLiteral("%1 B").arg(bytes);
    if (b < 1024.0 * 1024.0) return QStringLiteral("%1 KiB").arg(b / 1024.0, 0, 'f', 1);
    if (b < 1024.0 * 1024.0 * 1024.0) return QStringLiteral("%1 MiB").arg(b / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 GiB").arg(b / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

bool PortableAppsManager::loadFromDiskCache()
{
    const QString path = getPortableCacheFilePath();
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
    const QJsonArray arr = root.value(QStringLiteral("apps")).toArray();

    m_apps.clear();
    m_apps.reserve(arr.size());

    for (const auto &val : arr) {
        const QJsonObject obj = val.toObject();
        PortableAppInfo app;
        app.id = obj.value(QStringLiteral("id")).toString();
        app.name = obj.value(QStringLiteral("name")).toString();
        app.path = obj.value(QStringLiteral("path")).toString();
        app.size = obj.value(QStringLiteral("size")).toString();
        app.sizeBytes = static_cast<qint64>(obj.value(QStringLiteral("sizeBytes")).toDouble());
        app.isAppImage = obj.value(QStringLiteral("isAppImage")).toBool();
        app.icon = obj.value(QStringLiteral("icon")).toString();
        app.isInMenu = obj.value(QStringLiteral("isInMenu")).toBool();
        app.lastModified = obj.value(QStringLiteral("lastModified")).toString();
        m_apps.append(app);
    }

    updateStaticCache(m_apps);
    emit appsChanged();
    return true;
}

void PortableAppsManager::saveToDiskCache() const
{
    const QString path = getPortableCacheFilePath();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QJsonObject root;
    QJsonArray arr;
    for (const auto &app : m_apps) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), app.id);
        obj.insert(QStringLiteral("name"), app.name);
        obj.insert(QStringLiteral("path"), app.path);
        obj.insert(QStringLiteral("size"), app.size);
        obj.insert(QStringLiteral("sizeBytes"), app.sizeBytes);
        obj.insert(QStringLiteral("isAppImage"), app.isAppImage);
        obj.insert(QStringLiteral("icon"), app.icon);
        obj.insert(QStringLiteral("isInMenu"), app.isInMenu);
        obj.insert(QStringLiteral("lastModified"), app.lastModified);
        arr.append(obj);
    }

    root.insert(QStringLiteral("apps"), arr);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.close();
}

bool PortableAppsManager::checkIfInMenu(const QString &execPath)
{
    const QString userAppsDir = QDir::homePath() + QStringLiteral("/.local/share/applications");
    QDir dir(userAppsDir);
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files | QDir::Readable);

    for (const auto &fi : files) {
        KDesktopFile df(fi.absoluteFilePath());
        const QString exec = df.desktopGroup().readEntry(QStringLiteral("Exec"), QString());
        if (exec.contains(execPath)) {
            return true;
        }
    }
    return false;
}

QVector<PortableAppInfo> PortableAppsManager::doScan()
{
    QVector<PortableAppInfo> result;
    const QStringList scanDirs = {
        QDir::homePath() + QStringLiteral("/Applications"),
        QDir::homePath() + QStringLiteral("/bin"),
        QDir::homePath() + QStringLiteral("/.local/bin"),
        QDir::homePath() + QStringLiteral("/Downloads"),
        QStringLiteral("/opt")
    };

    QSet<QString> seenPaths;
    int index = 0;

    for (const auto &dirPath : scanDirs) {
        if (!QFile::exists(dirPath)) {
            continue;
        }

        const bool isDownloads = dirPath.endsWith(QStringLiteral("Downloads"));

        QDirIterator it(dirPath, QDir::Files | QDir::Executable, isDownloads ? QDirIterator::NoIteratorFlags : QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fi = it.fileInfo();

            if (seenPaths.contains(fi.absoluteFilePath())) {
                continue;
            }

            const QString ext = fi.suffix().toLower();
            const bool isAppImage = (ext == QStringLiteral("appimage"));

            if (isDownloads) {
                if (!isAppImage && !ext.isEmpty()) {
                    continue;
                }
            }

            if (!fi.isExecutable()) {
                continue;
            }

            if (ext == QStringLiteral("sh") || ext == QStringLiteral("py") || ext == QStringLiteral("pl")) {
                continue;
            }

            seenPaths.insert(fi.absoluteFilePath());

            PortableAppInfo item;
            item.id = QStringLiteral("portapp_%1").arg(index++);
            item.name = fi.baseName();
            item.path = fi.absoluteFilePath();
            item.size = formatSize(fi.size());
            item.sizeBytes = fi.size();
            item.isAppImage = isAppImage;
            item.icon = isAppImage ? QStringLiteral("application-x-executable") : QStringLiteral("system-run");
            item.isInMenu = checkIfInMenu(item.path);
            item.lastModified = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"));

            result.append(item);
        }
    }
    return result;
}

void PortableAppsManager::scanPortableAppsSync()
{
    m_isScanning = true;
    emit isScanningChanged();

    m_apps = doScan();
    m_isScanning = false;

    updateStaticCache(m_apps);
    saveToDiskCache();

    emit isScanningChanged();
    emit appsChanged();
}

void PortableAppsManager::scanPortableApps(bool force)
{
    if (!force && s_hasStaticCache) {
        m_apps = s_staticApps;
        emit appsChanged();
        return;
    }

    if (m_isScanning) return;

    m_isScanning = true;
    emit isScanningChanged();

    QThread *thread = QThread::create([this]() {
        const auto result = doScan();

        QMetaObject::invokeMethod(this, [this, result]() {
            m_apps = result;
            m_isScanning = false;

            updateStaticCache(m_apps);
            saveToDiskCache();

            emit isScanningChanged();
            emit appsChanged();

            LogModel::instance()->append(
                QStringLiteral("PortableApps"),
                QStringLiteral("info"),
                QStringLiteral("Discovered %1 portable application(s)").arg(m_apps.size())
            );
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

bool PortableAppsManager::addToMenu(const QString &path, const QString &customName)
{
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isExecutable()) {
        emit operationFinished(false, QStringLiteral("Executable file not found."));
        return false;
    }

    const QString appName = customName.trimmed().isEmpty() ? fi.baseName() : customName.trimmed();
    const QString desktopDir = QDir::homePath() + QStringLiteral("/.local/share/applications");
    QDir().mkpath(desktopDir);

    QString cleanFileName = fi.baseName().toLower();
    cleanFileName.remove(QRegularExpression(QStringLiteral("[^a-z0-9_-]")));
    if (cleanFileName.isEmpty()) cleanFileName = QStringLiteral("app");

    const QString desktopPath = QStringLiteral("%1/klarity_portable_%2.desktop").arg(desktopDir, cleanFileName);

    QFile file(desktopPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit operationFinished(false, QStringLiteral("Failed to create desktop entry in %1").arg(desktopPath));
        return false;
    }

    QTextStream out(&file);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=" << appName << "\n";
    out << "Exec=\"" << fi.absoluteFilePath() << "\"\n";
    out << "Icon=" << (fi.suffix().toLower() == QStringLiteral("appimage") ? "application-x-executable" : "system-run") << "\n";
    out << "Terminal=false\n";
    out << "Categories=Utility;Application;\n";
    out << "Comment=Portable application added via Klarity\n";
    out << "X-Klarity-Portable=true\n";
    file.close();

    QProcess::execute(QStringLiteral("update-desktop-database"), {desktopDir});

    LogModel::instance()->append(
        QStringLiteral("PortableApps"),
        QStringLiteral("info"),
        QStringLiteral("Added '%1' to KDE Application Menu (%2)").arg(appName, desktopPath)
    );

    for (auto &app : m_apps) {
        if (app.path == path) {
            app.isInMenu = true;
            break;
        }
    }
    updateStaticCache(m_apps);
    saveToDiskCache();

    emit appsChanged();
    emit operationFinished(true, QStringLiteral("'%1' added to Application Menu.").arg(appName));
    return true;
}

bool PortableAppsManager::removeFromMenu(const QString &path)
{
    const QString desktopDir = QDir::homePath() + QStringLiteral("/.local/share/applications");
    QDir dir(desktopDir);
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files);

    bool removed = false;
    for (const auto &fi : files) {
        KDesktopFile df(fi.absoluteFilePath());
        const QString exec = df.desktopGroup().readEntry(QStringLiteral("Exec"), QString());
        if (exec.contains(path)) {
            QFile::remove(fi.absoluteFilePath());
            removed = true;
        }
    }

    if (removed) {
        QProcess::execute(QStringLiteral("update-desktop-database"), {desktopDir});
        for (auto &app : m_apps) {
            if (app.path == path) {
                app.isInMenu = false;
                break;
            }
        }
        updateStaticCache(m_apps);
        saveToDiskCache();

        emit appsChanged();
        emit operationFinished(true, QStringLiteral("Removed from Application Menu."));
        return true;
    }

    emit operationFinished(false, QStringLiteral("No associated menu entry found."));
    return false;
}

bool PortableAppsManager::deleteApp(const QString &path)
{
    removeFromMenu(path);

    if (QFile::remove(path)) {
        LogModel::instance()->append(
            QStringLiteral("PortableApps"),
            QStringLiteral("info"),
            QStringLiteral("Deleted portable application: %1").arg(path)
        );

        for (int i = 0; i < m_apps.size(); ++i) {
            if (m_apps[i].path == path) {
                m_apps.removeAt(i);
                break;
            }
        }
        updateStaticCache(m_apps);
        saveToDiskCache();

        emit appsChanged();
        emit operationFinished(true, QStringLiteral("Application deleted successfully."));
        return true;
    }

    emit operationFinished(false, QStringLiteral("Failed to delete application file."));
    return false;
}

void PortableAppsManager::launchApp(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isExecutable()) {
        emit operationFinished(false, QStringLiteral("Cannot launch: file not found or not executable."));
        return;
    }

    const bool started = QProcess::startDetached(path, {});
    if (started) {
        LogModel::instance()->append(
            QStringLiteral("PortableApps"),
            QStringLiteral("info"),
            QStringLiteral("Launched application: %1").arg(path)
        );
        emit operationFinished(true, QStringLiteral("Application launched: %1").arg(fi.fileName()));
    } else {
        emit operationFinished(false, QStringLiteral("Failed to launch application: %1").arg(path));
    }
}

bool PortableAppsManager::addManualApp(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isExecutable()) {
        emit operationFinished(false, QStringLiteral("Selected file is not an executable binary or AppImage."));
        return false;
    }

    for (const auto &app : m_apps) {
        if (app.path == fi.absoluteFilePath()) {
            emit operationFinished(false, QStringLiteral("Application already present in catalog."));
            return false;
        }
    }

    const QString ext = fi.suffix().toLower();
    const bool isAppImage = (ext == QStringLiteral("appimage"));

    PortableAppInfo item;
    item.id = QStringLiteral("portapp_%1").arg(m_apps.size());
    item.name = fi.baseName();
    item.path = fi.absoluteFilePath();
    item.size = formatSize(fi.size());
    item.sizeBytes = fi.size();
    item.isAppImage = isAppImage;
    item.icon = isAppImage ? QStringLiteral("application-x-executable") : QStringLiteral("system-run");
    item.isInMenu = checkIfInMenu(item.path);
    item.lastModified = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"));

    m_apps.append(item);
    updateStaticCache(m_apps);
    saveToDiskCache();

    emit appsChanged();
    emit operationFinished(true, QStringLiteral("Added portable app: %1").arg(item.name));
    return true;
}
