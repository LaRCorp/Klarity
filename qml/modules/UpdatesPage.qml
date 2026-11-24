import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("System Updates")

    UpdateManager {
        id: updateMgr

        onFinished: (exitCode, success) => {
            if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                if (success) {
                    applicationWindow().showPassiveNotification(i18n("Update completed successfully."))
                } else {
                    applicationWindow().showPassiveNotification(i18n("Update finished with errors (exit code: %1).", exitCode))
                }
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Check Updates")
            icon.name: "view-refresh"
            tooltip: i18n("Scan repositories for available package, Flatpak, and AUR updates")
            enabled: !updateMgr.isRunning && !updateMgr.isChecking
            onTriggered: updateMgr.forceCheckUpdates()
        },
        Kirigami.Action {
            text: i18n("Arch News")
            icon.name: "internet-web-browser"
            tooltip: i18n("Open official Arch Linux news announcements in default web browser")
            onTriggered: updateMgr.openArchNews()
        },
        Kirigami.Action {
            text: i18n("Clear Console")
            icon.name: "edit-clear"
            tooltip: i18n("Clear current console log output")
            onTriggered: updateMgr.clearLogs()
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

    ConfirmDialog {
        id: systemUpgradeConfirmDialog
        title: i18n("Confirm Full System Upgrade")
        dangerous: false
        confirmButtonText: i18n("Proceed Upgrade")
        message: i18n("A full system upgrade (pacman -Syu) updates all core packages. Before proceeding, make sure you have checked Arch Linux news announcements for any manual intervention requirements.")
        warningText: i18n("Unattended upgrades without reading Arch News may cause configuration issues or broken dependencies.")

        onConfirmed: {
            updateMgr.runSystemUpgrade()
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        ScanProgress {
            Layout.fillWidth: true
            running: updateMgr.isChecking
            text: i18n("Checking for package updates (Pacman, Flatpak, AUR)...")
        }

        // ================= SECTION 1: STATUS SUMMARY CARD =================
        Kirigami.Card {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: updateMgr.totalUpdatesCount > 0 ? "emblem-warning" : "emblem-checked"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    Kirigami.Heading {
                        level: 3
                        text: i18n("Update Status Overview")
                        Layout.fillWidth: true
                    }

                    QQC2.Button {
                        text: i18n("Arch News")
                        icon.name: "internet-web-browser"
                        onClicked: updateMgr.openArchNews()
                    }

                    QQC2.Button {
                        text: i18n("Check Now")
                        icon.name: "view-refresh"
                        enabled: !updateMgr.isRunning && !updateMgr.isChecking
                        onClicked: updateMgr.forceCheckUpdates()
                    }
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    // Total updates badge
                    Rectangle {
                        implicitWidth: totalLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: totalLabel.implicitHeight + Kirigami.Units.smallSpacing
                        radius: 6
                        color: updateMgr.totalUpdatesCount > 0 ? Kirigami.Theme.neutralBackgroundColor : Kirigami.Theme.positiveBackgroundColor

                        QQC2.Label {
                            id: totalLabel
                            anchors.centerIn: parent
                            font.bold: true
                            text: updateMgr.totalUpdatesCount > 0
                                  ? i18n("%1 updates available", updateMgr.totalUpdatesCount)
                                  : i18n("System is fully up to date")
                            color: updateMgr.totalUpdatesCount > 0 ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.positiveTextColor
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: i18n("Official Pacman: %1  |  Flatpak: %2  |  AUR: %3  •  Checked: %4",
                                   updateMgr.pacmanUpdatesCount,
                                   updateMgr.hasFlatpak ? updateMgr.flatpakUpdatesCount.toString() : i18n("N/A"),
                                   updateMgr.aurHelper.length > 0 ? updateMgr.aurUpdatesCount.toString() : i18n("N/A"),
                                   updateMgr.lastCheckTimeStr)
                        opacity: 0.85
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.7
                    text: i18n("Tip: Always check Arch News before a full system upgrade. For official packages, authentication via Polkit (pkexec) is used for security.")
                }
            }
        }

        // ================= SECTION 2: UPDATE ACTION CARDS =================
        Kirigami.Heading {
            level: 3
            text: i18n("Update Actions")
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.width > Kirigami.Units.gridUnit * 42 ? 2 : 1
            rowSpacing: Kirigami.Units.mediumSpacing
            columnSpacing: Kirigami.Units.mediumSpacing

            // Card 1: System Upgrade
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "system-software-update"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Kirigami.Heading {
                            level: 4
                            text: i18n("System Upgrade")
                        }

                        QQC2.Label {
                            text: "pacman -Syu --noconfirm"
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                            opacity: 0.65
                        }
                    }

                    Rectangle {
                        implicitWidth: pacCountLabel.implicitWidth + 12
                        implicitHeight: pacCountLabel.implicitHeight + 4
                        radius: 4
                        color: updateMgr.pacmanUpdatesCount > 0 ? "#2d4a68" : "#243229"

                        QQC2.Label {
                            id: pacCountLabel
                            anchors.centerIn: parent
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            font.bold: true
                            text: updateMgr.pacmanUpdatesCount > 0 ? i18n("%1 pending", updateMgr.pacmanUpdatesCount) : i18n("Up to date")
                            color: updateMgr.pacmanUpdatesCount > 0 ? "#70b9ff" : "#5af78e"
                        }
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: i18n("Perform a complete synchronization and upgrade of core distribution packages and official repositories via Polkit.")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Button {
                            text: i18n("Upgrade All")
                            icon.name: "system-software-update"
                            enabled: !updateMgr.isRunning && !updateMgr.isChecking
                            onClicked: systemUpgradeConfirmDialog.open()
                        }

                        QQC2.Button {
                            text: i18n("Check News")
                            icon.name: "internet-web-browser"
                            onClicked: updateMgr.openArchNews()
                        }
                    }
                }
            }

            // Card 2: Package Update
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "package-upgrade"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Kirigami.Heading {
                            level: 4
                            text: i18n("Package Update")
                        }

                        QQC2.Label {
                            text: "pacman -Syu --needed --noconfirm"
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                            opacity: 0.65
                        }
                    }

                    Rectangle {
                        implicitWidth: pacCountLabel2.implicitWidth + 12
                        implicitHeight: pacCountLabel2.implicitHeight + 4
                        radius: 4
                        color: updateMgr.pacmanUpdatesCount > 0 ? "#2d4a68" : "#243229"

                        QQC2.Label {
                            id: pacCountLabel2
                            anchors.centerIn: parent
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            font.bold: true
                            text: updateMgr.pacmanUpdatesCount > 0 ? i18n("%1 pending", updateMgr.pacmanUpdatesCount) : i18n("Up to date")
                            color: updateMgr.pacmanUpdatesCount > 0 ? "#70b9ff" : "#5af78e"
                        }
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: i18n("Upgrade outdated repository packages while skipping already up-to-date dependencies.")
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        QQC2.Button {
                            text: i18n("Update Packages")
                            icon.name: "package-upgrade"
                            enabled: !updateMgr.isRunning && !updateMgr.isChecking
                            onClicked: updateMgr.runPackageUpdate()
                        }
                    }
                }
            }

            // Card 3: Flatpak Update
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "flatpak-discover"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Kirigami.Heading {
                            level: 4
                            text: i18n("Flatpak Applications")
                        }

                        QQC2.Label {
                            text: "flatpak update -y"
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                            opacity: 0.65
                        }
                    }

                    Rectangle {
                        implicitWidth: fpCountLabel.implicitWidth + 12
                        implicitHeight: fpCountLabel.implicitHeight + 4
                        radius: 4
                        color: !updateMgr.hasFlatpak ? "#3a2424" : (updateMgr.flatpakUpdatesCount > 0 ? "#2d4a68" : "#243229")

                        QQC2.Label {
                            id: fpCountLabel
                            anchors.centerIn: parent
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            font.bold: true
                            text: !updateMgr.hasFlatpak
                                  ? i18n("Not installed")
                                  : (updateMgr.flatpakUpdatesCount > 0 ? i18n("%1 pending", updateMgr.flatpakUpdatesCount) : i18n("Up to date"))
                            color: !updateMgr.hasFlatpak ? "#ff8585" : (updateMgr.flatpakUpdatesCount > 0 ? "#70b9ff" : "#5af78e")
                        }
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: i18n("Update installed Flatpak applications and shared runtime runtimes without requiring root privileges.")
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        QQC2.Button {
                            text: i18n("Update Flatpak")
                            icon.name: "flatpak-discover"
                            enabled: !updateMgr.isRunning && !updateMgr.isChecking && updateMgr.hasFlatpak
                            onClicked: updateMgr.runFlatpakUpdate()
                        }
                    }
                }
            }

            // Card 4: AUR Update
            Kirigami.Card {
                Layout.fillWidth: true

                header: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "software-properties"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Kirigami.Heading {
                            level: 4
                            text: i18n("AUR Packages")
                        }

                        QQC2.Label {
                            text: updateMgr.aurHelper.length > 0 ? (updateMgr.aurHelper + " -Sua --noconfirm") : i18n("yay/paru not detected")
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                            opacity: 0.65
                        }
                    }

                    Rectangle {
                        implicitWidth: aurCountLabel.implicitWidth + 12
                        implicitHeight: aurCountLabel.implicitHeight + 4
                        radius: 4
                        color: updateMgr.aurHelper.length === 0 ? "#3a2424" : (updateMgr.aurUpdatesCount > 0 ? "#2d4a68" : "#243229")

                        QQC2.Label {
                            id: aurCountLabel
                            anchors.centerIn: parent
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            font.bold: true
                            text: updateMgr.aurHelper.length === 0
                                  ? i18n("No helper")
                                  : (updateMgr.aurUpdatesCount > 0 ? i18n("%1 pending", updateMgr.aurUpdatesCount) : i18n("Up to date"))
                            color: updateMgr.aurHelper.length === 0 ? "#ff8585" : (updateMgr.aurUpdatesCount > 0 ? "#70b9ff" : "#5af78e")
                        }
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: updateMgr.aurHelper.length > 0
                              ? i18n("Update packages installed from Arch User Repository using '%1' helper.", updateMgr.aurHelper)
                              : i18n("No supported AUR helper found (yay or paru). Install yay or paru to manage AUR packages.")
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        QQC2.Button {
                            text: i18n("Update AUR")
                            icon.name: "software-properties"
                            enabled: !updateMgr.isRunning && !updateMgr.isChecking && updateMgr.aurHelper.length > 0
                            onClicked: updateMgr.runAurUpdate()
                        }
                    }
                }
            }
        }

        // ================= SECTION 3: RUNNING OPERATION BANNER =================
        Kirigami.Card {
            Layout.fillWidth: true
            visible: updateMgr.isRunning

            header: RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                QQC2.BusyIndicator {
                    running: updateMgr.isRunning
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Kirigami.Heading {
                        level: 4
                        text: i18n("Executing: %1", updateMgr.currentOperation)
                    }

                    QQC2.Label {
                        text: i18n("Process is actively streaming output below. You can cancel at any time.")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.7
                    }
                }

                QQC2.Button {
                    text: i18n("Cancel Operation")
                    icon.name: "process-stop"
                    onClicked: updateMgr.cancelOperation()
                }
            }
        }

        // ================= SECTION 4: REAL-TIME TERMINAL LOG =================
        Kirigami.Card {
            Layout.fillWidth: true

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
                        level: 3
                        text: i18n("Console Output")
                        Layout.fillWidth: true
                    }

                    QQC2.Button {
                        text: i18n("Copy Terminal")
                        icon.name: "edit-copy"
                        enabled: updateMgr.logOutput.length > 0
                        onClicked: {
                            updateMgr.copyToClipboard(updateMgr.logOutput)
                            if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                                applicationWindow().showPassiveNotification(i18n("Console output copied to clipboard."))
                            }
                        }
                    }

                    QQC2.Button {
                        text: i18n("Clear")
                        icon.name: "edit-clear"
                        enabled: updateMgr.logOutput.length > 0
                        onClicked: updateMgr.clearLogs()
                    }
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 18
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 18
                    color: "#16191e"
                    radius: 6
                    border.color: "#2e3440"
                    border.width: 1
                    clip: true

                    QQC2.ScrollView {
                        id: logScrollView
                        anchors.fill: parent
                        anchors.margins: 4
                        clip: true

                        QQC2.TextArea {
                            id: logTextArea
                            readOnly: true
                            text: updateMgr.logOutput.length > 0
                                  ? updateMgr.logOutput
                                  : i18n("No operations running. Terminal output will stream here in real time during updates...")
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: updateMgr.logOutput.length > 0 ? "#d8dee9" : "#616e85"
                            background: null
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                        }
                    }

                    Connections {
                        target: updateMgr
                        function onOutputReceived(text) {
                            logTextArea.cursorPosition = logTextArea.length
                        }
                    }
                }
            }
        }
    }
}
