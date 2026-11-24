#include "DiskHealthManager.h"
#include "core/LogModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>
#include <QThread>
#include <KLocalizedString>

DiskHealthManager::DiskHealthManager(QObject *parent)
    : QObject(parent)
{
    checkSmartdStatus();
    scanDisks();
}

DiskHealthManager::~DiskHealthManager()
{
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

void DiskHealthManager::checkSmartdStatus()
{
    QProcess proc;
    proc.start(QStringLiteral("systemctl"), {QStringLiteral("is-active"), QStringLiteral("--quiet"), QStringLiteral("smartd")});
    proc.waitForFinished(1000);
    const bool active = (proc.exitCode() == 0);

    QProcess procEn;
    procEn.start(QStringLiteral("systemctl"), {QStringLiteral("is-enabled"), QStringLiteral("--quiet"), QStringLiteral("smartd")});
    procEn.waitForFinished(1000);
    const bool enabled = (procEn.exitCode() == 0);

    if (m_smartdActive != active || m_smartdEnabled != enabled) {
        m_smartdActive = active;
        m_smartdEnabled = enabled;
        emit smartdStatusChanged();
    }
}

void DiskHealthManager::appendLog(const QString &text)
{
    m_logOutput.append(text);
    if (m_logOutput.size() > 100000) {
        m_logOutput = m_logOutput.right(70000);
    }
    emit logOutputChanged();
}

void DiskHealthManager::clearLogs()
{
    m_logOutput.clear();
    emit logOutputChanged();
}

QString DiskHealthManager::formatBytes(qint64 bytes)
{
    if (bytes <= 0) return QStringLiteral("0 B");
    const QStringList units = {QStringLiteral("B"), QStringLiteral("KB"), QStringLiteral("MB"), QStringLiteral("GB"), QStringLiteral("TB"), QStringLiteral("PB")};
    double b = bytes;
    int i = 0;
    while (b >= 1000.0 && i < units.size() - 1) {
        b /= 1000.0;
        i++;
    }
    return QString::asprintf("%.1f %s", b, qPrintable(units[i]));
}

void DiskHealthManager::scanDisks()
{
    if (m_isScanning) return;

    m_isScanning = true;
    emit isScanningChanged();

    appendLog(QStringLiteral("============================================================\n"));
    appendLog(QStringLiteral(">>> Scanning storage devices using smartctl...\n"));
    appendLog(QStringLiteral("============================================================\n\n"));

    QThread *worker = QThread::create([this]() {
        // Step 1: Discover devices without root via smartctl --scan -j
        QProcess scanProc;
        scanProc.start(QStringLiteral("smartctl"), {QStringLiteral("--scan"), QStringLiteral("-j")});
        scanProc.waitForFinished(5000);

        QStringList discoveredDevices;
        QMap<QString, QString> deviceTypes;

        if (scanProc.exitCode() == 0) {
            const QJsonDocument doc = QJsonDocument::fromJson(scanProc.readAllStandardOutput());
            if (doc.isObject()) {
                const QJsonArray devArray = doc.object().value(QStringLiteral("devices")).toArray();
                for (const QJsonValue &v : devArray) {
                    const QJsonObject o = v.toObject();
                    const QString name = o.value(QStringLiteral("name")).toString();
                    const QString type = o.value(QStringLiteral("type")).toString();
                    if (!name.isEmpty() && !discoveredDevices.contains(name)) {
                        discoveredDevices.append(name);
                        deviceTypes[name] = type;
                    }
                }
            }
        }

        // Fallback: check /sys/block if smartctl returned nothing
        if (discoveredDevices.isEmpty()) {
            QDir blockDir(QStringLiteral("/sys/block"));
            const QStringList blocks = blockDir.entryList({QStringLiteral("sd*"), QStringLiteral("nvme*")}, QDir::Dirs);
            for (const QString &b : blocks) {
                if (!b.contains(QStringLiteral("boot")) && !b.contains(QLatin1Char('p'))) {
                    const QString devPath = QStringLiteral("/dev/") + b;
                    discoveredDevices.append(devPath);
                    deviceTypes[devPath] = b.startsWith(QStringLiteral("nvme")) ? QStringLiteral("nvme") : QStringLiteral("ata");
                }
            }
        }

        // Step 2: Query SMART data for each disk
        QVariantList resultDisks;
        bool polkitCancelled = false;

        for (const QString &dev : discoveredDevices) {
            QProcess smartProc;
            // First try without pkexec, if failed with permission denied try with pkexec
            smartProc.start(QStringLiteral("smartctl"), {QStringLiteral("-a"), QStringLiteral("-j"), dev});
            smartProc.waitForFinished(3000);

            QByteArray outData = smartProc.readAllStandardOutput();
            bool hasValidJson = false;
            QJsonDocument testDoc = QJsonDocument::fromJson(outData);
            if (testDoc.isObject() && testDoc.object().contains(QStringLiteral("smart_status"))) {
                hasValidJson = true;
            }

            if (!hasValidJson && !polkitCancelled) {
                // Try via pkexec
                QProcess pkProc;
                pkProc.start(QStringLiteral("pkexec"), {QStringLiteral("smartctl"), QStringLiteral("-a"), QStringLiteral("-j"), dev});
                pkProc.waitForFinished(10000);

                if (pkProc.exitCode() == 126 || pkProc.exitCode() == 127) {
                    polkitCancelled = true;
                } else {
                    outData = pkProc.readAllStandardOutput();
                }
            }

            QVariantMap diskMap = parseSmartctlJson(outData, dev, deviceTypes.value(dev));
            resultDisks.append(diskMap);
        }

        QMetaObject::invokeMethod(this, [this, resultDisks, polkitCancelled]() {
            m_disks = resultDisks;
            m_isScanning = false;
            emit disksChanged();
            emit isScanningChanged();

            checkSmartdStatus();

            if (polkitCancelled) {
                const QString msg = i18n("Root privileges via Polkit are required for detailed SMART attributes.");
                appendLog(QStringLiteral(">>> ") + msg + QStringLiteral("\n"));
                emit operationFinished(false, msg);
            } else {
                appendLog(i18n(">>> Scan completed. Found %1 disk(s).\n", m_disks.size()));
            }

            // Auto-select first disk if none selected
            if (!m_disks.isEmpty()) {
                if (m_selectedDevice.isEmpty() || !m_selectedDiskDetail.contains(QStringLiteral("device"))) {
                    selectDisk(m_disks.first().toMap().value(QStringLiteral("device")).toString());
                }
            }
        }, Qt::QueuedConnection);
    });

    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

QVariantMap DiskHealthManager::parseSmartctlJson(const QByteArray &jsonData, const QString &device, const QString &typeHint)
{
    QVariantMap disk;
    disk[QStringLiteral("device")] = device;
    disk[QStringLiteral("type")] = typeHint.toUpper();
    disk[QStringLiteral("isNvme")] = (typeHint.toLower() == QStringLiteral("nvme") || device.contains(QStringLiteral("nvme")));
    disk[QStringLiteral("model")] = QFileInfo(device).fileName();
    disk[QStringLiteral("serial")] = QStringLiteral("Unknown");
    disk[QStringLiteral("firmware")] = QStringLiteral("N/A");
    disk[QStringLiteral("size")] = QStringLiteral("Unknown");
    disk[QStringLiteral("temperature")] = QStringLiteral("N/A");
    disk[QStringLiteral("temperatureValue")] = 0;
    disk[QStringLiteral("healthStatus")] = QStringLiteral("UNKNOWN");
    disk[QStringLiteral("healthColor")] = QStringLiteral("#888888");
    disk[QStringLiteral("loaded")] = false;
    disk[QStringLiteral("powerOnHours")] = 0;
    disk[QStringLiteral("powerCycles")] = 0;
    disk[QStringLiteral("unsafeShutdowns")] = 0;
    disk[QStringLiteral("wearIndicator")] = QStringLiteral("N/A");
    disk[QStringLiteral("mediaErrors")] = 0;
    disk[QStringLiteral("criticalWarning")] = 0;
    disk[QStringLiteral("selfTestStatus")] = i18n("Idle");
    disk[QStringLiteral("selfTestPercentRemaining")] = 0;
    disk[QStringLiteral("selfTestInProgress")] = false;

    QVariantList attributes;

    const QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        disk[QStringLiteral("loaded")] = true;

        // Model & Device Identification
        if (root.contains(QStringLiteral("model_name"))) {
            disk[QStringLiteral("model")] = root.value(QStringLiteral("model_name")).toString();
        }
        if (root.contains(QStringLiteral("serial_number"))) {
            disk[QStringLiteral("serial")] = root.value(QStringLiteral("serial_number")).toString();
        }
        if (root.contains(QStringLiteral("firmware_version"))) {
            disk[QStringLiteral("firmware")] = root.value(QStringLiteral("firmware_version")).toString();
        }

        // Capacity
        if (root.contains(QStringLiteral("user_capacity"))) {
            const qint64 bytes = root.value(QStringLiteral("user_capacity")).toObject().value(QStringLiteral("bytes")).toVariant().toLongLong();
            if (bytes > 0) {
                disk[QStringLiteral("size")] = formatBytes(bytes);
            }
        }

        // Overall Health Status
        bool passed = false;
        if (root.contains(QStringLiteral("smart_status"))) {
            passed = root.value(QStringLiteral("smart_status")).toObject().value(QStringLiteral("passed")).toBool();
        }

        // Temperature
        int tempVal = 0;
        if (root.contains(QStringLiteral("temperature"))) {
            tempVal = root.value(QStringLiteral("temperature")).toObject().value(QStringLiteral("current")).toInt();
        }
        if (tempVal > 0) {
            disk[QStringLiteral("temperature")] = QStringLiteral("%1 °C").arg(tempVal);
            disk[QStringLiteral("temperatureValue")] = tempVal;
        }

        // 1. Process NVMe Data
        if (root.contains(QStringLiteral("nvme_smart_health_information_log"))) {
            disk[QStringLiteral("isNvme")] = true;
            disk[QStringLiteral("type")] = QStringLiteral("NVME");
            const QJsonObject nvme = root.value(QStringLiteral("nvme_smart_health_information_log")).toObject();

            const int critWarn = nvme.value(QStringLiteral("critical_warning")).toInt();
            disk[QStringLiteral("criticalWarning")] = critWarn;
            const int percentUsed = nvme.value(QStringLiteral("percentage_used")).toInt();
            disk[QStringLiteral("wearIndicator")] = QStringLiteral("%1%").arg(percentUsed);
            const int availSpare = nvme.value(QStringLiteral("available_spare")).toInt();
            const int spareThresh = nvme.value(QStringLiteral("available_spare_threshold")).toInt();
            const int powerHours = nvme.value(QStringLiteral("power_on_hours")).toInt();
            disk[QStringLiteral("powerOnHours")] = powerHours;
            const int powerCycles = nvme.value(QStringLiteral("power_cycles")).toInt();
            disk[QStringLiteral("powerCycles")] = powerCycles;
            const int unsafeShut = nvme.value(QStringLiteral("unsafe_shutdowns")).toInt();
            disk[QStringLiteral("unsafeShutdowns")] = unsafeShut;
            const int mediaErr = nvme.value(QStringLiteral("media_errors")).toInt();
            disk[QStringLiteral("mediaErrors")] = mediaErr;

            if (tempVal == 0 && nvme.contains(QStringLiteral("temperature"))) {
                tempVal = nvme.value(QStringLiteral("temperature")).toInt();
                disk[QStringLiteral("temperature")] = QStringLiteral("%1 °C").arg(tempVal);
                disk[QStringLiteral("temperatureValue")] = tempVal;
            }

            // Build NVMe attribute table
            auto addAttr = [&attributes](const QString &id, const QString &name, const QString &raw, bool isCrit, bool isWarn) {
                QVariantMap a;
                a[QStringLiteral("id")] = id;
                a[QStringLiteral("name")] = name;
                a[QStringLiteral("value")] = raw;
                a[QStringLiteral("worst")] = QStringLiteral("-");
                a[QStringLiteral("threshold")] = QStringLiteral("-");
                a[QStringLiteral("raw")] = raw;
                a[QStringLiteral("isCritical")] = isCrit;
                a[QStringLiteral("isWarning")] = isWarn;
                attributes.append(a);
            };

            addAttr(QStringLiteral("0x01"), i18n("Critical Warning"), QString::number(critWarn), true, critWarn > 0);
            addAttr(QStringLiteral("0x02"), i18n("Composite Temperature"), QStringLiteral("%1 °C").arg(tempVal), false, tempVal > 70);
            addAttr(QStringLiteral("0x03"), i18n("Available Spare"), QStringLiteral("%1% (threshold: %2%)").arg(availSpare).arg(spareThresh), true, availSpare < spareThresh);
            addAttr(QStringLiteral("0x04"), i18n("Percentage Used (Wear)"), QStringLiteral("%1%").arg(percentUsed), true, percentUsed > 90);
            addAttr(QStringLiteral("0x05"), i18n("Media and Data Integrity Errors"), QString::number(mediaErr), true, mediaErr > 0);
            addAttr(QStringLiteral("0x06"), i18n("Power On Hours"), i18n("%1 hours", powerHours), false, false);
            addAttr(QStringLiteral("0x07"), i18n("Power Cycles"), QString::number(powerCycles), false, false);
            addAttr(QStringLiteral("0x08"), i18n("Unsafe Shutdowns"), QString::number(unsafeShut), false, unsafeShut > 100);

            if (critWarn > 0 || mediaErr > 0 || !passed) {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("FAILED");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#ff5555");
            } else if (availSpare < spareThresh || percentUsed > 80 || tempVal > 68) {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("WARNING");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#ffcc00");
            } else {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("PASSED");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#5af78e");
            }
        }
        // 2. Process SATA / ATA Data
        else if (root.contains(QStringLiteral("ata_smart_attributes"))) {
            disk[QStringLiteral("isNvme")] = false;
            if (disk[QStringLiteral("type")].toString() == QStringLiteral("NVME")) {
                disk[QStringLiteral("type")] = QStringLiteral("SATA");
            }
            const QJsonArray tbl = root.value(QStringLiteral("ata_smart_attributes")).toObject().value(QStringLiteral("table")).toArray();

            bool hasWarning = false;
            bool hasCriticalFailure = !passed;

            for (const QJsonValue &av : tbl) {
                const QJsonObject ao = av.toObject();
                const int id = ao.value(QStringLiteral("id")).toInt();
                const QString name = ao.value(QStringLiteral("name")).toString();
                const int val = ao.value(QStringLiteral("value")).toInt();
                const int worst = ao.value(QStringLiteral("worst")).toInt();
                const int thresh = ao.value(QStringLiteral("thresh")).toInt();
                const qint64 rawVal = ao.value(QStringLiteral("raw")).toObject().value(QStringLiteral("value")).toVariant().toLongLong();

                const bool isCrit = (id == 5 || id == 197 || id == 198 || id == 177 || id == 231 || id == 233);
                bool isWarn = false;

                if (id == 5 && rawVal > 0) { // Reallocated_Sector_Ct
                    isWarn = true;
                    hasWarning = true;
                } else if (id == 197 && rawVal > 0) { // Current_Pending_Sector
                    isWarn = true;
                    hasWarning = true;
                } else if (id == 198 && rawVal > 0) { // Offline_Uncorrectable
                    isWarn = true;
                    hasCriticalFailure = true;
                } else if ((id == 177 || id == 231 || id == 233) && val < 20) { // SSD Wear Level
                    isWarn = true;
                    hasWarning = true;
                }

                if (id == 9) { // Power_On_Hours
                    disk[QStringLiteral("powerOnHours")] = rawVal;
                } else if (id == 12) { // Power_Cycle_Count
                    disk[QStringLiteral("powerCycles")] = rawVal;
                } else if (id == 194 && tempVal == 0) { // Temperature_Celsius
                    tempVal = rawVal;
                    disk[QStringLiteral("temperature")] = QStringLiteral("%1 °C").arg(tempVal);
                    disk[QStringLiteral("temperatureValue")] = tempVal;
                } else if (id == 177 || id == 231 || id == 233) {
                    disk[QStringLiteral("wearIndicator")] = QStringLiteral("%1%").arg(100 - val);
                }

                QVariantMap a;
                a[QStringLiteral("id")] = QString::number(id);
                a[QStringLiteral("name")] = name;
                a[QStringLiteral("value")] = QString::number(val);
                a[QStringLiteral("worst")] = QString::number(worst);
                a[QStringLiteral("threshold")] = QString::number(thresh);
                a[QStringLiteral("raw")] = QString::number(rawVal);
                a[QStringLiteral("isCritical")] = isCrit;
                a[QStringLiteral("isWarning")] = isWarn;
                attributes.append(a);
            }

            if (hasCriticalFailure) {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("FAILED");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#ff5555");
            } else if (hasWarning) {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("WARNING");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#ffcc00");
            } else {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("PASSED");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#5af78e");
            }
        } else {
            // Basic passed / failed
            if (passed) {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("PASSED");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#5af78e");
            } else {
                disk[QStringLiteral("healthStatus")] = QStringLiteral("FAILED");
                disk[QStringLiteral("healthColor")] = QStringLiteral("#ff5555");
            }
        }

        // Self-test execution status
        if (root.contains(QStringLiteral("ata_smart_data"))) {
            const QJsonObject selfTest = root.value(QStringLiteral("ata_smart_data")).toObject().value(QStringLiteral("self_test")).toObject();
            const QJsonObject stStatus = selfTest.value(QStringLiteral("status")).toObject();
            const bool inProgress = stStatus.value(QStringLiteral("passed")).toBool() == false && stStatus.value(QStringLiteral("string")).toString().contains(QStringLiteral("remaining"));
            disk[QStringLiteral("selfTestInProgress")] = inProgress;
            disk[QStringLiteral("selfTestStatus")] = stStatus.value(QStringLiteral("string")).toString();
            disk[QStringLiteral("selfTestPercentRemaining")] = stStatus.value(QStringLiteral("percent_remaining")).toInt();
        }
    }

    disk[QStringLiteral("attributes")] = attributes;
    return disk;
}

void DiskHealthManager::selectDisk(const QString &device)
{
    m_selectedDevice = device;
    emit selectedDeviceChanged();

    for (const auto &v : m_disks) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("device")).toString() == device) {
            m_selectedDiskDetail = m;
            emit selectedDiskDetailChanged();
            break;
        }
    }
}

