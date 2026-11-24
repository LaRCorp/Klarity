#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct PackageInfo
{
    QString id;
    QString name;
    QString version;
    QString source;      // "Pacman", "AUR", "Flatpak", "Snap"
    QString size;
    qint64 sizeBytes = 0;
    QString description;
    QString icon;
    bool isGui = false;
    bool isExplicit = true;
    bool hasUpdate = false;
    QString exec;
    QString category;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("version"), version},
            {QStringLiteral("source"), source},
            {QStringLiteral("size"), size},
            {QStringLiteral("sizeBytes"), sizeBytes},
            {QStringLiteral("description"), description},
            {QStringLiteral("icon"), icon},
            {QStringLiteral("isGui"), isGui},
            {QStringLiteral("isExplicit"), isExplicit},
            {QStringLiteral("hasUpdate"), hasUpdate},
            {QStringLiteral("exec"), exec},
            {QStringLiteral("category"), category}
        };
    }
};

class PackageManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList packages READ packages NOTIFY packagesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY packagesChanged)
    Q_PROPERTY(int guiCount READ guiCount NOTIFY packagesChanged)
    Q_PROPERTY(int explicitCount READ explicitCount NOTIFY packagesChanged)
    Q_PROPERTY(int pacmanCount READ pacmanCount NOTIFY packagesChanged)
    Q_PROPERTY(int aurCount READ aurCount NOTIFY packagesChanged)
    Q_PROPERTY(int flatpakCount READ flatpakCount NOTIFY packagesChanged)
    Q_PROPERTY(int snapCount READ snapCount NOTIFY packagesChanged)
    Q_PROPERTY(int updateCount READ updateCount NOTIFY updatesChecked)

public:
    explicit PackageManager(QObject *parent = nullptr);
    ~PackageManager() override = default;

    QVariantList packages() const;
    bool isLoading() const { return m_isLoading; }
    int totalCount() const { return m_packages.size(); }
    int guiCount() const { return m_guiCount; }
    int explicitCount() const { return m_explicitCount; }
    int pacmanCount() const { return m_pacmanCount; }
    int aurCount() const { return m_aurCount; }
    int flatpakCount() const { return m_flatpakCount; }
    int snapCount() const { return m_snapCount; }
    int updateCount() const { return m_updateCount; }

    Q_INVOKABLE void loadPackages(bool forceRefresh = false);
    Q_INVOKABLE void checkUpdates();
    Q_INVOKABLE void removePackage(const QString &source, const QString &name);
    Q_INVOKABLE QVariantMap getPackageDetails(const QString &source, const QString &name);
    Q_INVOKABLE bool launchPackage(const QString &source, const QString &name);

    void loadPackagesSync();

signals:
    void packagesChanged();
    void isLoadingChanged();
    void updatesChecked();
    void operationFinished(bool success, const QString &message);

private:
    bool loadFromDiskCache();
    void saveToDiskCache() const;

    struct DesktopMeta {
        QString displayName;
        QString icon;
        QString exec;
        QString category;
        bool noDisplay = false;
    };

    static QHash<QString, DesktopMeta> collectDesktopMap();
    static QHash<QString, QString> collectPacmanGuiMap();
    static void collectPacmanAndAur(const QHash<QString, DesktopMeta> &desktopMap,
                                   const QHash<QString, QString> &guiMap,
                                   QVector<PackageInfo> &pkgs,
                                   int &pCount, int &aCount, int &gCount, int &eCount);
    static void collectFlatpak(const QHash<QString, DesktopMeta> &desktopMap, QVector<PackageInfo> &pkgs, int &fCount, int &gCount);
    static void collectSnap(QVector<PackageInfo> &pkgs, int &sCount, int &gCount);

    QVector<PackageInfo> m_packages;
    bool m_isLoading = false;
    int m_guiCount = 0;
    int m_explicitCount = 0;
    int m_pacmanCount = 0;
    int m_aurCount = 0;
    int m_flatpakCount = 0;
    int m_snapCount = 0;
    int m_updateCount = 0;

    // Static memory cache across component re-creations
    static bool s_hasStaticCache;
    static QVector<PackageInfo> s_staticPackages;
    static int s_staticGuiCount;
    static int s_staticExplicitCount;
    static int s_staticPacmanCount;
    static int s_staticAurCount;
    static int s_staticFlatpakCount;
    static int s_staticSnapCount;
    static void updateStaticCache(const QVector<PackageInfo> &pkgs, int gCount, int eCount, int pCount, int aCount, int fCount, int sCount);
};
