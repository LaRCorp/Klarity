#include "DnsManager.h"
#include "core/LogModel.h"
#include "core/ProcessRunner.h"
#include "core/PkexecRunner.h"

#include <QDesktopServices>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QThread>
#include <QUrl>
#include <KLocalizedString>

DnsManager::DnsManager(QObject *parent)
    : QObject(parent)
{
    refreshCurrentDns();
}

QVariantList DnsManager::availableProviders() const
{
    return QVariantList{
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("dhcp")},
            {QStringLiteral("name"), i18n("Automatic (DHCP / Router)")},
            {QStringLiteral("description"), i18n("Default DNS assigned automatically by your local router or DHCP network")},
            {QStringLiteral("servers"), QStringLiteral("")},
            {QStringLiteral("icon"), QStringLiteral("network-wired")},
            {QStringLiteral("isEncrypted"), false},
            {QStringLiteral("encryptionType"), QStringLiteral("Standard (Port 53)")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("cloudflare")},
            {QStringLiteral("name"), QStringLiteral("Cloudflare (1.1.1.1)")},
            {QStringLiteral("description"), i18n("Ultra-fast, privacy-oriented public DNS with DNS-over-TLS/HTTPS support")},
            {QStringLiteral("servers"), QStringLiteral("1.1.1.1 1.0.0.1")},
            {QStringLiteral("icon"), QStringLiteral("cloud")},
            {QStringLiteral("isEncrypted"), true},
            {QStringLiteral("encryptionType"), QStringLiteral("DoT / DoH")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("google")},
            {QStringLiteral("name"), QStringLiteral("Google Public DNS (8.8.8.8)")},
            {QStringLiteral("description"), i18n("Global Anycast DNS infrastructure with 8.8.8.8 and 8.8.4.4")},
            {QStringLiteral("servers"), QStringLiteral("8.8.8.8 8.8.4.4")},
            {QStringLiteral("icon"), QStringLiteral("google")},
            {QStringLiteral("isEncrypted"), true},
            {QStringLiteral("encryptionType"), QStringLiteral("DoT / DoH")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("adguard")},
            {QStringLiteral("name"), QStringLiteral("AdGuard DNS (Ad-Block)")},
            {QStringLiteral("description"), i18n("Blocks banner ads, tracking scripts, telemetry, and phishing domains system-wide")},
            {QStringLiteral("servers"), QStringLiteral("94.140.14.14 94.140.15.15")},
            {QStringLiteral("icon"), QStringLiteral("security-high")},
            {QStringLiteral("isEncrypted"), true},
            {QStringLiteral("encryptionType"), QStringLiteral("DoT / DoH")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("quad9")},
            {QStringLiteral("name"), QStringLiteral("Quad9 (Malware Blocker)")},
            {QStringLiteral("description"), i18n("Non-profit security DNS blocking malicious domains, bots, and phishing with 9.9.9.9")},
            {QStringLiteral("servers"), QStringLiteral("9.9.9.9 149.112.112.112")},
            {QStringLiteral("icon"), QStringLiteral("security-high")},
            {QStringLiteral("isEncrypted"), true},
            {QStringLiteral("encryptionType"), QStringLiteral("DoT / DoH")}
        },
        QVariantMap{
            {QStringLiteral("id"), QStringLiteral("custom")},
            {QStringLiteral("name"), i18n("Custom DNS")},
            {QStringLiteral("description"), i18n("Manually specify primary and secondary DNS server IPv4 or IPv6 addresses")},
            {QStringLiteral("servers"), QStringLiteral("")},
            {QStringLiteral("icon"), QStringLiteral("edit-entry")},
            {QStringLiteral("isEncrypted"), false},
            {QStringLiteral("encryptionType"), QStringLiteral("Custom")}
        }
    };
}

