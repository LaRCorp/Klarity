#include "PowerManager.h"
#include "core/LogModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <KLocalizedString>

PowerManager::PowerManager(QObject *parent)
    : QObject(parent)
{
    initHardwareInfo();
    loadAutoCpuFreqConfig();
    refreshStatus();

    connect(&m_monitorTimer, &QTimer::timeout, this, &PowerManager::onMonitorTick);
    m_monitorTimer.start(1000); // 1-second live telemetry updates
}

PowerManager::~PowerManager()
{
    m_monitorTimer.stop();
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(1000)) {
                m_process->kill();
            }
        }
        delete m_process;
        m_process = nullptr;
    }
}

void PowerManager::initHardwareInfo()
{
    // 1. auto-cpufreq binary check
    m_isAutoCpuFreqInstalled = !QStandardPaths::findExecutable(QStringLiteral("auto-cpufreq")).isEmpty();

    // 2. CPU Driver Detection
    QFile driverFile(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/scaling_driver"));
    if (driverFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_cpuDriver = QString::fromUtf8(driverFile.readAll()).trimmed();
        driverFile.close();
    } else {
        m_cpuDriver = QStringLiteral("generic_cpufreq");
    }

    // 3. Turbo Boost detection
    if (QFile::exists(QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"))) {
        m_turboSupported = true;
    } else if (QFile::exists(QStringLiteral("/sys/devices/system/cpu/cpufreq/boost"))) {
        m_turboSupported = true;
    } else {
        m_turboSupported = false;
    }

    // 4. Available Governors
    QFile govFile(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"));
    if (!govFile.exists()) {
        govFile.setFileName(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors"));
    }
    if (govFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString content = QString::fromUtf8(govFile.readAll()).trimmed();
        const QStringList parts = content.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        for (const QString &p : parts) {
            const QString item = p.trimmed();
            if (!item.isEmpty() && !m_availableGovernors.contains(item)) {
                m_availableGovernors.append(item);
            }
        }
        govFile.close();
    }
    if (m_availableGovernors.isEmpty()) {
        m_availableGovernors = {QStringLiteral("powersave"), QStringLiteral("performance")};
    }

    // 5. Min / Max Hardware Frequency bounds (in MHz)
    QFile minF(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq"));
    if (minF.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_minFreqMhz = minF.readAll().trimmed().toInt() / 1000;
        minF.close();
    }
    if (m_minFreqMhz <= 0) m_minFreqMhz = 400;

    QFile maxF(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq"));
    if (maxF.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_maxFreqMhz = maxF.readAll().trimmed().toInt() / 1000;
        maxF.close();
    }
    if (m_maxFreqMhz <= 0) m_maxFreqMhz = 4400;

    m_scalingMinFreqMhz = m_minFreqMhz;
    m_scalingMaxFreqMhz = m_maxFreqMhz;

    // 6. CPU Core Count
    QDir cpuDir(QStringLiteral("/sys/devices/system/cpu/cpufreq"));
    const QStringList policies = cpuDir.entryList({QStringLiteral("policy*")}, QDir::Dirs);
    if (!policies.isEmpty()) {
        m_cpuCoreCount = policies.size();
    } else {
        m_cpuCoreCount = qMax(1, QThread::idealThreadCount());
    }

    // 7. Battery Thresholds support
    const QStringList batPaths = {
        QStringLiteral("/sys/class/power_supply/BAT0/charge_control_start_threshold"),
        QStringLiteral("/sys/class/power_supply/BAT1/charge_control_start_threshold"),
        QStringLiteral("/sys/class/power_supply/BAT0/charge_start_threshold"),
        QStringLiteral("/sys/class/power_supply/BAT1/charge_start_threshold")
    };
    for (const QString &bp : batPaths) {
        if (QFile::exists(bp)) {
            m_hasBatteryThresholds = true;
            break;
        }
    }
}

void PowerManager::loadAutoCpuFreqConfig()
{
    const QString configPath = QStringLiteral("/etc/auto-cpufreq.conf");
    if (QFile::exists(configPath)) {
        QSettings settings(configPath, QSettings::IniFormat);

        settings.beginGroup(QStringLiteral("charger"));
        m_chargerGovernor = settings.value(QStringLiteral("governor"), QStringLiteral("performance")).toString();
        m_chargerTurbo = settings.value(QStringLiteral("turbo"), QStringLiteral("auto")).toString();
        settings.endGroup();

        settings.beginGroup(QStringLiteral("battery"));
        m_batteryGovernor = settings.value(QStringLiteral("governor"), QStringLiteral("powersave")).toString();
        m_batteryTurbo = settings.value(QStringLiteral("turbo"), QStringLiteral("auto")).toString();
        m_enableThresholds = settings.value(QStringLiteral("enable_thresholds"), false).toBool();
        m_startThreshold = settings.value(QStringLiteral("start_threshold"), 20).toInt();
        m_stopThreshold = settings.value(QStringLiteral("stop_threshold"), 80).toInt();
        settings.endGroup();
    } else {
        // Defaults
        m_chargerGovernor = QStringLiteral("performance");
        m_chargerTurbo = QStringLiteral("auto");
        m_batteryGovernor = QStringLiteral("powersave");
        m_batteryTurbo = QStringLiteral("auto");
        m_enableThresholds = false;
        m_startThreshold = 20;
        m_stopThreshold = 80;
    }
    emit autoConfigChanged();
}

void PowerManager::setChargerGovernor(const QString &gov)
{
    if (m_chargerGovernor != gov) {
        m_chargerGovernor = gov;
        emit autoConfigChanged();
    }
}

void PowerManager::setChargerTurbo(const QString &turbo)
{
    if (m_chargerTurbo != turbo) {
        m_chargerTurbo = turbo;
        emit autoConfigChanged();
    }
}

void PowerManager::setBatteryGovernor(const QString &gov)
{
    if (m_batteryGovernor != gov) {
        m_batteryGovernor = gov;
        emit autoConfigChanged();
    }
}

void PowerManager::setBatteryTurbo(const QString &turbo)
{
    if (m_batteryTurbo != turbo) {
        m_batteryTurbo = turbo;
        emit autoConfigChanged();
    }
}

void PowerManager::setEnableThresholds(bool enabled)
{
    if (m_enableThresholds != enabled) {
        m_enableThresholds = enabled;
        emit autoConfigChanged();
    }
}

void PowerManager::setStartThreshold(int val)
{
    if (m_startThreshold != val) {
        m_startThreshold = val;
        emit autoConfigChanged();
    }
}

void PowerManager::setStopThreshold(int val)
{
    if (m_stopThreshold != val) {
        m_stopThreshold = val;
        emit autoConfigChanged();
    }
}

void PowerManager::startMonitoring()
{
    if (!m_monitorTimer.isActive()) {
        m_monitorTimer.start(1000);
    }
}

void PowerManager::stopMonitoring()
{
    m_monitorTimer.stop();
}

void PowerManager::refreshStatus()
{
    onMonitorTick();
}

void PowerManager::onMonitorTick()
{
    // 1. Check auto-cpufreq service status
    const bool wasRunning = m_isAutoCpuFreqRunning;
    QProcess proc;
    proc.start(QStringLiteral("systemctl"), {QStringLiteral("is-active"), QStringLiteral("--quiet"), QStringLiteral("auto-cpufreq")});
    proc.waitForFinished(1000);
    m_isAutoCpuFreqRunning = (proc.exitCode() == 0);
    if (wasRunning != m_isAutoCpuFreqRunning) {
        emit autoCpuFreqRunningChanged();
    }

    // 2. Read CPU Package Temperature
    double detectedTemp = 0.0;
    QDir thermalDir(QStringLiteral("/sys/class/thermal"));
    const QStringList zones = thermalDir.entryList({QStringLiteral("thermal_zone*")}, QDir::Dirs);
    for (const QString &zone : zones) {
        QFile typeFile(QStringLiteral("/sys/class/thermal/") + zone + QStringLiteral("/type"));
        if (typeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString typeStr = QString::fromUtf8(typeFile.readAll()).trimmed().toLower();
            typeFile.close();
            if (typeStr.contains(QStringLiteral("pkg")) || typeStr.contains(QStringLiteral("x86")) ||
                typeStr.contains(QStringLiteral("tcpu")) || typeStr.contains(QStringLiteral("coretemp")) ||
                typeStr.contains(QStringLiteral("k10temp")) || typeStr.contains(QStringLiteral("acpitz"))) {
                QFile tempFile(QStringLiteral("/sys/class/thermal/") + zone + QStringLiteral("/temp"));
                if (tempFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    const int rawTemp = tempFile.readAll().trimmed().toInt();
                    if (rawTemp > 0) {
                        detectedTemp = rawTemp / 1000.0;
                        tempFile.close();
                        break;
                    }
                    tempFile.close();
                }
            }
        }
    }
    if (detectedTemp > 0.0) {
        m_cpuTemperature = detectedTemp;
    }

    // 3. Read Governor & Frequencies
    QFile govFile(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor"));
    if (govFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_currentGovernor = QString::fromUtf8(govFile.readAll()).trimmed();
        govFile.close();
    }

    QFile minS(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq"));
    if (minS.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_scalingMinFreqMhz = minS.readAll().trimmed().toInt() / 1000;
        minS.close();
    }
    QFile maxS(QStringLiteral("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq"));
    if (maxS.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_scalingMaxFreqMhz = maxS.readAll().trimmed().toInt() / 1000;
        maxS.close();
    }

    // Per-core frequency polling
    QVariantList freqList;
    double sumFreq = 0.0;
    int coreCount = 0;
    for (int i = 0; i < m_cpuCoreCount; ++i) {
        const QString freqPath = QStringLiteral("/sys/devices/system/cpu/cpufreq/policy%1/scaling_cur_freq").arg(i);
        QFile fFile(freqPath);
        int curMhz = 0;
        if (fFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            curMhz = fFile.readAll().trimmed().toInt() / 1000;
            fFile.close();
        }
        if (curMhz > 0) {
            sumFreq += curMhz;
            coreCount++;
            freqList.append(curMhz);
        }
    }
    m_coreFrequencies = freqList;
    if (coreCount > 0) {
        m_cpuAverageFrequencyMhz = sumFreq / coreCount;
    }

    // 4. Turbo Boost Status
    if (QFile::exists(QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"))) {
        QFile nt(QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"));
        if (nt.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_isTurboEnabled = (nt.readAll().trimmed().toInt() == 0);
            nt.close();
        }
    } else if (QFile::exists(QStringLiteral("/sys/devices/system/cpu/cpufreq/boost"))) {
        QFile b(QStringLiteral("/sys/devices/system/cpu/cpufreq/boost"));
        if (b.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_isTurboEnabled = (b.readAll().trimmed().toInt() == 1);
            b.close();
        }
    }

    // 5. Battery and AC status
    QDir psDir(QStringLiteral("/sys/class/power_supply"));
    const QStringList bats = psDir.entryList({QStringLiteral("BAT*")}, QDir::Dirs);
    if (!bats.isEmpty()) {
        const QString bat = bats.first();
        QFile capFile(QStringLiteral("/sys/class/power_supply/") + bat + QStringLiteral("/capacity"));
        if (capFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_batteryLevel = capFile.readAll().trimmed().toInt();
            capFile.close();
        }

        QFile stFile(QStringLiteral("/sys/class/power_supply/") + bat + QStringLiteral("/status"));
        if (stFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_batteryStatus = QString::fromUtf8(stFile.readAll()).trimmed();
            stFile.close();
            m_onBattery = (m_batteryStatus == QStringLiteral("Discharging"));
        }
    } else {
        m_batteryLevel = 100;
        m_batteryStatus = i18n("AC Power");
        m_onBattery = false;
    }

    emit statusUpdated();
    emit turboChanged();
    emit scalingFreqsChanged();
}

void PowerManager::appendLog(const QString &text)
{
    m_logOutput.append(text);
    if (m_logOutput.size() > 100000) {
        m_logOutput = m_logOutput.right(70000);
    }
    emit logOutputChanged();
}

void PowerManager::clearLogs()
{
    m_logOutput.clear();
    emit logOutputChanged();
}

void PowerManager::runPrivilegedCommand(const QString &operationName, const QString &command)
{
    if (m_isBusy) return;

    m_isBusy = true;
    m_operationStatus = operationName;
    emit isBusyChanged();
    emit operationStatusChanged();

    appendLog(QStringLiteral("============================================================\n"));
    appendLog(QStringLiteral(">>> Starting: %1\n").arg(operationName));
    appendLog(QStringLiteral(">>> pkexec %1\n").arg(command));
    appendLog(QStringLiteral("============================================================\n\n"));

    if (m_process) {
        delete m_process;
    }
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &PowerManager::onProcessOutput);
    connect(m_process, &QProcess::finished, this, &PowerManager::onProcessFinished);

    LogModel::instance()->append(
        QStringLiteral("PowerManager"),
        QStringLiteral("info"),
        QStringLiteral("Running privileged command for '%1': %2").arg(operationName, command)
    );

    m_process->start(QStringLiteral("pkexec"), {QStringLiteral("bash"), QStringLiteral("-c"), command});
}

void PowerManager::onProcessOutput()
{
    if (!m_process) return;
    const QString chunk = QString::fromUtf8(m_process->readAllStandardOutput());
    if (!chunk.isEmpty()) {
        appendLog(chunk);
    }
}

void PowerManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    const bool success = (exitCode == 0);
    const QString opName = m_operationStatus;

    m_isBusy = false;
    emit isBusyChanged();

    const QString res = success
        ? i18n("\n>>> Operation '%1' completed SUCCESSFULLY (exit code 0).\n", opName)
        : i18n("\n>>> Operation '%1' FAILED with exit code %2.\n", opName, exitCode);
    appendLog(res);

    LogModel::instance()->append(
        QStringLiteral("PowerManager"),
        success ? QStringLiteral("info") : QStringLiteral("error"),
        QStringLiteral("Command for '%1' finished with exit code %2").arg(opName).arg(exitCode)
    );

    emit operationFinished(success, res.trimmed());

    // Recheck status
    refreshStatus();
}

void PowerManager::installAutoCpuFreq()
{
    runPrivilegedCommand(
        i18n("Install auto-cpufreq daemon"),
        QStringLiteral("pacman -S --noconfirm auto-cpufreq")
    );
}

void PowerManager::toggleAutoCpuFreq(bool enable)
{
    const QString cmd = enable
        ? QStringLiteral("systemctl enable --now auto-cpufreq")
        : QStringLiteral("systemctl disable --now auto-cpufreq");

    runPrivilegedCommand(
        enable ? i18n("Enable and Start auto-cpufreq") : i18n("Stop and Disable auto-cpufreq"),
        cmd
    );
}

void PowerManager::saveAutoCpuFreqConfig()
{
    const QString tmpPath = QDir::tempPath() + QStringLiteral("/klarity_auto_cpufreq.conf");
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        appendLog(i18n("Error: Could not write temporary configuration file.\n"));
        return;
    }

    QTextStream out(&tmpFile);
    out << "# auto-cpufreq configuration managed by Klarity\n\n";

    out << "[charger]\n";
    out << "governor = " << m_chargerGovernor << "\n";
    out << "turbo = " << m_chargerTurbo << "\n\n";

    out << "[battery]\n";
    out << "governor = " << m_batteryGovernor << "\n";
    out << "turbo = " << m_batteryTurbo << "\n";
    if (m_hasBatteryThresholds && m_enableThresholds) {
        out << "enable_thresholds = true\n";
        out << "start_threshold = " << m_startThreshold << "\n";
        out << "stop_threshold = " << m_stopThreshold << "\n";
    }
    tmpFile.close();

    const QString cmd = QStringLiteral("cp %1 /etc/auto-cpufreq.conf && systemctl restart auto-cpufreq").arg(tmpPath);
    runPrivilegedCommand(i18n("Save auto-cpufreq Profile & Restart Daemon"), cmd);
}

void PowerManager::setGovernor(const QString &governor)
{
    if (governor.isEmpty()) return;

    const QString cmd = QStringLiteral("for p in /sys/devices/system/cpu/cpufreq/policy*; do echo '%1' > \"$p/scaling_governor\"; done").arg(governor);
    runPrivilegedCommand(i18n("Apply CPU Governor (%1)", governor), cmd);
}

void PowerManager::setTurbo(bool enabled)
{
    QString cmd;
    if (QFile::exists(QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"))) {
        // Intel no_turbo: 0 is enabled, 1 is disabled
        const int val = enabled ? 0 : 1;
        cmd = QStringLiteral("echo %1 > /sys/devices/system/cpu/intel_pstate/no_turbo").arg(val);
    } else {
        const int val = enabled ? 1 : 0;
        cmd = QStringLiteral("echo %1 > /sys/devices/system/cpu/cpufreq/boost").arg(val);
    }

    runPrivilegedCommand(
        enabled ? i18n("Enable CPU Turbo Boost") : i18n("Disable CPU Turbo Boost"),
        cmd
    );
}

void PowerManager::setFrequencyLimits(int minMhz, int maxMhz)
{
    if (minMhz > maxMhz) {
        qSwap(minMhz, maxMhz);
    }
    const int minKhz = minMhz * 1000;
    const int maxKhz = maxMhz * 1000;

    const QString cmd = QStringLiteral(
        "for p in /sys/devices/system/cpu/cpufreq/policy*; do "
        "echo %1 > \"$p/scaling_min_freq\" && echo %2 > \"$p/scaling_max_freq\"; done"
    ).arg(minKhz).arg(maxKhz);

    runPrivilegedCommand(
        i18n("Set Frequency Limits (%1 - %2 MHz)", minMhz, maxMhz),
        cmd
    );
}
