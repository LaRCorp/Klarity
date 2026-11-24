import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("DNS Center")

    DnsManager {
        id: dnsMgr
        onOperationFinished: (success, message) => {
            if (success) {
                if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                    applicationWindow().showPassiveNotification(message)
                }
            } else {
                errorBanner.showError(message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Refresh")
            icon.name: "view-refresh"
            tooltip: i18n("Reload current DNS configuration from systemd-resolved and NetworkManager")
            onTriggered: dnsMgr.refreshCurrentDns()
        },
        Kirigami.Action {
            text: i18n("Check DNS")
            icon.name: "network-connect"
            tooltip: i18n("Test active DNS resolution availability and latency")
            onTriggered: dnsMgr.checkDns(checkDomainField.text)
        },
        Kirigami.Action {
            text: i18n("Compare Providers")
            icon.name: "speedometer"
            tooltip: i18n("Benchmark latency across Cloudflare, Google, AdGuard, and Quad9")
            onTriggered: dnsMgr.compareProviders(checkDomainField.text)
        },
        Kirigami.Action {
            text: i18n("Leak Test")
            icon.name: "security-high"
            tooltip: i18n("Check if DNS queries are leaking or intercepted by your ISP")
            onTriggered: dnsMgr.checkLeaks()
        },
        Kirigami.Action {
            text: i18n("Execution Logs")
            icon.name: "utilities-terminal"
            tooltip: i18n("Open execution logs")
            onTriggered: {
                if (applicationWindow() && applicationWindow().contextDrawer) {
                    applicationWindow().contextDrawer.open()
                }
            }
        }
    ]

    ConfirmDialog {
        id: changeDnsConfirmDialog
        title: i18n("Confirm DNS Change")
        dangerous: false
        property string targetProviderId: ""
        property string targetProviderName: ""
        property string targetServers: ""

        onConfirmed: {
            dnsMgr.setDnsProvider(targetProviderId, targetServers)
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        ErrorBanner {
            id: errorBanner
            Layout.fillWidth: true
        }

        ScanProgress {
            Layout.fillWidth: true
            running: dnsMgr.isLoading || dnsMgr.isBenchmarking || dnsMgr.isCheckingLeak
            text: {
                if (dnsMgr.isBenchmarking) return i18n("Benchmarking DNS providers latency...")
                if (dnsMgr.isCheckingLeak) return i18n("Testing DNS leaks and detecting visible resolver...")
                return i18n("Querying network interfaces and DNS servers...")
            }
            onCancelled: dnsMgr.refreshCurrentDns()
        }

        // ================= SECTION 1: CURRENT ACTIVE DNS =================
        Kirigami.Card {
            Layout.fillWidth: true

            header: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "network-wired"
                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                    implicitHeight: Kirigami.Units.iconSizes.smallMedium
                }

                Kirigami.Heading {
                    level: 3
                    text: i18n("Active DNS Configuration")
                }

                Item { Layout.fillWidth: true }

                QQC2.Button {
                    icon.name: "view-refresh"
                    text: i18n("Refresh")
                    onClicked: dnsMgr.refreshCurrentDns()
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                // Key Status Badges & IP
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label {
                            text: i18n("Primary DNS Server")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.7
                        }
                        QQC2.Label {
                            text: dnsMgr.activeServer.length > 0 ? dnsMgr.activeServer : i18n("Detecting...")
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
                            font.family: "monospace"
                            color: "#3daee9"
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label {
                            text: i18n("Identified Provider")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.7
                        }
                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Rectangle {
                                implicitHeight: Kirigami.Units.gridUnit * 1.4
                                implicitWidth: activeProviderLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                                radius: 4
                                color: "#16283a"
                                border.color: "#1d99f3"
                                border.width: 1
                                QQC2.Label {
                                    id: activeProviderLabel
                                    anchors.centerIn: parent
                                    text: dnsMgr.activeProvider
                                    color: "#3daee9"
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label {
                            text: i18n("Active Interface")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.7
                        }
                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Kirigami.Icon {
                                source: dnsMgr.activeInterface.indexOf("Wi-Fi") !== -1 ? "network-wireless" : "network-wired"
                                implicitWidth: 16
                                implicitHeight: 16
                            }
                            QQC2.Label {
                                text: dnsMgr.activeInterface.length > 0 ? dnsMgr.activeInterface : i18n("None")
                                font.bold: true
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label {
                            text: i18n("Connection")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.7
                        }
                        QQC2.Label {
                            text: dnsMgr.activeConnection.length > 0 ? dnsMgr.activeConnection : i18n("Direct")
                            font.bold: true
                            opacity: 0.85
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label {
                            text: i18n("Encryption (DoT/DoH)")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.7
                        }
                        Rectangle {
                            implicitHeight: Kirigami.Units.gridUnit * 1.4
                            implicitWidth: encStatusLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                            radius: 4
                            color: dnsMgr.isEncrypted ? "#15332f" : "#2f3640"
                            border.color: dnsMgr.isEncrypted ? "#16a085" : "#718093"
                            border.width: 1
                            QQC2.Label {
                                id: encStatusLabel
                                anchors.centerIn: parent
                                text: dnsMgr.encryptionStatus
                                color: dnsMgr.isEncrypted ? "#2ecc71" : "#dcdde1"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }
                    }
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                // All Configured Servers on this link
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: i18n("Configured DNS Servers:")
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: dnsMgr.dnsServers.length > 0 ? dnsMgr.dnsServers.join("   •   ") : i18n("Using default local resolver")
                        font.family: "monospace"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        color: "#95a5a6"
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // ================= SECTION 2: SWITCH DNS PROVIDER =================
        Kirigami.Card {
            Layout.fillWidth: true

            header: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "preferences-system-network"
                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                    implicitHeight: Kirigami.Units.iconSizes.smallMedium
                }

                Kirigami.Heading {
                    level: 3
                    text: i18n("Switch DNS Provider")
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing

                QQC2.Label {
                    text: i18n("Choose a DNS provider to enhance browsing speed, bypass ISP restrictions, block advertisements, or protect against phishing attacks:")
                    opacity: 0.8
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // Provider Selection Dropdown & Apply Action
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    QQC2.Label {
                        text: i18n("Select Provider:")
                        font.bold: true
                    }

                    QQC2.ComboBox {
                        id: providerCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        model: dnsMgr.availableProviders
                        currentIndex: 1 // Default to Cloudflare for quick setup
                    }

                    QQC2.Button {
                        text: i18n("Apply DNS")
                        icon.name: "dialog-ok"
                        enabled: !dnsMgr.isLoading && dnsMgr.activeConnection.length > 0
                        onClicked: {
                            const cur = providerCombo.model[providerCombo.currentIndex]
                            if (cur.id === "custom") {
                                if (customServersField.text.trim().length === 0) {
                                    errorBanner.showError(i18n("Please specify at least one valid DNS server IP address."))
                                    return
                                }
                                changeDnsConfirmDialog.targetProviderId = "custom"
                                changeDnsConfirmDialog.targetProviderName = i18n("Custom DNS")
                                changeDnsConfirmDialog.targetServers = customServersField.text.trim()
                                changeDnsConfirmDialog.text = i18n("Are you sure you want to set custom DNS servers '%1' on connection '%2'?", customServersField.text.trim(), dnsMgr.activeConnection)
                            } else {
                                changeDnsConfirmDialog.targetProviderId = cur.id
                                changeDnsConfirmDialog.targetProviderName = cur.name
                                changeDnsConfirmDialog.targetServers = cur.servers
                                changeDnsConfirmDialog.text = i18n("Are you sure you want to switch DNS on connection '%1' to %2 (%3)?", dnsMgr.activeConnection, cur.name, cur.servers.length > 0 ? cur.servers : i18n("DHCP"))
                            }
                            changeDnsConfirmDialog.open()
                        }
                    }
                }

                // Selected Provider Card Details
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: providerDetailsCol.implicitHeight + Kirigami.Units.mediumSpacing * 2
                    color: "#252b33"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#38414a"
                    border.width: 1

                    ColumnLayout {
                        id: providerDetailsCol
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            spacing: Kirigami.Units.mediumSpacing
                            Kirigami.Icon {
                                source: providerCombo.model[providerCombo.currentIndex].icon
                                implicitWidth: 28
                                implicitHeight: 28
                            }
                            ColumnLayout {
                                spacing: 2
                                QQC2.Label {
                                    text: providerCombo.model[providerCombo.currentIndex].name
                                    font.bold: true
                                }
                                QQC2.Label {
                                    text: providerCombo.model[providerCombo.currentIndex].description
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    opacity: 0.8
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        RowLayout {
                            visible: providerCombo.model[providerCombo.currentIndex].servers.length > 0
                            spacing: Kirigami.Units.smallSpacing
                            QQC2.Label {
                                text: i18n("Server Addresses:")
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                            QQC2.Label {
                                text: providerCombo.model[providerCombo.currentIndex].servers
                                font.family: "monospace"
                                font.bold: true
                                color: "#3daee9"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }
                    }
                }

                // Custom DNS Address Field (Only visible when Custom selected)
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: providerCombo.model[providerCombo.currentIndex].id === "custom"
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: i18n("Custom DNS IP Addresses (space-separated, IPv4 or IPv6):")
                        font.bold: true
                    }

                    QQC2.TextField {
                        id: customServersField
                        Layout.fillWidth: true
                        placeholderText: "e.g. 1.1.1.1 1.0.0.1 2606:4700:4700::1111"
                    }
                }
            }
        }

        // ================= SECTION 3: DIAGNOSTICS & BENCHMARK =================
        Kirigami.Card {
            Layout.fillWidth: true

            header: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "speedometer"
                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                    implicitHeight: Kirigami.Units.iconSizes.smallMedium
                }

                Kirigami.Heading {
                    level: 3
                    text: i18n("DNS Diagnostics & Latency Benchmark")
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing

                // Test Domain & Action Buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    QQC2.Label {
                        text: i18n("Benchmark Target:")
                        font.bold: true
                    }

                    QQC2.TextField {
                        id: checkDomainField
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 12
                        text: "google.com"
                        placeholderText: "google.com"
                    }

                    QQC2.Button {
                        text: i18n("Check DNS")
                        icon.name: "network-connect"
                        enabled: !dnsMgr.isLoading && !dnsMgr.isBenchmarking
                        onClicked: dnsMgr.checkDns(checkDomainField.text)
                    }

                    QQC2.Button {
                        text: i18n("Compare Providers")
                        icon.name: "speedometer"
                        enabled: !dnsMgr.isLoading && !dnsMgr.isBenchmarking
                        onClicked: dnsMgr.compareProviders(checkDomainField.text)
                    }

                    QQC2.Button {
                        text: i18n("Test DNS Leaks")
                        icon.name: "security-high"
                        enabled: !dnsMgr.isCheckingLeak
                        onClicked: dnsMgr.checkLeaks()
                    }

                    QQC2.Button {
                        text: i18n("Open dnsleaktest.com")
                        icon.name: "internet-web-browser"
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Open full browser test suite at dnsleaktest.com")
                        onClicked: dnsMgr.openLeakTestInBrowser()
                    }
                }

                // 1. QUICK DNS CHECK RESULT
                Rectangle {
                    Layout.fillWidth: true
                    visible: dnsMgr.checkResult.hasOwnProperty("success")
                    implicitHeight: checkResRow.implicitHeight + Kirigami.Units.mediumSpacing * 2
                    color: dnsMgr.checkResult.success ? "#16283a" : "#3d1b1b"
                    border.color: dnsMgr.checkResult.success ? "#1d99f3" : "#e74c3c"
                    border.width: 1
                    radius: Kirigami.Units.smallSpacing

                    RowLayout {
                        id: checkResRow
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: Kirigami.Units.mediumSpacing

                        Kirigami.Icon {
                            source: dnsMgr.checkResult.success ? "dialog-ok" : "dialog-error"
                            implicitWidth: 28
                            implicitHeight: 28
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: dnsMgr.checkResult.success ? i18n("DNS Resolution Working Properly") : i18n("DNS Resolution Failed")
                                font.bold: true
                                color: dnsMgr.checkResult.success ? "#2ecc71" : "#e74c3c"
                            }
                            QQC2.Label {
                                text: dnsMgr.checkResult.domain ? i18n("Domain '%1' resolved to %2 via server %3", dnsMgr.checkResult.domain, dnsMgr.checkResult.resolvedIp, dnsMgr.checkResult.server) : ""
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.85
                            }
                        }

                        Rectangle {
                            implicitHeight: Kirigami.Units.gridUnit * 1.5
                            implicitWidth: latencyPill.implicitWidth + Kirigami.Units.mediumSpacing * 2
                            radius: 4
                            color: (dnsMgr.checkResult.latencyMs || 0) < 50 ? "#1b4d3e" : ((dnsMgr.checkResult.latencyMs || 0) < 100 ? "#2c3e50" : "#5d4037")
                            QQC2.Label {
                                id: latencyPill
                                anchors.centerIn: parent
                                text: dnsMgr.checkResult.latencyMs !== undefined ? i18n("%1 ms", dnsMgr.checkResult.latencyMs) : ""
                                font.bold: true
                                color: "#ffffff"
                            }
                        }
                    }
                }

                // 2. LEAK TEST RESULT
                Rectangle {
                    Layout.fillWidth: true
                    visible: dnsMgr.leakResult.hasOwnProperty("success")
                    implicitHeight: leakResRow.implicitHeight + Kirigami.Units.mediumSpacing * 2
                    color: dnsMgr.leakResult.isLeaking ? "#3d2a1b" : "#162d26"
                    border.color: dnsMgr.leakResult.isLeaking ? "#e67e22" : "#2ecc71"
                    border.width: 1
                    radius: Kirigami.Units.smallSpacing

                    RowLayout {
                        id: leakResRow
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: Kirigami.Units.mediumSpacing

                        Kirigami.Icon {
                            source: dnsMgr.leakResult.isLeaking ? "dialog-warning" : "security-high"
                            implicitWidth: 28
                            implicitHeight: 28
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: dnsMgr.leakResult.isLeaking ? i18n("Potential DNS Leak Detected!") : i18n("DNS Leak Test Passed")
                                font.bold: true
                                color: dnsMgr.leakResult.isLeaking ? "#f39c12" : "#2ecc71"
                            }
                            QQC2.Label {
                                text: dnsMgr.leakResult.statusMessage || ""
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.9
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            QQC2.Label {
                                text: i18n("Visible Resolver:")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: dnsMgr.leakResult.detectedIp || ""
                                font.bold: true
                                font.family: "monospace"
                                color: "#3daee9"
                            }
                        }
                    }
                }

                // 3. BENCHMARK COMPARISON TABLE
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: dnsMgr.benchmarkResults.length > 0
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: i18n("Benchmark Results for '%1':", checkDomainField.text)
                        font.bold: true
                    }

                    Repeater {
                        model: dnsMgr.benchmarkResults

                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            height: 52
                            radius: 4
                            color: modelData.isFastest ? "#173426" : "#252b33"
                            border.color: modelData.isFastest ? "#27ae60" : "#38414a"
                            border.width: modelData.isFastest ? 1.5 : 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.mediumSpacing
                                spacing: Kirigami.Units.mediumSpacing

                                Kirigami.Icon {
                                    source: modelData.isFastest ? "rating" : "speedometer"
                                    implicitWidth: 22
                                    implicitHeight: 22
                                }

                                ColumnLayout {
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 12
                                    spacing: 1
                                    QQC2.Label {
                                        text: modelData.name
                                        font.bold: true
                                    }
                                    QQC2.Label {
                                        text: modelData.ip
                                        font.family: "monospace"
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        opacity: 0.7
                                    }
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: modelData.features
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    opacity: 0.8
                                    elide: Text.ElideRight
                                }

                                Rectangle {
                                    visible: modelData.isFastest
                                    implicitHeight: Kirigami.Units.gridUnit * 1.3
                                    implicitWidth: fastestBadge.implicitWidth + Kirigami.Units.smallSpacing * 2
                                    radius: 4
                                    color: "#27ae60"
                                    QQC2.Label {
                                        id: fastestBadge
                                        anchors.centerIn: parent
                                        text: i18n("FASTEST")
                                        font.bold: true
                                        color: "#ffffff"
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                    }
                                }

                                Rectangle {
                                    implicitHeight: Kirigami.Units.gridUnit * 1.4
                                    implicitWidth: Kirigami.Units.gridUnit * 4.5
                                    radius: 4
                                    color: {
                                        if (modelData.latencyMs < 0) return "#4a2828"
                                        if (modelData.latencyMs < 30) return "#1e4d2b"
                                        if (modelData.latencyMs < 70) return "#1f3a52"
                                        return "#4d381e"
                                    }
                                    QQC2.Label {
                                        anchors.centerIn: parent
                                        text: modelData.latencyMs >= 0 ? i18n("%1 ms", modelData.latencyMs) : i18n("Timed out")
                                        font.bold: true
                                        font.family: "monospace"
                                        color: "#ffffff"
                                    }
                                }

                                QQC2.Button {
                                    text: i18n("Switch")
                                    icon.name: "dialog-ok"
                                    visible: modelData.id !== "current"
                                    onClicked: {
                                        changeDnsConfirmDialog.targetProviderId = modelData.id
                                        changeDnsConfirmDialog.targetProviderName = modelData.name
                                        changeDnsConfirmDialog.targetServers = modelData.ip
                                        changeDnsConfirmDialog.text = i18n("Switch DNS configuration to '%1' (%2)?", modelData.name, modelData.ip)
                                        changeDnsConfirmDialog.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
