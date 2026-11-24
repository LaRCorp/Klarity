import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Autostart Management")

    AutostartManager {
        id: autostartMgr

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Add Application")
            icon.name: "list-add"
            tooltip: i18n("Add custom application to autostart")
            onTriggered: addAppDialog.open()
        },
        Kirigami.Action {
            text: i18n("Refresh")
            icon.name: "view-refresh"
            tooltip: i18n("Reload autostart entries")
            onTriggered: autostartMgr.load()
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
        id: deleteConfirmDlg
        dangerous: true
        warningText: i18n("Are you sure you want to remove this autostart entry?")
        property string targetId: ""
        onConfirmed: {
            autostartMgr.removeEntry(targetId)
        }
    }

    // Dialog for adding custom autostart entry
    KirigamiDialogs.Dialog {
        id: addAppDialog
        title: i18n("Add Autostart Application")
        preferredWidth: Kirigami.Units.gridUnit * 25
        standardButtons: KirigamiDialogs.Dialog.NoButton

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.mediumSpacing

            QQC2.Label {
                text: i18n("Application Name:")
                font.bold: true
            }
            QQC2.TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: i18n("e.g. My Script or Web App")
            }

            QQC2.Label {
                text: i18n("Command or Exec Path:")
                font.bold: true
            }
            QQC2.TextField {
                id: cmdField
                Layout.fillWidth: true
                placeholderText: i18n("e.g. /usr/bin/my-app --minimized")
            }

            QQC2.Label {
                text: i18n("Icon Name (optional):")
                font.bold: true
            }
            QQC2.TextField {
                id: iconField
                Layout.fillWidth: true
                text: "system-run"
                placeholderText: i18n("system-run")
            }

            QQC2.Label {
                text: i18n("Description / Comment (optional):")
                font.bold: true
            }
            QQC2.TextField {
                id: commentField
                Layout.fillWidth: true
                placeholderText: i18n("Starts service at login")
            }
        }

        customFooterActions: [
            Kirigami.Action {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onTriggered: addAppDialog.close()
            },
            Kirigami.Action {
                text: i18n("Add to Autostart")
                icon.name: "dialog-ok"
                enabled: nameField.text.trim().length > 0 && cmdField.text.trim().length > 0
                onTriggered: {
                    const ok = autostartMgr.addDesktopEntry(nameField.text, cmdField.text, iconField.text, commentField.text)
                    if (ok) {
                        nameField.text = ""
                        cmdField.text = ""
                        commentField.text = ""
                        addAppDialog.close()
                    }
                }
            }
        ]
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        ErrorBanner {
            id: errorBanner
            Layout.fillWidth: true
        }

        // Filter & Search Controls
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18n("Filter by name or command...")
            }

            QQC2.ComboBox {
                id: typeFilter
                model: [i18n("All Types"), i18n("Desktop"), i18n("Systemd")]
                currentIndex: 0
            }

            QQC2.ComboBox {
                id: scopeFilter
                model: [i18n("All Scopes"), i18n("User"), i18n("System")]
                currentIndex: 0
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // List of Autostart Cards
        ListView {
            id: entriesList
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            spacing: Kirigami.Units.smallSpacing

            model: {
                const search = searchField.text.toLowerCase().trim()
                const selType = typeFilter.currentIndex
                const selScope = scopeFilter.currentIndex

                return autostartMgr.entries.filter(item => {
                    if (search.length > 0) {
                        const nameMatches = item.name.toLowerCase().includes(search)
                        const cmdMatches = item.command.toLowerCase().includes(search)
                        if (!nameMatches && !cmdMatches) return false
                    }
                    if (selType === 1 && item.type !== "Desktop") return false
                    if (selType === 2 && item.type !== "Systemd") return false
                    if (selScope === 1 && item.scope !== "User") return false
                    if (selScope === 2 && item.scope !== "System") return false
                    return true
                })
            }

            delegate: Kirigami.Card {
                id: card
                width: entriesList.width

                contentItem: RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: modelData.icon
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Heading {
                                level: 4
                                text: modelData.name
                            }

                            // Type Chip
                            Rectangle {
                                implicitWidth: typeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                implicitHeight: Kirigami.Units.gridUnit * 1.1
                                radius: height / 2
                                color: modelData.type === "Desktop" ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
                                border.color: Kirigami.Theme.disabledTextColor
                                border.width: 1

                                QQC2.Label {
                                    id: typeLabel
                                    anchors.centerIn: parent
                                    text: modelData.type
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                    color: modelData.type === "Desktop" ? "white" : Kirigami.Theme.textColor
                                }
                            }

                            // Scope Chip
                            Rectangle {
                                implicitWidth: scopeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                implicitHeight: Kirigami.Units.gridUnit * 1.1
                                radius: height / 2
                                color: Kirigami.Theme.alternateBackgroundColor

                                QQC2.Label {
                                    id: scopeLabel
                                    anchors.centerIn: parent
                                    text: modelData.scope
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                    opacity: 0.8
                                }
                            }
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.command
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            elide: Text.ElideRight
                            opacity: 0.7
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.description
                            visible: text.length > 0
                            font.italic: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            opacity: 0.6
                        }
                    }

                    StyledSwitch {
                        checked: modelData.enabled
                        onToggled: {
                            autostartMgr.toggleEntry(modelData.id, checked)
                        }
                    }

                    QQC2.ToolButton {
                        icon.name: "edit-delete"
                        QQC2.ToolTip.text: i18n("Delete entry")
                        QQC2.ToolTip.visible: hovered
                        visible: modelData.scope === "User"
                        onClicked: {
                            deleteConfirmDlg.targetId = modelData.id
                            deleteConfirmDlg.open()
                        }
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12
            Layout.alignment: Qt.AlignCenter
            visible: entriesList.count === 0
            text: i18n("No autostart entries found")
            icon.name: "preferences-system-autostart"
        }
    }
}