void DnsManager::refreshCurrentDns()
{
    m_isLoading = true;
    emit isLoadingChanged();

    // 1. First query active NetworkManager connections to know device and connection name
    parseNmcliStatus();

    // 2. Query systemd-resolved via resolvectl status
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    proc.setProcessEnvironment(env);
    proc.start(QStringLiteral("resolvectl"), {QStringLiteral("status")});
    if (proc.waitForFinished(2000)) {
        const QString output = QString::fromUtf8(proc.readAllStandardOutput());
        parseResolvectlStatus(output);
    }

    // 3. Fallback: if resolvectl didn't report servers, use nmcli device show
    if (m_dnsServers.isEmpty() && !m_activeInterface.isEmpty()) {
        QProcess nmProc;
        nmProc.setProcessEnvironment(env);
        nmProc.start(QStringLiteral("nmcli"), {QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("IP4.DNS,IP6.DNS"), QStringLiteral("device"), QStringLiteral("show"), m_activeInterface});
        if (nmProc.waitForFinished(2000)) {
            const QString nmOut = QString::fromUtf8(nmProc.readAllStandardOutput());
            const QStringList lines = nmOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                const int colonIdx = line.indexOf(QLatin1Char(':'));
                if (colonIdx != -1) {
                    const QString val = line.mid(colonIdx + 1).trimmed();
                    if (!val.isEmpty() && !m_dnsServers.contains(val)) {
                        m_dnsServers.append(val);
                    }
                }
            }
            if (m_activeServer.isEmpty() && !m_dnsServers.isEmpty()) {
                m_activeServer = m_dnsServers.first();
            }
        }
    }

    if (m_activeServer.isEmpty()) {
        m_activeServer = m_dnsServers.isEmpty() ? QStringLiteral("127.0.0.53") : m_dnsServers.first();
    }

    m_activeProvider = detectProvider(m_activeServer, m_dnsServers);

    m_isLoading = false;
    emit isLoadingChanged();
    emit dnsChanged();

    LogModel::instance()->append(
        QStringLiteral("DnsManager"),
        QStringLiteral("info"),
        QStringLiteral("Active DNS: %1 (%2) on %3, Encrypted: %4")
            .arg(m_activeServer, m_activeProvider, m_activeInterface, m_encryptionStatus)
    );
}

void DnsManager::parseNmcliStatus()
{
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    proc.setProcessEnvironment(env);
    proc.start(QStringLiteral("nmcli"), {QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("DEVICE,TYPE,STATE,CONNECTION"), QStringLiteral("device")});
    if (proc.waitForFinished(2000)) {
        const QString out = QString::fromUtf8(proc.readAllStandardOutput());
        const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.split(QLatin1Char(':'));
            if (parts.size() >= 4) {
                const QString dev = parts[0].trimmed();
                const QString type = parts[1].trimmed();
                const QString state = parts[2].trimmed();
                const QString conn = parts[3].trimmed();

                if (state == QStringLiteral("connected") && dev != QStringLiteral("lo")) {
                    m_activeInterface = dev;
                    if (type == QStringLiteral("wifi")) {
                        m_activeInterface += QStringLiteral(" (Wi-Fi)");
                    } else if (type == QStringLiteral("ethernet")) {
                        m_activeInterface += QStringLiteral(" (Ethernet)");
                    }
                    m_activeConnection = conn;
                    break;
                }
            }
        }
    }
}

void DnsManager::parseResolvectlStatus(const QString &output)
{
    const QStringList lines = output.split(QLatin1Char('\n'));
    bool inActiveLink = false;
    QString currentLink;
    bool hasDefaultRoute = false;

    QString parsedActiveServer;
    QStringList parsedServers;
    QString parsedProtocols;

    // Check Global DoT status first
    bool globalDoT = output.contains(QStringLiteral("+DNSOverTLS"));

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();

        if (line.startsWith(QStringLiteral("Link "))) {
            // New link block
            inActiveLink = false;
            hasDefaultRoute = false;
            static const QRegularExpression linkRe(QStringLiteral(R"(Link \d+ \(([^)]+)\))"));
            const auto match = linkRe.match(line);
            if (match.hasMatch()) {
                currentLink = match.captured(1);
            }
            continue;
        }

        if (trimmed.startsWith(QStringLiteral("Default Route: yes"))) {
            hasDefaultRoute = true;
            inActiveLink = true;
            if (m_activeInterface.isEmpty()) {
                m_activeInterface = currentLink;
            }
        }

        if (inActiveLink || hasDefaultRoute) {
            if (trimmed.startsWith(QStringLiteral("Current DNS Server:"))) {
                parsedActiveServer = trimmed.mid(QStringLiteral("Current DNS Server:").length()).trimmed();
            } else if (trimmed.startsWith(QStringLiteral("DNS Servers:"))) {
                const QString rest = trimmed.mid(QStringLiteral("DNS Servers:").length()).trimmed();
                const QStringList parts = rest.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                for (const QString &p : parts) {
                    if (!parsedServers.contains(p)) {
                        parsedServers.append(p);
                    }
                }
            } else if (trimmed.startsWith(QStringLiteral("Protocols:"))) {
                parsedProtocols = trimmed;
            }
        }
    }

    if (!parsedActiveServer.isEmpty()) {
        m_activeServer = parsedActiveServer;
    }
    if (!parsedServers.isEmpty()) {
        m_dnsServers = parsedServers;
    }

    m_isEncrypted = globalDoT || parsedProtocols.contains(QStringLiteral("+DNSOverTLS"));
    if (m_isEncrypted) {
        m_encryptionStatus = QStringLiteral("DoT (DNS-over-TLS) Active");
    } else {
        m_encryptionStatus = QStringLiteral("Standard Unencrypted (Port 53)");
    }
}

