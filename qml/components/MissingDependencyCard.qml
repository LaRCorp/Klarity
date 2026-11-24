import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.klarity 1.0

Kirigami.Card {
    id: root

    property string moduleTitle: ""
    property string dependencyName: ""
    property string installCommand: ""

    signal recheckRequested()

    banner.title: i18n("Missing Required Tool: %1", root.dependencyName)

    DependencyChecker {
        id: depChecker
    }

    PkexecRunner {
        id: pkRunner

        onFinished: (exitCode, output) => {
            installIndicator.running = false
            if (exitCode === 0) {
                root.recheckRequested()
            }
        }

        onCancelled: {
            installIndicator.running = false
        }

        onErrorOccurred: (reason) => {
            installIndicator.running = false
        }
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("The module '%1' requires '%2' to function properly.", root.moduleTitle, root.dependencyName)
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.bold: true
            text: i18n("Recommended package install command:")
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: cmdLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
            color: Kirigami.Theme.alternateBackgroundColor
            radius: Kirigami.Units.smallSpacing

            Kirigami.SelectableLabel {
                id: cmdLabel
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                font.family: "monospace"
                text: root.installCommand.length > 0 ? root.installCommand : depChecker.installHint(root.dependencyName)
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: cmdLabel.text.startsWith("AUR:")
            font.italic: true
            color: Kirigami.Theme.neutralTextColor
            text: i18n("This package is in the AUR. Please build or install it manually using your AUR helper (e.g., yay or paru).")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.BusyIndicator {
                id: installIndicator
                running: pkRunner.isRunning
                visible: running
                implicitWidth: Kirigami.Units.gridUnit * 1.5
                implicitHeight: Kirigami.Units.gridUnit * 1.5
            }

            QQC2.Button {
                text: i18n("Install with pacman")
                icon.name: "system-software-install"
                enabled: !pkRunner.isRunning && !cmdLabel.text.startsWith("AUR:")
                onClicked: {
                    installIndicator.running = true
                    pkRunner.runRoot("pacman", ["-S", "--noconfirm", root.dependencyName])
                }
            }

            QQC2.Button {
                text: i18n("Re-check")
                icon.name: "view-refresh"
                enabled: !pkRunner.isRunning
                onClicked: {
                    root.recheckRequested()
                }
            }
        }
    }
}