void DiskHealthManager::refreshDisk(const QString &device)
{
    if (device.isEmpty()) return;

    appendLog(QStringLiteral(">>> Refreshing SMART attributes for %1...\n").arg(device));

    QThread *worker = QThread::create([this, device]() {
        QProcess proc;
        proc.start(QStringLiteral("pkexec"), {QStringLiteral("smartctl"), QStringLiteral("-a"), QStringLiteral("-j"), device});
        proc.waitForFinished(10000);

        const QByteArray out = proc.readAllStandardOutput();
        const QVariantMap updatedMap = parseSmartctlJson(out, device, device.contains(QStringLiteral("nvme")) ? QStringLiteral("nvme") : QStringLiteral("sata"));

        QMetaObject::invokeMethod(this, [this, device, updatedMap]() {
            // Update in m_disks
            for (int i = 0; i < m_disks.size(); ++i) {
                if (m_disks[i].toMap().value(QStringLiteral("device")).toString() == device) {
                    m_disks[i] = updatedMap;
                    emit disksChanged();
                    break;
                }
            }
            if (m_selectedDevice == device) {
                m_selectedDiskDetail = updatedMap;
                emit selectedDiskDetailChanged();
            }
            appendLog(i18n(">>> SMART data refreshed for %1.\n", device));
        }, Qt::QueuedConnection);
    });

    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void DiskHealthManager::runSelfTest(const QString &device, const QString &type)
{
    if (device.isEmpty() || m_isBusy) return;

    m_isBusy = true;
    m_operationStatus = i18n("Running %1 S.M.A.R.T. self-test on %2", type, device);
    emit isBusyChanged();
    emit operationStatusChanged();

    appendLog(QStringLiteral("============================================================\n"));
    appendLog(QStringLiteral(">>> Starting self-test: pkexec smartctl -t %1 %2\n").arg(type, device));
    appendLog(QStringLiteral("============================================================\n\n"));

    if (m_process) delete m_process;
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        if (m_process) appendLog(QString::fromUtf8(m_process->readAllStandardOutput()));
    });

    connect(m_process, &QProcess::finished, this, [this, device](int exitCode) {
        m_isBusy = false;
        emit isBusyChanged();

        const bool success = (exitCode == 0);
        const QString res = success
            ? i18n("S.M.A.R.T. self-test routine initiated on %1.", device)
            : i18n("Failed to launch self-test on %1 (exit code: %2).", device, exitCode);

        appendLog(QStringLiteral("\n>>> ") + res + QStringLiteral("\n"));
        emit operationFinished(success, res);

        refreshDisk(device);
    });

    m_process->start(QStringLiteral("pkexec"), {QStringLiteral("smartctl"), QStringLiteral("-t"), type, device});
}

