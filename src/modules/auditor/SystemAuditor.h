#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct AuditItem
{
    QString id;
    QString category;      // "symlinks", "orphans", "cache", "configs", "services"
    QString title;
    QString subtitle;
    QString path;
    qint64 sizeBytes = 0;
    QString formattedSize;
    QString riskLevel;     // "safe", "review"
    bool selected = false;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("category"), category},
            {QStringLiteral("title"), title},
            {QStringLiteral("subtitle"), subtitle},
            {QStringLiteral("path"), path},
            {QStringLiteral("sizeBytes"), sizeBytes},
            {QStringLiteral("formattedSize"), formattedSize},
            {QStringLiteral("riskLevel"), riskLevel},
            {QStringLiteral("selected"), selected}
        };
    }
};

class SystemAuditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList items READ items NOTIFY itemsChanged)
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(bool isCleaning READ isCleaning NOTIFY isCleaningChanged)
    Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)
    Q_PROPERTY(QString scanStatusText READ scanStatusText NOTIFY scanStatusTextChanged)
    Q_PROPERTY(qint64 totalReclaimableBytes READ totalReclaimableBytes NOTIFY itemsChanged)
    Q_PROPERTY(QString totalReclaimableFormatted READ totalReclaimableFormatted NOTIFY itemsChanged)
    Q_PROPERTY(int safeCount READ safeCount NOTIFY itemsChanged)
    Q_PROPERTY(int reviewCount READ reviewCount NOTIFY itemsChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY itemsChanged)

public:
    explicit SystemAuditor(QObject *parent = nullptr);
    ~SystemAuditor() override = default;

    QVariantList items() const;
    bool isScanning() const { return m_isScanning; }
    bool isCleaning() const { return m_isCleaning; }
    int scanProgress() const { return m_scanProgress; }
    QString scanStatusText() const { return m_scanStatusText; }
    qint64 totalReclaimableBytes() const { return m_totalReclaimableBytes; }
    QString totalReclaimableFormatted() const;
    int safeCount() const { return m_safeCount; }
    int reviewCount() const { return m_reviewCount; }
    int selectedCount() const;

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void setItemSelected(const QString &id, bool selected);
    Q_INVOKABLE void selectAllSafe();
    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE void cleanSelected();
    Q_INVOKABLE bool isWhitelisted(const QString &name) const;

signals:
    void itemsChanged();
    void isScanningChanged();
    void isCleaningChanged();
    void scanProgressChanged();
    void scanStatusTextChanged();
    void operationFinished(bool success, const QString &message);

private:
    void scanBrokenSymlinks();
    void scanOrphanedPackages();
    void scanPackageCache();
    void scanOrphanedConfigs();
    void scanForgottenServices();
    void recalculateTotals();
    static QString formatSize(qint64 bytes);

    QVector<AuditItem> m_items;
    bool m_isScanning = false;
    bool m_isCleaning = false;
    int m_scanProgress = 0;
    QString m_scanStatusText;
    qint64 m_totalReclaimableBytes = 0;
    int m_safeCount = 0;
    int m_reviewCount = 0;
};