QString DnsManager::detectProvider(const QString &ip, const QStringList &allServers)
{
    const QString combined = (ip + QStringLiteral(" ") + allServers.join(QLatin1Char(' '))).toLower();

    if (combined.contains(QStringLiteral("1.1.1.1")) || combined.contains(QStringLiteral("1.0.0.1")) || combined.contains(QStringLiteral("2606:4700:"))) {
        return QStringLiteral("Cloudflare Public DNS");
    }
    if (combined.contains(QStringLiteral("8.8.8.8")) || combined.contains(QStringLiteral("8.8.4.4")) || combined.contains(QStringLiteral("2001:4860:"))) {
        return QStringLiteral("Google Public DNS");
    }
    if (combined.contains(QStringLiteral("94.140.14.")) || combined.contains(QStringLiteral("94.140.15.")) || combined.contains(QStringLiteral("2a10:50c0:"))) {
        return QStringLiteral("AdGuard DNS (Ad-Block)");
    }
    if (combined.contains(QStringLiteral("9.9.9.9")) || combined.contains(QStringLiteral("149.112.112.")) || combined.contains(QStringLiteral("2620:fe:"))) {
        return QStringLiteral("Quad9 (Security DNS)");
    }
    if (ip.startsWith(QStringLiteral("192.168.")) || ip.startsWith(QStringLiteral("10.")) || ip.startsWith(QStringLiteral("172.16.")) || ip.startsWith(QStringLiteral("fe80:"))) {
        return QStringLiteral("Local Router / DHCP");
    }
    if (ip == QStringLiteral("127.0.0.53")) {
        return QStringLiteral("systemd-resolved (Local Cache)");
    }
    return QStringLiteral("Custom / ISP DNS");
}

