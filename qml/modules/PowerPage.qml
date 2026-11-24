import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Power & Performance")

    PowerManager {
        id: powerMgr

        onOperationFinished: (success, message) => {
            if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                applicationWindow().showPassiveNotification(message)
            }
        }
    }

    property bool expertDisclaimerAcknowledged: false

    actions: [
        Kirigami.Action {
            text: i18n("Refresh Telemetry")
            icon.name: "view-refresh"
            tooltip: i18n("Update CPU telemetry, frequencies, and governor status")
            onTriggered: powerMgr.refreshStatus()
        },
        Kirigami.Action {
            text: i18n("Clear Logs")
            icon.name: "edit-clear"
            tooltip: i18n("Clear privileged operation logs")
            onTriggered: powerMgr.clearLogs()
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

    // Disclaimer dialog before switching to Expert mode
    ConfirmDialog {
        id: expertDisclaimerDialog
        title: i18n("Expert Hardware Disclaimer")
        dangerous: true
        requireCheckbox: true
        confirmButtonText: i18n("I Understand the Risks")
        cancelButtonText: i18n("Return to Auto Mode")
        warningText: i18n("Direct processor frequency and power management may cause severe thermal throttling, hardware overheating, or system crashes.")
        message: i18n("Direct sysfs controls bypass automatic daemon safeguards. Apply changes only if you understand your hardware capabilities and cooling profile.")

        onConfirmed: {
            root.expertDisclaimerAcknowledged = true
            modeTabBar.currentIndex = 1
        }
        onCancelled: {
            modeTabBar.currentIndex = 0
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        // Tab selection: Auto vs Expert
        QQC2.TabBar {
            id: modeTabBar
            Layout.fillWidth: true
            currentIndex: 0

            onCurrentIndexChanged: {
                if (currentIndex === 1 && !root.expertDisclaimerAcknowledged) {
                    expertDisclaimerDialog.open()
                }
            }

            QQC2.TabButton {
                text: i18n("Automatic (auto-cpufreq)")
                icon.name: "battery-charging"
            }

            QQC2.TabButton {
                text: i18n("Expert (Direct Sysfs)")
                icon.name: "configure"
            }
        }

        // =========================================================================
        // TAB 1: AUTOMATIC (AUTO-CPUFREQ)
        // =========================================================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            visible: modeTabBar.currentIndex === 0

            // 1. Daemon Status Card
            Kirigami.Card {
                Layout.fillWidth: true

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: powerMgr.isAutoCpuFreqRunning ? "dialog-ok" : "dialog-warning"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 3
                            text: i18n("auto-cpufreq Service Status")
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            implicitWidth: daemonStatusBadge.implicitWidth + 14
                            implicitHeight: daemonStatusBadge.implicitHeight + 6
                            radius: 4
                            color: !powerMgr.isAutoCpuFreqInstalled
                                   ? "#452424"
                                   : (powerMgr.isAutoCpuFreqRunning ? "#203b29" : "#3b3420")

                            QQC2.Label {
                                id: daemonStatusBadge
                                anchors.centerIn: parent
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                                text: !powerMgr.isAutoCpuFreqInstalled
                                      ? i18n("Not Installed")
                                      : (powerMgr.isAutoCpuFreqRunning ? i18n("Active & Optimizing") : i18n("Stopped / Inactive"))
                                color: !powerMgr.isAutoCpuFreqInstalled
                                       ? "#ff8585"
                                       : (powerMgr.isAutoCpuFreqRunning ? "#5af78e" : "#ffcc00")
                            }
                        }
                    }

                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: !powerMgr.isAutoCpuFreqInstalled
                              ? i18n("The auto-cpufreq daemon is not installed on this system. You can install it via pacman to automatically balance battery life and CPU performance without manual intervention.")
                              : i18n("auto-cpufreq is an automatic CPU speed & power optimizer for Linux. It continuously monitors laptop battery status, system load, and temperatures.")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Button {
                            visible: !powerMgr.isAutoCpuFreqInstalled
                            text: i18n("Install auto-cpufreq")
                            icon.name: "download"
                            enabled: !powerMgr.isBusy
                            onClicked: powerMgr.installAutoCpuFreq()
                        }

                        QQC2.Button {
                            visible: powerMgr.isAutoCpuFreqInstalled
                            text: powerMgr.isAutoCpuFreqRunning ? i18n("Stop Daemon") : i18n("Start & Enable Daemon")
                            icon.name: powerMgr.isAutoCpuFreqRunning ? "media-playback-stop" : "media-playback-start"
                            enabled: !powerMgr.isBusy
                            onClicked: powerMgr.toggleAutoCpuFreq(!powerMgr.isAutoCpuFreqRunning)
                        }

                        QQC2.Button {
                            visible: powerMgr.isAutoCpuFreqInstalled && powerMgr.isAutoCpuFreqRunning
                            text: i18n("Restart Daemon")
                            icon.name: "system-reboot"
                            enabled: !powerMgr.isBusy
                            onClicked: powerMgr.toggleAutoCpuFreq(true)
                        }
                    }
                }
            }

            // 2. Active Telemetry & Daemon Decisions
            Kirigami.Card {
                Layout.fillWidth: true

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: "speedometer"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 3
                            text: i18n("Active System Decisions & Metrics")
                            Layout.fillWidth: true
                        }
                    }

                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.minimumHeight: Kirigami.Units.gridUnit * 5
                        columns: root.width > Kirigami.Units.gridUnit * 36 ? 3 : 1
                        rowSpacing: Kirigami.Units.mediumSpacing
                        columnSpacing: Kirigami.Units.mediumSpacing

                        // Metric 1: Governor
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: i18n("Active Governor")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: powerMgr.currentGovernor.toUpperCase()
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                                color: powerMgr.currentGovernor === "performance" ? "#70b9ff" : "#5af78e"
                            }
                        }

                        // Metric 2: Turbo Boost
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: i18n("Turbo Boost")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: !powerMgr.turboSupported
                                      ? i18n("Unsupported")
                                      : (powerMgr.isTurboEnabled ? i18n("Enabled / Auto") : i18n("Disabled"))
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                                color: powerMgr.isTurboEnabled ? "#5af78e" : "#ffaa55"
                            }
                        }

                        // Metric 3: Power Source
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: i18n("Power Supply State")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: powerMgr.onBattery
                                      ? i18n("Battery (%1%)", powerMgr.batteryLevel)
                                      : i18n("AC Mains (%1)", powerMgr.batteryStatus)
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                                color: powerMgr.onBattery ? "#ffaa55" : "#70b9ff"
                            }
                        }

                        // Metric 4: CPU Package Temperature
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: i18n("CPU Package Temp")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: powerMgr.cpuTemperature > 0 ? (powerMgr.cpuTemperature.toFixed(1) + " °C") : i18n("N/A")
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                                color: powerMgr.cpuTemperature > 80 ? "#ff5555" : (powerMgr.cpuTemperature > 65 ? "#ffaa55" : "#5af78e")
                            }
                        }

                        // Metric 5: Average Core Frequency
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: i18n("Average Frequency")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: Math.round(powerMgr.cpuAverageFrequencyMhz) + " MHz"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                            }
                        }

                        // Metric 6: Scaling Driver
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            QQC2.Label {
                                text: i18n("Kernel Driver")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                            QQC2.Label {
                                text: powerMgr.cpuDriver
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                            }
                        }
                    }
                }
            }

            // 3. Profiles Editor (/etc/auto-cpufreq.conf)
            Kirigami.Card {
                Layout.fillWidth: true

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.largeSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: "document-edit"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 3
                            text: i18n("Daemon Profile Configuration (/etc/auto-cpufreq.conf)")
                            Layout.fillWidth: true
                        }
                    }

                    Kirigami.Separator {
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width > Kirigami.Units.gridUnit * 40 ? 2 : 1
                        rowSpacing: Kirigami.Units.largeSpacing
                        columnSpacing: Kirigami.Units.largeSpacing

                        // Left Profile: Connected to Charger (AC)
                        Kirigami.Card {
                            Layout.fillWidth: true

                            header: Kirigami.Heading {
                                level: 4
                                text: i18n("🔌 Charger (AC Connected)")
                            }

                            contentItem: ColumnLayout {
                                spacing: Kirigami.Units.mediumSpacing

                                ColumnLayout {
                                    spacing: 2
                                    QQC2.Label { text: i18n("Scaling Governor:") }
                                    QQC2.ComboBox {
                                        id: chargerGovCombo
                                        Layout.fillWidth: true
                                        model: ["performance", "powersave", "ondemand", "schedutil"]
                                        currentIndex: Math.max(0, model.indexOf(powerMgr.chargerGovernor))
                                        onActivated: powerMgr.chargerGovernor = currentText
                                    }
                                }

                                ColumnLayout {
                                    spacing: 2
                                    QQC2.Label { text: i18n("Turbo Boost Policy:") }
                                    QQC2.ComboBox {
                                        id: chargerTurboCombo
                                        Layout.fillWidth: true
                                        model: ["auto", "always", "never"]
                                        currentIndex: Math.max(0, model.indexOf(powerMgr.chargerTurbo))
                                        onActivated: powerMgr.chargerTurbo = currentText
                                    }
                                }
                            }
                        }

                        // Right Profile: Running on Battery
                        Kirigami.Card {
                            Layout.fillWidth: true

                            header: Kirigami.Heading {
                                level: 4
                                text: i18n("🔋 Battery Power")
                            }

                            contentItem: ColumnLayout {
                                spacing: Kirigami.Units.mediumSpacing

                                ColumnLayout {
                                    spacing: 2
                                    QQC2.Label { text: i18n("Scaling Governor:") }
                                    QQC2.ComboBox {
                                        id: batteryGovCombo
                                        Layout.fillWidth: true
                                        model: ["powersave", "performance", "ondemand", "schedutil"]
                                        currentIndex: Math.max(0, model.indexOf(powerMgr.batteryGovernor))
                                        onActivated: powerMgr.batteryGovernor = currentText
                                    }
                                }

                                ColumnLayout {
                                    spacing: 2
                                    QQC2.Label { text: i18n("Turbo Boost Policy:") }
                                    QQC2.ComboBox {
                                        id: batteryTurboCombo
                                        Layout.fillWidth: true
                                        model: ["auto", "never", "always"]
                                        currentIndex: Math.max(0, model.indexOf(powerMgr.batteryTurbo))
                                        onActivated: powerMgr.batteryTurbo = currentText
                                    }
                                }
                            }
                        }
                    }

                    // Battery Thresholds (if supported)
                    Kirigami.Card {
                        Layout.fillWidth: true
                        visible: powerMgr.hasBatteryThresholds

                        header: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Heading {
                                level: 4
                                text: i18n("Battery Charging Thresholds")
                                Layout.fillWidth: true
                            }

                            StyledSwitch {
                                id: thresholdSwitch
                                checked: powerMgr.enableThresholds
                                onToggled: powerMgr.enableThresholds = checked
                            }
                        }

                        contentItem: ColumnLayout {
                            spacing: Kirigami.Units.mediumSpacing
                            enabled: thresholdSwitch.checked

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label {
                                    text: i18n("Start charging threshold: %1%", Math.round(startSlider.value))
                                    Layout.fillWidth: true
                                }
                                StyledSlider {
                                    id: startSlider
                                    Layout.preferredWidth: 200
                                    from: 10
                                    to: Math.min(90, stopSlider.value - 5)
                                    stepSize: 1
                                    value: powerMgr.startThreshold
                                    onMoved: powerMgr.startThreshold = Math.round(value)
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label {
                                    text: i18n("Stop charging threshold: %1%", Math.round(stopSlider.value))
                                    Layout.fillWidth: true
                                }
                                StyledSlider {
                                    id: stopSlider
                                    Layout.preferredWidth: 200
                                    from: Math.max(20, startSlider.value + 5)
                                    to: 100
                                    stepSize: 1
                                    value: powerMgr.stopThreshold
                                    onMoved: powerMgr.stopThreshold = Math.round(value)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        QQC2.Button {
                            text: i18n("Save & Restart Daemon")
                            icon.name: "document-save"
                            enabled: !powerMgr.isBusy
                            onClicked: powerMgr.saveAutoCpuFreqConfig()
                        }
                    }
                }
            }
        }

        // =========================================================================
        // TAB 2: EXPERT (DIRECT SYSFS CONTROL)
        // =========================================================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing
            visible: modeTabBar.currentIndex === 1

            // 1. Direct sysfs Control Card
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "configure"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    Kirigami.Heading {
                        level: 3
                        text: i18n("Kernel CPUFreq & Turbo Control")
                        Layout.fillWidth: true
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.largeSpacing

                    // Warning Inline Message
                    Kirigami.InlineMessage {
                        Layout.fillWidth: true
                        visible: true
                        type: Kirigami.MessageType.Warning
                        text: i18n("Changes made here are directly written to /sys/devices/system/cpu and affect all CPU cores.")
                    }

                    // Direct Governor selector
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            QQC2.Label {
                                text: i18n("CPU Scaling Governor (All Cores)")
                                font.bold: true
                            }
                            QQC2.Label {
                                text: i18n("Driver: %1 | Active: %2", powerMgr.cpuDriver, powerMgr.currentGovernor)
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                        }

                        QQC2.ComboBox {
                            id: expertGovCombo
                            Layout.preferredWidth: 160
                            model: powerMgr.availableGovernors
                            currentIndex: Math.max(0, powerMgr.availableGovernors.indexOf(powerMgr.currentGovernor))
                        }

                        QQC2.Button {
                            text: i18n("Apply Governor")
                            icon.name: "dialog-ok-apply"
                            enabled: !powerMgr.isBusy
                            onClicked: powerMgr.setGovernor(expertGovCombo.currentText)
                        }
                    }

                    Kirigami.Separator { Layout.fillWidth: true }

                    // Turbo Boost switch
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            QQC2.Label {
                                text: i18n("CPU Turbo Boost (no_turbo / cpufreq boost)")
                                font.bold: true
                            }
                            QQC2.Label {
                                text: powerMgr.turboSupported
                                      ? i18n("Toggle hardware thermal/frequency boost capability.")
                                      : i18n("Hardware turbo boost is not supported by current CPU or driver.")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.7
                            }
                        }

                        StyledSwitch {
                            id: turboSwitch
                            enabled: powerMgr.turboSupported && !powerMgr.isBusy
                            checked: powerMgr.isTurboEnabled
                            onToggled: powerMgr.setTurbo(checked)
                        }
                    }

                    Kirigami.Separator { Layout.fillWidth: true }

                    // Frequency Limits Sliders
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: i18n("Frequency Range Limits (Min: %1 MHz, Max: %2 MHz)", Math.round(minFreqSlider.value), Math.round(maxFreqSlider.value))
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.mediumSpacing

                            QQC2.Label {
                                text: i18n("Min Freq:")
                                Layout.preferredWidth: 80
                            }

                            StyledSlider {
                                id: minFreqSlider
                                Layout.fillWidth: true
                                from: powerMgr.minFreqMhz
                                to: Math.min(powerMgr.maxFreqMhz, maxFreqSlider.value)
                                stepSize: 100
                                value: powerMgr.scalingMinFreqMhz
                            }

                            QQC2.Label {
                                text: Math.round(minFreqSlider.value) + " MHz"
                                font.family: "monospace"
                                Layout.preferredWidth: 80
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.mediumSpacing

                            QQC2.Label {
                                text: i18n("Max Freq:")
                                Layout.preferredWidth: 80
                            }

                            StyledSlider {
                                id: maxFreqSlider
                                Layout.fillWidth: true
                                from: Math.max(powerMgr.minFreqMhz, minFreqSlider.value)
                                to: powerMgr.maxFreqMhz
                                stepSize: 100
                                value: powerMgr.scalingMaxFreqMhz
                            }

                            QQC2.Label {
                                text: Math.round(maxFreqSlider.value) + " MHz"
                                font.family: "monospace"
                                Layout.preferredWidth: 80
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            QQC2.Button {
                                text: i18n("Apply Frequency Limits")
                                icon.name: "dialog-ok-apply"
                                enabled: !powerMgr.isBusy
                                onClicked: powerMgr.setFrequencyLimits(Math.round(minFreqSlider.value), Math.round(maxFreqSlider.value))
                            }
                        }
                    }
                }
            }

            // 2. Live Telemetry & Per-Core Telemetry
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "speedometer"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    Kirigami.Heading {
                        level: 3
                        text: i18n("Live Multi-Core Frequency Telemetry")
                        Layout.fillWidth: true
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width > Kirigami.Units.gridUnit * 38 ? 4 : 2
                        rowSpacing: Kirigami.Units.smallSpacing
                        columnSpacing: Kirigami.Units.smallSpacing

                        Repeater {
                            model: powerMgr.coreFrequencies

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 38
                                radius: 4
                                color: "#1e222b"
                                border.color: "#313844"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: Kirigami.Units.smallSpacing

                                    QQC2.Label {
                                        text: i18n("Core %1", index)
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                                        opacity: 0.7
                                    }

                                    Item { Layout.fillWidth: true }

                                    QQC2.Label {
                                        text: modelData + " MHz"
                                        font.bold: true
                                        font.family: "monospace"
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        color: modelData > 3500 ? "#70b9ff" : "#5af78e"
                                    }
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
            visible: powerMgr.isBusy || powerMgr.logOutput.length > 0

            header: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "utilities-terminal"
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                Kirigami.Heading {
                    level: 4
                    text: powerMgr.isBusy ? i18n("Executing: %1", powerMgr.operationStatus) : i18n("Command Log")
                    Layout.fillWidth: true
                }

                QQC2.BusyIndicator {
                    running: powerMgr.isBusy
                    implicitWidth: 20
                    implicitHeight: 20
                }

                QQC2.Button {
                    text: i18n("Clear")
                    icon.name: "edit-clear"
                    enabled: powerMgr.logOutput.length > 0
                    onClicked: powerMgr.clearLogs()
                }
            }

            contentItem: Rectangle {
                Layout.fillWidth: true
                implicitHeight: Kirigami.Units.gridUnit * 12
                color: "#16191e"
                radius: 6
                border.color: "#2e3440"
                border.width: 1
                clip: true

                QQC2.ScrollView {
                    id: powerLogScrollView
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true

                    QQC2.ScrollBar.vertical: StyledScrollBar {
                        parent: powerLogScrollView
                        x: powerLogScrollView.width - width
                        y: 0
                        height: powerLogScrollView.height
                    }

                    QQC2.TextArea {
                        id: powerLogArea
                        readOnly: true
                        text: powerMgr.logOutput
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
