#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QTimer>
#include <QProcess>

class PowerManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // Monitoring Properties
    Q_PROPERTY(bool isAutoCpuFreqInstalled READ isAutoCpuFreqInstalled NOTIFY autoCpuFreqInstalledChanged)
    Q_PROPERTY(bool isAutoCpuFreqRunning READ isAutoCpuFreqRunning NOTIFY autoCpuFreqRunningChanged)
    Q_PROPERTY(QString currentGovernor READ currentGovernor NOTIFY statusUpdated)
    Q_PROPERTY(double cpuTemperature READ cpuTemperature NOTIFY statusUpdated)
    Q_PROPERTY(double cpuAverageFrequencyMhz READ cpuAverageFrequencyMhz NOTIFY statusUpdated)
    Q_PROPERTY(int cpuCoreCount READ cpuCoreCount CONSTANT)
    Q_PROPERTY(QVariantList coreFrequencies READ coreFrequencies NOTIFY statusUpdated)
    Q_PROPERTY(QString cpuDriver READ cpuDriver CONSTANT)
    Q_PROPERTY(QStringList availableGovernors READ availableGovernors CONSTANT)
    Q_PROPERTY(bool turboSupported READ turboSupported CONSTANT)
    Q_PROPERTY(bool isTurboEnabled READ isTurboEnabled NOTIFY turboChanged)
    Q_PROPERTY(int minFreqMhz READ minFreqMhz CONSTANT)
    Q_PROPERTY(int maxFreqMhz READ maxFreqMhz CONSTANT)
    Q_PROPERTY(int scalingMinFreqMhz READ scalingMinFreqMhz NOTIFY scalingFreqsChanged)
    Q_PROPERTY(int scalingMaxFreqMhz READ scalingMaxFreqMhz NOTIFY scalingFreqsChanged)
    Q_PROPERTY(bool hasBatteryThresholds READ hasBatteryThresholds CONSTANT)
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY statusUpdated)
    Q_PROPERTY(QString batteryStatus READ batteryStatus NOTIFY statusUpdated)
    Q_PROPERTY(bool onBattery READ onBattery NOTIFY statusUpdated)

    // auto-cpufreq Profile Config Properties
    Q_PROPERTY(QString chargerGovernor READ chargerGovernor WRITE setChargerGovernor NOTIFY autoConfigChanged)
    Q_PROPERTY(QString chargerTurbo READ chargerTurbo WRITE setChargerTurbo NOTIFY autoConfigChanged)
    Q_PROPERTY(QString batteryGovernor READ batteryGovernor WRITE setBatteryGovernor NOTIFY autoConfigChanged)
    Q_PROPERTY(QString batteryTurbo READ batteryTurbo WRITE setBatteryTurbo NOTIFY autoConfigChanged)
    Q_PROPERTY(bool enableThresholds READ enableThresholds WRITE setEnableThresholds NOTIFY autoConfigChanged)
    Q_PROPERTY(int startThreshold READ startThreshold WRITE setStartThreshold NOTIFY autoConfigChanged)
    Q_PROPERTY(int stopThreshold READ stopThreshold WRITE setStopThreshold NOTIFY autoConfigChanged)

    // Process & Log Properties
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString operationStatus READ operationStatus NOTIFY operationStatusChanged)
    Q_PROPERTY(QString logOutput READ logOutput NOTIFY logOutputChanged)