void DnsManager::setDnsProvider(const QString &providerId, const QString &customServers)
{
    if (m_activeConnection.isEmpty()) {
        parseNmcliStatus();
    }

    if (m_activeConnection.isEmpty()) {
        const QString err = i18n("No active network connection found to apply DNS settings.");
        LogModel::instance()->append(QStringLiteral("DnsManager"), QStringLiteral("error"), err);
        emit operationFinished(false, err);
        return;
    }

    QString ignoreAuto = QStringLiteral("yes");
    QString dnsList;

    if (providerId == QStringLiteral("dhcp") || providerId == QStringLiteral("automatic")) {
        ignoreAuto = QStringLiteral("no");
        dnsList = QStringLiteral("");
    } else if (providerId == QStringLiteral("cloudflare")) {
        dnsList = QStringLiteral("1.1.1.1 1.0.0.1");
    } else if (providerId == QStringLiteral("google")) {
        dnsList = QStringLiteral("8.8.8.8 8.8.4.4");
    } else if (providerId == QStringLiteral("adguard")) {
        dnsList = QStringLiteral("94.140.14.14 94.140.15.15");
    } else if (providerId == QStringLiteral("quad9")) {
        dnsList = QStringLiteral("9.9.9.9 149.112.112.112");
    } else if (providerId == QStringLiteral("custom")) {
        dnsList = customServers.trimmed();
        if (dnsList.isEmpty()) {
            emit operationFinished(false, i18n("Custom DNS address cannot be empty."));
            return;
        }
    }

    m_isLoading = true;
    emit isLoadingChanged();

    LogModel::instance()->append(
        QStringLiteral("DnsManager"),
        QStringLiteral("info"),
        QStringLiteral("Applying DNS provider '%1' (servers: '%2') to connection '%3'...")
            .arg(providerId, dnsList, m_activeConnection)
    );

    // Run nmcli connection modify
    QStringList modArgs{
        QStringLiteral("connection"),
        QStringLiteral("modify"),
        m_activeConnection,
        QStringLiteral("ipv4.ignore-auto-dns"),
        ignoreAuto
    };

    if (ignoreAuto == QStringLiteral("yes")) {
        modArgs << QStringLiteral("ipv4.dns") << dnsList;
    } else {
        modArgs << QStringLiteral("ipv4.dns") << QStringLiteral("");
    }

    QProcess modProc;
    modProc.start(QStringLiteral("nmcli"), modArgs);
    bool ok = modProc.waitForFinished(4000) && (modProc.exitCode() == 0);

    // If permission was denied, fall back to pkexec via ProcessRunner
    if (!ok) {
        LogModel::instance()->append(
            QStringLiteral("DnsManager"),
            QStringLiteral("warning"),
            QStringLiteral("Standard nmcli modify failed, attempting privileged execution...")
        );
        const auto pkRes = ProcessRunner::run(QStringLiteral("pkexec"), QStringList{QStringLiteral("nmcli")} + modArgs, 15000);
        ok = (pkRes.exitCode == 0);
    }

    if (ok) {
        // Reactivate connection to apply changes immediately
        QProcess upProc;
        upProc.start(QStringLiteral("nmcli"), {QStringLiteral("connection"), QStringLiteral("up"), m_activeConnection});
        upProc.waitForFinished(5000);

        // Also update resolvectl if interface is known
        QString cleanIface = m_activeInterface;
        const int spaceIdx = cleanIface.indexOf(QLatin1Char(' '));
        if (spaceIdx != -1) cleanIface = cleanIface.left(spaceIdx);

        if (!cleanIface.isEmpty()) {
            if (dnsList.isEmpty()) {
                QProcess::execute(QStringLiteral("resolvectl"), {QStringLiteral("revert"), cleanIface});
            } else {
                QStringList resolveArgs{QStringLiteral("dns"), cleanIface};
                resolveArgs.append(dnsList.split(QLatin1Char(' '), Qt::SkipEmptyParts));
                QProcess::execute(QStringLiteral("resolvectl"), resolveArgs);
            }
        }

        refreshCurrentDns();

        const QString successMsg = i18n("DNS provider '%1' successfully applied to '%2'!", providerId, m_activeConnection);
        LogModel::instance()->append(QStringLiteral("DnsManager"), QStringLiteral("info"), successMsg);
        emit operationFinished(true, successMsg);
    } else {
        m_isLoading = false;
        emit isLoadingChanged();
        const QString failMsg = i18n("Failed to configure DNS settings for connection '%1'.", m_activeConnection);
        LogModel::instance()->append(QStringLiteral("DnsManager"), QStringLiteral("error"), failMsg);
        emit operationFinished(false, failMsg);
    }
}

