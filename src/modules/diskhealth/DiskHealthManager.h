#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QProcess>

class DiskHealthManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString operationStatus READ operationStatus NOTIFY operationStatusChanged)
    Q_PROPERTY(QVariantList disks READ disks NOTIFY disksChanged)
    Q_PROPERTY(QVariantMap selectedDiskDetail READ selectedDiskDetail NOTIFY selectedDiskDetailChanged)
    Q_PROPERTY(QString selectedDevice READ selectedDevice NOTIFY selectedDeviceChanged)
    Q_PROPERTY(bool smartdActive READ smartdActive NOTIFY smartdStatusChanged)
    Q_PROPERTY(bool smartdEnabled READ smartdEnabled NOTIFY smartdStatusChanged)
    Q_PROPERTY(QString logOutput READ logOutput NOTIFY logOutputChanged)

public:
    explicit DiskHealthManager(QObject *parent = nullptr);
    ~DiskHealthManager() override;

    bool isScanning() const { return m_isScanning; }
    bool isBusy() const { return m_isBusy; }
    QString operationStatus() const { return m_operationStatus; }
    QVariantList disks() const { return m_disks; }
    QVariantMap selectedDiskDetail() const { return m_selectedDiskDetail; }
    QString selectedDevice() const { return m_selectedDevice; }
    bool smartdActive() const { return m_smartdActive; }
    bool smartdEnabled() const { return m_smartdEnabled; }
    QString logOutput() const { return m_logOutput; }

    Q_INVOKABLE void scanDisks();
    Q_INVOKABLE void selectDisk(const QString &device);
    Q_INVOKABLE void refreshDisk(const QString &device);
    Q_INVOKABLE void runSelfTest(const QString &device, const QString &type);
    Q_INVOKABLE void abortSelfTest(const QString &device);
    Q_INVOKABLE void toggleSmartd(bool enable);
    Q_INVOKABLE void clearLogs();

signals:
    void isScanningChanged();
    void isBusyChanged();
    void operationStatusChanged();
    void disksChanged();
    void selectedDiskDetailChanged();
    void selectedDeviceChanged();
    void smartdStatusChanged();
    void logOutputChanged();
    void operationFinished(bool success, const QString &message);

public slots:
    void checkSmartdStatus();

private:
    void appendLog(const QString &text);
    QVariantMap parseSmartctlJson(const QByteArray &jsonData, const QString &device, const QString &typeHint);
    static QString formatBytes(qint64 bytes);

    QProcess *m_process = nullptr;
    bool m_isScanning = false;
    bool m_isBusy = false;
    QString m_operationStatus;
    QVariantList m_disks;
    QVariantMap m_selectedDiskDetail;
    QString m_selectedDevice;
    bool m_smartdActive = false;
    bool m_smartdEnabled = false;
    QString m_logOutput;
};