public:
    explicit PowerManager(QObject *parent = nullptr);
    ~PowerManager() override;

    bool isAutoCpuFreqInstalled() const { return m_isAutoCpuFreqInstalled; }
    bool isAutoCpuFreqRunning() const { return m_isAutoCpuFreqRunning; }
    QString currentGovernor() const { return m_currentGovernor; }
    double cpuTemperature() const { return m_cpuTemperature; }
    double cpuAverageFrequencyMhz() const { return m_cpuAverageFrequencyMhz; }
    int cpuCoreCount() const { return m_cpuCoreCount; }
    QVariantList coreFrequencies() const { return m_coreFrequencies; }
    QString cpuDriver() const { return m_cpuDriver; }
    QStringList availableGovernors() const { return m_availableGovernors; }
    bool turboSupported() const { return m_turboSupported; }
    bool isTurboEnabled() const { return m_isTurboEnabled; }
    int minFreqMhz() const { return m_minFreqMhz; }
    int maxFreqMhz() const { return m_maxFreqMhz; }
    int scalingMinFreqMhz() const { return m_scalingMinFreqMhz; }
    int scalingMaxFreqMhz() const { return m_scalingMaxFreqMhz; }
    bool hasBatteryThresholds() const { return m_hasBatteryThresholds; }
    int batteryLevel() const { return m_batteryLevel; }
    QString batteryStatus() const { return m_batteryStatus; }
    bool onBattery() const { return m_onBattery; }

    QString chargerGovernor() const { return m_chargerGovernor; }
    void setChargerGovernor(const QString &gov);
    QString chargerTurbo() const { return m_chargerTurbo; }
    void setChargerTurbo(const QString &turbo);
    QString batteryGovernor() const { return m_batteryGovernor; }
    void setBatteryGovernor(const QString &gov);
    QString batteryTurbo() const { return m_batteryTurbo; }
    void setBatteryTurbo(const QString &turbo);
    bool enableThresholds() const { return m_enableThresholds; }
    void setEnableThresholds(bool enabled);
    int startThreshold() const { return m_startThreshold; }
    void setStartThreshold(int val);
    int stopThreshold() const { return m_stopThreshold; }
    void setStopThreshold(int val);

    bool isBusy() const { return m_isBusy; }
    QString operationStatus() const { return m_operationStatus; }
    QString logOutput() const { return m_logOutput; }

    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();

    Q_INVOKABLE void installAutoCpuFreq();
    Q_INVOKABLE void toggleAutoCpuFreq(bool enable);
    Q_INVOKABLE void saveAutoCpuFreqConfig();

    Q_INVOKABLE void setGovernor(const QString &governor);
    Q_INVOKABLE void setTurbo(bool enabled);
    Q_INVOKABLE void setFrequencyLimits(int minMhz, int maxMhz);
    Q_INVOKABLE void clearLogs();

signals:
    void autoCpuFreqInstalledChanged();
    void autoCpuFreqRunningChanged();
    void statusUpdated();
    void turboChanged();
    void scalingFreqsChanged();
    void autoConfigChanged();
    void isBusyChanged();
    void operationStatusChanged();
    void logOutputChanged();
    void operationFinished(bool success, const QString &message);

private slots:
    void onMonitorTick();
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initHardwareInfo();
    void loadAutoCpuFreqConfig();
    void appendLog(const QString &text);
    void runPrivilegedCommand(const QString &operationName, const QString &command);

    QTimer m_monitorTimer;
    QProcess *m_process = nullptr;

    bool m_isAutoCpuFreqInstalled = false;
    bool m_isAutoCpuFreqRunning = false;
    QString m_currentGovernor = QStringLiteral("powersave");
    double m_cpuTemperature = 0.0;
    double m_cpuAverageFrequencyMhz = 0.0;
    int m_cpuCoreCount = 1;
    QVariantList m_coreFrequencies;
    QString m_cpuDriver;
    QStringList m_availableGovernors;
    bool m_turboSupported = false;
    bool m_isTurboEnabled = true;
    int m_minFreqMhz = 400;
    int m_maxFreqMhz = 4000;
    int m_scalingMinFreqMhz = 400;
    int m_scalingMaxFreqMhz = 4000;
    bool m_hasBatteryThresholds = false;
    int m_batteryLevel = 100;
    QString m_batteryStatus = QStringLiteral("Unknown");
    bool m_onBattery = false;

    // auto-cpufreq config
    QString m_chargerGovernor = QStringLiteral("performance");
    QString m_chargerTurbo = QStringLiteral("auto");
    QString m_batteryGovernor = QStringLiteral("powersave");
    QString m_batteryTurbo = QStringLiteral("auto");
    bool m_enableThresholds = false;
    int m_startThreshold = 20;
    int m_stopThreshold = 80;

    bool m_isBusy = false;
    QString m_operationStatus;
    QString m_logOutput;
};