void DnsManager::checkDns(const QString &domain)
{
    m_isLoading = true;
    emit isLoadingChanged();

    const QString targetDomain = domain.trimmed().isEmpty() ? QStringLiteral("google.com") : domain.trimmed();

    QThread *thread = QThread::create([this, targetDomain]() {
        QElapsedTimer timer;
        timer.start();

        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
        proc.setProcessEnvironment(env);
        proc.start(QStringLiteral("dig"), {QStringLiteral("+short"), QStringLiteral("+time=2"), QStringLiteral("+tries=1"), targetDomain});

        bool success = false;
        QString resolvedIp;
        qint64 elapsedMs = 0;

        if (proc.waitForFinished(3000) && proc.exitCode() == 0) {
            elapsedMs = timer.elapsed();
            const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            if (!lines.isEmpty()) {
                resolvedIp = lines.first().trimmed();
                success = true;
            }
        }

        // Fallback to nslookup if dig is not available
        if (!success) {
            timer.restart();
            QProcess nsProc;
            nsProc.setProcessEnvironment(env);
            nsProc.start(QStringLiteral("nslookup"), {targetDomain});
            if (nsProc.waitForFinished(3000) && nsProc.exitCode() == 0) {
                elapsedMs = timer.elapsed();
                const QString out = QString::fromUtf8(nsProc.readAllStandardOutput());
                static const QRegularExpression addrRe(QStringLiteral(R"(Address:\s+([0-9a-fA-F:.]+))"));
                auto it = addrRe.globalMatch(out);
                while (it.hasNext()) {
                    auto match = it.next();
                    QString found = match.captured(1);
                    if (found != m_activeServer && !found.startsWith(QStringLiteral("127.0.0."))) {
                        resolvedIp = found;
                        success = true;
                        break;
                    }
                }
            }
        }

        QMetaObject::invokeMethod(this, [this, success, elapsedMs, targetDomain, resolvedIp]() {
            m_checkResult = QVariantMap{
                {QStringLiteral("success"), success},
                {QStringLiteral("latencyMs"), (int)elapsedMs},
                {QStringLiteral("domain"), targetDomain},
                {QStringLiteral("resolvedIp"), resolvedIp.isEmpty() ? i18n("Unreachable") : resolvedIp},
                {QStringLiteral("server"), m_activeServer}
            };
            m_isLoading = false;
            emit isLoadingChanged();
            emit checkResultChanged();

            if (success) {
                LogModel::instance()->append(
                    QStringLiteral("DnsManager"),
                    QStringLiteral("info"),
                    QStringLiteral("DNS Test OK: %1 resolved to %2 in %3 ms")
                        .arg(targetDomain, resolvedIp).arg(elapsedMs)
                );
            } else {
                LogModel::instance()->append(
                    QStringLiteral("DnsManager"),
                    QStringLiteral("error"),
                    QStringLiteral("DNS Test FAILED: Cannot resolve %1").arg(targetDomain)
                );
            }
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void DnsManager::compareProviders(const QString &domain)
{
    m_isBenchmarking = true;
    emit isBenchmarkingChanged();

    const QString targetDomain = domain.trimmed().isEmpty() ? QStringLiteral("google.com") : domain.trimmed();

    struct TargetBenchmark {
        QString id;
        QString name;
        QString ip;
        QString features;
    };

    QList<TargetBenchmark> targets{
        {QStringLiteral("cloudflare"), QStringLiteral("Cloudflare"), QStringLiteral("1.1.1.1"), QStringLiteral("DoT/DoH, 1.1.1.1")},
        {QStringLiteral("google"), QStringLiteral("Google Public DNS"), QStringLiteral("8.8.8.8"), QStringLiteral("Global Anycast, 8.8.8.8")},
        {QStringLiteral("adguard"), QStringLiteral("AdGuard DNS"), QStringLiteral("94.140.14.14"), QStringLiteral("Ad & Tracker Blocker")},
        {QStringLiteral("quad9"), QStringLiteral("Quad9 DNS"), QStringLiteral("9.9.9.9"), QStringLiteral("Malware & Phishing Protection")}
    };

    if (!m_activeServer.isEmpty() && m_activeServer != QStringLiteral("1.1.1.1") &&
        m_activeServer != QStringLiteral("8.8.8.8") && m_activeServer != QStringLiteral("94.140.14.14") &&
        m_activeServer != QStringLiteral("9.9.9.9")) {
        targets.prepend({QStringLiteral("current"), i18n("Current (%1)", m_activeServer), m_activeServer, m_activeProvider});
    }

    QThread *thread = QThread::create([this, targets, targetDomain]() {
        QVariantList results;
        int minLatency = 999999;
        int fastestIdx = -1;

        for (int i = 0; i < targets.size(); ++i) {
            const auto &tgt = targets[i];
            QProcess proc;
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
            proc.setProcessEnvironment(env);

            const QString serverArg = QStringLiteral("@") + tgt.ip;
            proc.start(QStringLiteral("dig"), {serverArg, targetDomain, QStringLiteral("+stats"), QStringLiteral("+time=2"), QStringLiteral("+tries=1")});

            int latencyMs = -1;
            if (proc.waitForFinished(2500) && proc.exitCode() == 0) {
                const QString out = QString::fromUtf8(proc.readAllStandardOutput());
                static const QRegularExpression timeRe(QStringLiteral(R"(Query time:\s+(\d+)\s+msec)"));
                const auto match = timeRe.match(out);
                if (match.hasMatch()) {
                    latencyMs = match.captured(1).toInt();
                }
            }

            if (latencyMs >= 0 && latencyMs < minLatency) {
                minLatency = latencyMs;
                fastestIdx = i;
            }

            results.append(QVariantMap{
                {QStringLiteral("id"), tgt.id},
                {QStringLiteral("name"), tgt.name},
                {QStringLiteral("ip"), tgt.ip},
                {QStringLiteral("features"), tgt.features},
                {QStringLiteral("latencyMs"), latencyMs},
                {QStringLiteral("isFastest"), false}
            });
        }

        if (fastestIdx >= 0 && fastestIdx < results.size()) {
            QVariantMap fastestMap = results[fastestIdx].toMap();
            fastestMap[QStringLiteral("isFastest")] = true;
            results[fastestIdx] = fastestMap;
        }

        QMetaObject::invokeMethod(this, [this, results]() {
            m_benchmarkResults = results;
            m_isBenchmarking = false;
            emit isBenchmarkingChanged();
            emit benchmarkResultsChanged();

            LogModel::instance()->append(
                QStringLiteral("DnsManager"),
                QStringLiteral("info"),
                QStringLiteral("DNS benchmark completed across %1 providers").arg(results.size())
            );
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void DnsManager::checkLeaks()
{
    m_isCheckingLeak = true;
    emit isCheckingLeakChanged();

    QThread *thread = QThread::create([this]() {
        QProcess proc;
        proc.start(QStringLiteral("curl"), {QStringLiteral("-s"), QStringLiteral("-L"), QStringLiteral("-m"), QStringLiteral("5"), QStringLiteral("https://edns.ip-api.com/json")});

        bool success = false;
        QString detectedIp;
        QString detectedGeo;
        bool isLeaking = false;

        if (proc.waitForFinished(6000) && proc.exitCode() == 0) {
            const QByteArray data = proc.readAllStandardOutput();
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject()) {
                const QJsonObject dnsObj = doc.object().value(QStringLiteral("dns")).toObject();
                detectedIp = dnsObj.value(QStringLiteral("ip")).toString();
                detectedGeo = dnsObj.value(QStringLiteral("geo")).toString();
                if (!detectedIp.isEmpty()) {
                    success = true;
                }
            }
        }

        // Determine if leaking:
        // If current provider is Cloudflare/Google/AdGuard/Quad9, but detected Geo mentions local ISP or does not match
        if (success) {
            const QString activeLow = m_activeProvider.toLower();
            const QString geoLow = detectedGeo.toLower();

            if (activeLow.contains(QStringLiteral("cloudflare")) && !geoLow.contains(QStringLiteral("cloudflare"))) {
                isLeaking = true;
            } else if (activeLow.contains(QStringLiteral("google")) && !geoLow.contains(QStringLiteral("google"))) {
                isLeaking = true;
            } else if (activeLow.contains(QStringLiteral("adguard")) && !geoLow.contains(QStringLiteral("adguard"))) {
                isLeaking = true;
            } else if (activeLow.contains(QStringLiteral("quad9")) && !geoLow.contains(QStringLiteral("quad9"))) {
                isLeaking = true;
            }
        }

        QMetaObject::invokeMethod(this, [this, success, detectedIp, detectedGeo, isLeaking]() {
            m_leakResult = QVariantMap{
                {QStringLiteral("success"), success},
                {QStringLiteral("detectedIp"), detectedIp.isEmpty() ? i18n("Unknown / Request Timed Out") : detectedIp},
                {QStringLiteral("detectedGeo"), detectedGeo.isEmpty() ? i18n("N/A") : detectedGeo},
                {QStringLiteral("isLeaking"), isLeaking},
                {QStringLiteral("statusMessage"), success
                    ? (isLeaking ? i18n("DNS Leak Detected: Your requests are handled by '%1' rather than your configured encrypted provider!", detectedGeo)
                                 : i18n("No DNS Leaks: Resolver '%1' matches your configured provider.", detectedGeo))
                    : i18n("Failed to reach leak test service. Check your internet connection.")}
            };

            m_isCheckingLeak = false;
            emit isCheckingLeakChanged();
            emit leakResultChanged();

            LogModel::instance()->append(
                QStringLiteral("DnsManager"),
                isLeaking ? QStringLiteral("warning") : QStringLiteral("info"),
                QStringLiteral("Leak test completed: Visible resolver %1 (%2), Leaking: %3")
                    .arg(detectedIp, detectedGeo, isLeaking ? QStringLiteral("YES") : QStringLiteral("NO"))
            );
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void DnsManager::openLeakTestInBrowser()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://dnsleaktest.com")));
}
