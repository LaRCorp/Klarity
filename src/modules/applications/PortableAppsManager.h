#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct PortableAppInfo
{
    QString id;
    QString name;
    QString path;
    QString size;
    qint64 sizeBytes = 0;
    bool isAppImage = false;
    QString icon;
    bool isInMenu = false;
    QString lastModified;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("path"), path},
            {QStringLiteral("size"), size},
            {QStringLiteral("sizeBytes"), sizeBytes},
            {QStringLiteral("isAppImage"), isAppImage},
            {QStringLiteral("icon"), icon},
            {QStringLiteral("isInMenu"), isInMenu},
            {QStringLiteral("lastModified"), lastModified}
        };
    }
};

class PortableAppsManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(int count READ count NOTIFY appsChanged)

public:
    explicit PortableAppsManager(QObject *parent = nullptr);
    ~PortableAppsManager() override = default;

    QVariantList apps() const;
    bool isScanning() const { return m_isScanning; }
    int count() const { return m_apps.size(); }

    Q_INVOKABLE void scanPortableApps(bool force = false);
    Q_INVOKABLE bool addToMenu(const QString &path, const QString &customName = QString());
    Q_INVOKABLE bool removeFromMenu(const QString &path);
    Q_INVOKABLE bool deleteApp(const QString &path);
    Q_INVOKABLE void launchApp(const QString &path);
    Q_INVOKABLE bool addManualApp(const QString &path);

    void scanPortableAppsSync();

signals:
    void appsChanged();
    void isScanningChanged();
    void operationFinished(bool success, const QString &message);

private:
    bool loadFromDiskCache();
    void saveToDiskCache() const;

    static bool checkIfInMenu(const QString &execPath);
    static QString formatSize(qint64 bytes);
    static QVector<PortableAppInfo> doScan();

    QVector<PortableAppInfo> m_apps;
    bool m_isScanning = false;

    // Static in-memory cache
    static bool s_hasStaticCache;
    static QVector<PortableAppInfo> s_staticApps;
    static void updateStaticCache(const QVector<PortableAppInfo> &apps);
};
