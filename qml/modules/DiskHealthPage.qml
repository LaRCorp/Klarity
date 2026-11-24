import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    property string currentView: "list" // "list" or "detail"

    title: currentView === "detail"
           ? i18n("Disk Details: %1", diskMgr.selectedDevice)
           : i18n("Disk Health & S.M.A.R.T.")

    DiskHealthManager {
        id: diskMgr

        onOperationFinished: (success, message) => {
            if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                applicationWindow().showPassiveNotification(message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            visible: root.currentView === "detail"
            text: i18n("Back to Disks")
            icon.name: "go-previous"
            tooltip: i18n("Return to all detected storage drives")
            onTriggered: root.currentView = "list"
        },
        Kirigami.Action {
            text: i18n("Scan Disks")
            icon.name: "view-refresh"
            tooltip: i18n("Scan system drives and reload S.M.A.R.T. telemetry")
            enabled: !diskMgr.isScanning && !diskMgr.isBusy
            onTriggered: {
                if (root.currentView === "detail" && diskMgr.selectedDevice.length > 0) {
                    diskMgr.refreshDisk(diskMgr.selectedDevice)
                } else {
                    diskMgr.scanDisks()
                }
            }
        },
        Kirigami.Action {
            text: diskMgr.smartdActive ? i18n("smartd: Active") : i18n("smartd: Inactive")
            icon.name: diskMgr.smartdActive ? "dialog-ok" : "dialog-warning"
            tooltip: i18n("Toggle automatic background drive health monitoring daemon (smartd)")
            onTriggered: diskMgr.toggleSmartd(!diskMgr.smartdActive)
        },
        Kirigami.Action {
            text: i18n("Clear Logs")
            icon.name: "edit-clear"
            tooltip: i18n("Clear diagnostic terminal log")
            onTriggered: diskMgr.clearLogs()
        },
        Kirigami.Action {
            text: i18n("Execution Logs")
            icon.name: "utilities-terminal"
            tooltip: i18n("Open system execution logs drawer")
            onTriggered: {
                if (applicationWindow() && applicationWindow().contextDrawer) {
                    applicationWindow().contextDrawer.open()
                }
            }
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        ScanProgress {
            Layout.fillWidth: true
            running: diskMgr.isScanning
            text: i18n("Scanning storage controllers and querying S.M.A.R.T. telemetry...")
        }

        // =========================================================================
        // SMARTD BACKGROUND MONITORING BANNER
        // =========================================================================
        Kirigami.Card {
            Layout.fillWidth: true

            header: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: diskMgr.smartdActive ? "security-high" : "security-medium"
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Kirigami.Heading {
                        level: 4
                        text: i18n("Background SMART Monitoring (smartd)")
                    }

                    QQC2.Label {
                        text: diskMgr.smartdActive
                              ? i18n("smartd service is active and will alert you upon disk attribute degradation.")
                              : i18n("smartd daemon is inactive. Enable it to get early warnings for drive failures.")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.75
                    }
                }

                Rectangle {
                    implicitWidth: smartdStatusLabel.implicitWidth + 14
                    implicitHeight: smartdStatusLabel.implicitHeight + 6
                    radius: 4
                    color: diskMgr.smartdActive ? "#203b29" : "#3b3420"

                    QQC2.Label {
                        id: smartdStatusLabel
                        anchors.centerIn: parent
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        text: diskMgr.smartdActive ? i18n("MONITORING ACTIVE") : i18n("DISABLED")
                        color: diskMgr.smartdActive ? "#5af78e" : "#ffcc00"
                    }
                }

                QQC2.Button {
                    text: diskMgr.smartdActive ? i18n("Stop smartd") : i18n("Enable Background Monitor")
                    icon.name: diskMgr.smartdActive ? "process-stop" : "system-run"
                    enabled: !diskMgr.isBusy
                    onClicked: diskMgr.toggleSmartd(!diskMgr.smartdActive)
                }
            }
        }

        // =========================================================================
        // VIEW 1: DRIVES LIST VIEW (MAIN OVERVIEW)
        // =========================================================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            visible: root.currentView === "list"

            Kirigami.Heading {
                level: 3
                text: i18n("Detected Physical Storage Drives (%1)", diskMgr.disks.length)
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width > Kirigami.Units.gridUnit * 42 ? 2 : 1
                rowSpacing: Kirigami.Units.mediumSpacing
                columnSpacing: Kirigami.Units.mediumSpacing

                Repeater {
                    model: diskMgr.disks

                    delegate: Kirigami.Card {
                        Layout.fillWidth: true

                        header: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            // Health Circle Status Indicator
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                color: modelData.healthColor || "#888888"
                                border.color: "#ffffff"
                                border.width: 1
                            }

                            Kirigami.Heading {
                                level: 4
                                text: modelData.model || modelData.device
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            // Protocol Badge (NVMe or SATA)
                            Rectangle {
                                implicitWidth: typeLabel.implicitWidth + 10
                                implicitHeight: typeLabel.implicitHeight + 4
                                radius: 4
                                color: modelData.isNvme ? "#1c354d" : "#322744"

                                QQC2.Label {
                                    id: typeLabel
                                    anchors.centerIn: parent
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                                    text: modelData.type || "DISK"
                                    color: modelData.isNvme ? "#70b9ff" : "#cb9eff"
                                }
                            }
                        }

                        contentItem: ColumnLayout {
                            spacing: Kirigami.Units.smallSpacing

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                rowSpacing: 4
                                columnSpacing: Kirigami.Units.mediumSpacing

                                QQC2.Label {
                                    text: i18n("Device:")
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.device
                                    font.family: "monospace"
                                    font.bold: true
                                }

                                QQC2.Label {
                                    text: i18n("Serial Number:")
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.serial
                                    font.family: "monospace"
                                }

                                QQC2.Label {
                                    text: i18n("Capacity:")
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.size
                                    font.bold: true
                                }

                                QQC2.Label {
                                    text: i18n("Temperature:")
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.temperature
                                    font.bold: true
                                    color: modelData.temperatureValue > 65 ? "#ff5555" : (modelData.temperatureValue > 50 ? "#ffcc00" : "#5af78e")
                                }

                                QQC2.Label {
                                    text: i18n("SMART Status:")
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.healthStatus
                                    font.bold: true
                                    color: modelData.healthColor || "#888888"
                                }
                            }

                            Kirigami.Separator { Layout.fillWidth: true }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.Button {
                                    text: i18n("Details & S.M.A.R.T.")
                                    icon.name: "document-properties"
                                    onClicked: {
                                        diskMgr.selectDisk(modelData.device)
                                        root.currentView = "detail"
                                    }
                                }

                                QQC2.Button {
                                    text: i18n("Refresh")
                                    icon.name: "view-refresh"
                                    enabled: !diskMgr.isBusy
                                    onClicked: diskMgr.refreshDisk(modelData.device)
                                }
                            }
                        }
                    }
                }
            }
        }

        // =========================================================================
        // VIEW 2: DISK DETAILED DIAGNOSTICS VIEW
        // =========================================================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            visible: root.currentView === "detail"

            // Header toolbar
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Button {
                    text: i18n("← Back to All Disks")
                    icon.name: "go-previous"
                    onClicked: root.currentView = "list"
                }

                Item { Layout.fillWidth: true }

                QQC2.Button {
                    text: i18n("Refresh SMART")
                    icon.name: "view-refresh"
                    enabled: !diskMgr.isBusy
                    onClicked: diskMgr.refreshDisk(diskMgr.selectedDevice)
                }
            }

            // Section 1: Overview & Verdict Card
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: diskMgr.selectedDiskDetail.healthStatus === "PASSED" ? "dialog-ok" : "dialog-warning"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Kirigami.Heading {
                            level: 3
                            text: diskMgr.selectedDiskDetail.model || diskMgr.selectedDevice
                        }

                        QQC2.Label {
                            text: i18n("Device: %1 | Firmware: %2 | Type: %3",
                                       diskMgr.selectedDiskDetail.device || "",
                                       diskMgr.selectedDiskDetail.firmware || "",
                                       diskMgr.selectedDiskDetail.type || "")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.7
                        }
                    }

                    Rectangle {
                        implicitWidth: verdictLabel.implicitWidth + 18
                        implicitHeight: verdictLabel.implicitHeight + 8
                        radius: 6
                        color: diskMgr.selectedDiskDetail.healthColor || "#888888"

                        QQC2.Label {
                            id: verdictLabel
                            anchors.centerIn: parent
                            font.bold: true
                            text: diskMgr.selectedDiskDetail.healthStatus || "UNKNOWN"
                            color: "#000000"
                        }
                    }
                }

                contentItem: GridLayout {
                    Layout.fillWidth: true
                    columns: root.width > Kirigami.Units.gridUnit * 40 ? 4 : 2
                    rowSpacing: Kirigami.Units.mediumSpacing
                    columnSpacing: Kirigami.Units.mediumSpacing

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: i18n("Capacity"); font.pointSize: Kirigami.Theme.smallFont.pointSize; opacity: 0.7 }
                        QQC2.Label { text: diskMgr.selectedDiskDetail.size || "Unknown"; font.bold: true; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1 }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: i18n("Temperature"); font.pointSize: Kirigami.Theme.smallFont.pointSize; opacity: 0.7 }
                        QQC2.Label {
                            text: diskMgr.selectedDiskDetail.temperature || "N/A"
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                            color: (diskMgr.selectedDiskDetail.temperatureValue || 0) > 65 ? "#ff5555" : "#5af78e"
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: i18n("Power On Hours"); font.pointSize: Kirigami.Theme.smallFont.pointSize; opacity: 0.7 }
                        QQC2.Label { text: i18n("%1 hrs", diskMgr.selectedDiskDetail.powerOnHours || 0); font.bold: true; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1 }
                    }

                    ColumnLayout {
                        spacing: 2
                        QQC2.Label { text: i18n("Power Cycles"); font.pointSize: Kirigami.Theme.smallFont.pointSize; opacity: 0.7 }
                        QQC2.Label { text: (diskMgr.selectedDiskDetail.powerCycles || 0).toString(); font.bold: true; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1 }
                    }
                }
            }

            // Section 2: Critical Attributes Cards
            Kirigami.Heading {
                level: 3
                text: i18n("Critical Reliability Indicators")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width > Kirigami.Units.gridUnit * 42 ? 4 : 2
                rowSpacing: Kirigami.Units.mediumSpacing
                columnSpacing: Kirigami.Units.mediumSpacing

                // Metric 1: Reallocated / Media Errors
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 5.5
                    radius: 6
                    color: (diskMgr.selectedDiskDetail.mediaErrors || 0) > 0 ? "#452424" : "#1e222b"
                    border.color: (diskMgr.selectedDiskDetail.mediaErrors || 0) > 0 ? "#ff5555" : "#313844"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: 2

                        QQC2.Label {
                            text: i18n("Media / Sector Errors")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.75
                        }
                        QQC2.Label {
                            text: (diskMgr.selectedDiskDetail.mediaErrors || 0).toString()
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
                            color: (diskMgr.selectedDiskDetail.mediaErrors || 0) > 0 ? "#ff5555" : "#5af78e"
                        }
                    }
                }

                // Metric 2: Unsafe Shutdowns
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 5.5
                    radius: 6
                    color: "#1e222b"
                    border.color: "#313844"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: 2

                        QQC2.Label {
                            text: i18n("Unsafe Shutdowns")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.75
                        }
                        QQC2.Label {
                            text: (diskMgr.selectedDiskDetail.unsafeShutdowns || 0).toString()
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
                        }
                    }
                }

                // Metric 3: Wear Indicator
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 5.5
                    radius: 6
                    color: "#1e222b"
                    border.color: "#313844"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: 2

                        QQC2.Label {
                            text: i18n("SSD Wear Level")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.75
                        }
                        QQC2.Label {
                            text: diskMgr.selectedDiskDetail.wearIndicator || i18n("N/A")
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
                            color: "#70b9ff"
                        }
                    }
                }

                // Metric 4: Critical Warnings
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 5.5
                    radius: 6
                    color: (diskMgr.selectedDiskDetail.criticalWarning || 0) > 0 ? "#452424" : "#1e222b"
                    border.color: (diskMgr.selectedDiskDetail.criticalWarning || 0) > 0 ? "#ff5555" : "#313844"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: 2

                        QQC2.Label {
                            text: i18n("Critical Warning Code")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.75
                        }
                        QQC2.Label {
                            text: (diskMgr.selectedDiskDetail.criticalWarning || 0).toString()
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
                            color: (diskMgr.selectedDiskDetail.criticalWarning || 0) > 0 ? "#ff5555" : "#5af78e"
                        }
                    }
                }
            }

            // Section 3: Diagnostic Tests Card
            Kirigami.Card {
                Layout.fillWidth: true

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: "system-run"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 3
                            text: i18n("Self-Test Execution & Diagnostics")
                            Layout.fillWidth: true
                        }
                    }

                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        QQC2.Label {
                            text: i18n("Status: %1", diskMgr.selectedDiskDetail.selfTestStatus || i18n("Idle"))
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        QQC2.BusyIndicator {
                            running: diskMgr.selectedDiskDetail.selfTestInProgress || diskMgr.isBusy
                            visible: running
                            implicitWidth: 20
                            implicitHeight: 20
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Button {
                            text: i18n("Run Short Test (-t short)")
                            icon.name: "media-playback-start"
                            enabled: !diskMgr.isBusy
                            onClicked: diskMgr.runSelfTest(diskMgr.selectedDevice, "short")
                        }

                        QQC2.Button {
                            text: i18n("Run Extended Test (-t long)")
                            icon.name: "media-playback-start"
                            enabled: !diskMgr.isBusy
                            onClicked: diskMgr.runSelfTest(diskMgr.selectedDevice, "long")
                        }

                        QQC2.Button {
                            text: i18n("Abort Test (-X)")
                            icon.name: "process-stop"
                            enabled: !diskMgr.isBusy
                            onClicked: diskMgr.abortSelfTest(diskMgr.selectedDevice)
                        }
                    }
                }
            }

            // Section 4: Full Attributes Table
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Heading {
                        level: 3
                        text: i18n("Full S.M.A.R.T. Attribute Table")
                        Layout.fillWidth: true
                    }
                }

                contentItem: ColumnLayout {
                    spacing: 4

                    // Table Header
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 32
                        color: "#252b33"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.smallSpacing
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label { text: i18n("ID"); font.bold: true; Layout.preferredWidth: 45 }
                            QQC2.Label { text: i18n("Attribute Name"); font.bold: true; Layout.fillWidth: true }
                            QQC2.Label { text: i18n("Current"); font.bold: true; Layout.preferredWidth: 60 }
                            QQC2.Label { text: i18n("Worst"); font.bold: true; Layout.preferredWidth: 60 }
                            QQC2.Label { text: i18n("Thresh"); font.bold: true; Layout.preferredWidth: 60 }
                            QQC2.Label { text: i18n("Raw Value"); font.bold: true; Layout.preferredWidth: 90 }
                        }
                    }

                    Repeater {
                        model: diskMgr.selectedDiskDetail.attributes

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 32
                            radius: 4
                            color: modelData.isWarning ? "#3a2222" : (index % 2 === 0 ? "#191c22" : "#20242b")
                            border.color: modelData.isWarning ? "#ff5555" : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.smallSpacing
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.Label {
                                    text: modelData.id
                                    font.family: "monospace"
                                    Layout.preferredWidth: 45
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.name
                                    Layout.fillWidth: true
                                    font.bold: modelData.isCritical
                                    color: modelData.isWarning ? "#ff8585" : Kirigami.Theme.textColor
                                    elide: Text.ElideRight
                                }
                                QQC2.Label {
                                    text: modelData.value
                                    font.family: "monospace"
                                    Layout.preferredWidth: 60
                                }
                                QQC2.Label {
                                    text: modelData.worst
                                    font.family: "monospace"
                                    Layout.preferredWidth: 60
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.threshold
                                    font.family: "monospace"
                                    Layout.preferredWidth: 60
                                    opacity: 0.7
                                }
                                QQC2.Label {
                                    text: modelData.raw
                                    font.family: "monospace"
                                    font.bold: modelData.isWarning
                                    color: modelData.isWarning ? "#ff8585" : Kirigami.Theme.textColor
                                    Layout.preferredWidth: 90
                                }
                            }
                        }
                    }
                }
            }
        }

        // =========================================================================
        // PRIVILEGED PROCESS CONSOLE OUTPUT
        // =========================================================================
        Kirigami.Card {
            Layout.fillWidth: true
            visible: diskMgr.isBusy || diskMgr.logOutput.length > 0

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "utilities-terminal"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    Kirigami.Heading {
                        level: 4
                        text: diskMgr.isBusy ? i18n("Executing: %1", diskMgr.operationStatus) : i18n("smartctl Log Output")
                        Layout.fillWidth: true
                    }

                    QQC2.BusyIndicator {
                        running: diskMgr.isBusy
                        implicitWidth: 20
                        implicitHeight: 20
                    }

                    QQC2.Button {
                        text: i18n("Clear")
                        icon.name: "edit-clear"
                        enabled: diskMgr.logOutput.length > 0
                        onClicked: diskMgr.clearLogs()
                    }
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 14
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 14
                    color: "#16191e"
                    radius: 6
                    border.color: "#2e3440"
                    border.width: 1
                    clip: true

                    QQC2.ScrollView {
                        id: diskLogScrollView
                        anchors.fill: parent
                        anchors.margins: 4
                        clip: true

                        QQC2.TextArea {
                            id: diskLogArea
                            readOnly: true
                            text: diskMgr.logOutput
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: "#d8dee9"
                            background: null
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true

                            onTextChanged: {
                                cursorPosition = length
                            }
                        }
                    }
                }
            }
        }
    }
}
