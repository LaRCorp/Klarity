import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("System Overview & Reference Template")

    actions: [
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

    DependencyChecker {
        id: depChecker
    }

    ProcessRunner {
        id: asyncRunner

        onOutputChunk: (chunk) => {
            console.log("Async chunk:", chunk)
        }

        onErrorChunk: (chunk) => {
            errorBanner.showError(i18n("Process reported warning: %1", chunk))
        }

        onFinished: (exitCode, output) => {
            scanProgress.running = false
            if (exitCode === 0) {
                lastOutputLabel.text = output.trim()
            } else {
                errorBanner.showError(i18n("Process failed with exit code %1", exitCode))
            }
        }
    }

    ConfirmDialog {
        id: confirmDlg
        dangerous: false
        message: i18n("This is a demonstration of the standard ConfirmDialog component.")
        onConfirmed: {
            LogModel.append("Placeholder", "info", "User confirmed action in ConfirmDialog")
        }
        onCancelled: {
            LogModel.append("Placeholder", "info", "User cancelled action in ConfirmDialog")
        }
    }

    ConfirmDialog {
        id: dangerousConfirmDlg
        dangerous: true
        warningText: i18n("DANGER: This action cannot be easily undone and requires manual confirmation.")
        message: i18n("Are you sure you want to proceed with this destructive action?")
        onConfirmed: {
            LogModel.append("Placeholder", "warn", "User confirmed DANGEROUS action!")
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        ErrorBanner {
            id: errorBanner
            Layout.fillWidth: true
        }

        // Section: Module Header
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 2
                text: i18n("Klarity Core Architecture Template")
            }

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("This module serves as the golden standard for all future Klarity modules. It demonstrates asynchronous command execution, dependency validation, standard dialogs, and logging.")
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // Section: Async Execution & Progress Demo
        Kirigami.Heading {
            level: 3
            text: i18n("Async Process Execution & Progress")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                text: i18n("Run Quick Check ('uname -a')")
                icon.name: "system-run"
                enabled: !asyncRunner.isRunning
                onClicked: {
                    errorBanner.clear()
                    scanProgress.statusText = i18n("Running uname check...")
                    scanProgress.running = true
                    asyncRunner.runAsync("uname", ["-a"])
                }
            }

            QQC2.Button {
                text: i18n("Run Long Async Task ('sleep 3')")
                icon.name: "chronometer"
                enabled: !asyncRunner.isRunning
                onClicked: {
                    errorBanner.clear()
                    scanProgress.statusText = i18n("Simulating background task (3s)...")
                    scanProgress.running = true
                    asyncRunner.runAsync("sh", ["-c", "for i in 1 2 3; do echo Step $i/3; sleep 1; done; echo Completed!"])
                }
            }

            QQC2.Button {
                text: i18n("Simulate Error Process")
                icon.name: "dialog-warning"
                enabled: !asyncRunner.isRunning
                onClicked: {
                    errorBanner.clear()
                    scanProgress.statusText = i18n("Triggering failing command...")
                    scanProgress.running = true
                    asyncRunner.runAsync("ls", ["/nonexistent_klarity_directory_path"])
                }
            }
        }

        ScanProgress {
            id: scanProgress
            Layout.fillWidth: true
            onCancelled: {
                asyncRunner.cancel()
                running = false
                LogModel.append("Placeholder", "warn", "Async task was cancelled by user")
            }
        }

        QQC2.Label {
            id: lastOutputLabel
            Layout.fillWidth: true
            wrapMode: Text.WrapAnywhere
            font.family: "monospace"
            visible: text.length > 0
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // Section: CardsListView with ModuleCards
        Kirigami.Heading {
            level: 3
            text: i18n("Interactive Module Cards")
        }

        Kirigami.CardsLayout {
            Layout.fillWidth: true

            ModuleCard {
                title: i18n("Safe Action Demonstration")
                iconName: "security-high"
                subtitle: i18n("Tests the standard non-destructive confirmation flow.")
                cardActions: [
                    Kirigami.Action {
                        text: i18n("Trigger Dialog")
                        icon.name: "dialog-ok"
                        onTriggered: confirmDlg.open()
                    }
                ]
            }

            ModuleCard {
                title: i18n("Dangerous Action Demonstration")
                iconName: "security-low"
                subtitle: i18n("Tests the destructive flow with required risk acknowledgment.")
                cardActions: [
                    Kirigami.Action {
                        text: i18n("Trigger Dangerous Flow")
                        icon.name: "dialog-warning"
                        onTriggered: dangerousConfirmDlg.open()
                    }
                ]
            }

            ModuleCard {
                title: i18n("Dependency Validation")
                iconName: "tools-check"
                subtitle: i18n("Status: 'fd' is %1, 'smartctl' is %2",
                               depChecker.hasCommand("fd") ? i18n("installed") : i18n("missing"),
                               depChecker.hasCommand("smartctl") ? i18n("installed") : i18n("missing"))
                cardActions: [
                    Kirigami.Action {
                        text: i18n("Re-check Tools")
                        icon.name: "view-refresh"
                        onTriggered: {
                            missingDepModel.updateMissing()
                        }
                    }
                ]
            }
        }

        // Section: Missing Dependencies Cards
        Kirigami.Heading {
            level: 3
            text: i18n("Dependency Checker Cards")
        }

        ColumnLayout {
            id: missingDepLayout
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                id: depRepeater
                model: missingDepModel.missingList

                delegate: MissingDependencyCard {
                    Layout.fillWidth: true
                    moduleTitle: i18n("Placeholder / System Core")
                    dependencyName: modelData
                    onRecheckRequested: {
                        missingDepModel.updateMissing()
                    }
                }
            }

            QQC2.Label {
                visible: missingDepModel.missingList.length === 0
                text: i18n("All required utilities for this module ('fd', 'smartctl') are installed.")
                color: Kirigami.Theme.positiveTextColor
            }
        }
    }

    QtObject {
        id: missingDepModel
        property var missingList: []

        function updateMissing() {
            missingList = depChecker.missing(["fd", "smartctl"])
        }

        Component.onCompleted: {
            updateMissing()
        }
    }
}