void DiskHealthManager::abortSelfTest(const QString &device)
{
    if (device.isEmpty()) return;

    appendLog(QStringLiteral(">>> Aborting self-test on %1...\n").arg(device));

    QProcess *abortProc = new QProcess(this);
    connect(abortProc, &QProcess::finished, this, [this, device, abortProc](int exitCode) {
        const bool success = (exitCode == 0);
        const QString res = success
            ? i18n("Self-test aborted on %1.", device)
            : i18n("Failed to abort self-test on %1.", device);
        appendLog(QStringLiteral(">>> ") + res + QStringLiteral("\n"));
        emit operationFinished(success, res);
        refreshDisk(device);
        abortProc->deleteLater();
    });

    abortProc->start(QStringLiteral("pkexec"), {QStringLiteral("smartctl"), QStringLiteral("-X"), device});
}

void DiskHealthManager::toggleSmartd(bool enable)
{
    const QString action = enable ? QStringLiteral("enable --now") : QStringLiteral("disable --now");
    appendLog(QStringLiteral(">>> Executing: pkexec systemctl %1 smartd\n").arg(action));

    QProcess *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, enable, p](int exitCode) {
        const bool success = (exitCode == 0);
        const QString res = success
            ? (enable ? i18n("smartd background monitoring enabled and started.") : i18n("smartd background monitoring disabled."))
            : i18n("Failed to change smartd service status (exit code %1).", exitCode);

        appendLog(QStringLiteral(">>> ") + res + QStringLiteral("\n"));
        emit operationFinished(success, res);

        checkSmartdStatus();
        p->deleteLater();
    });

    p->start(QStringLiteral("pkexec"), {QStringLiteral("systemctl"), enable ? QStringLiteral("enable") : QStringLiteral("disable"), QStringLiteral("--now"), QStringLiteral("smartd")});
}
