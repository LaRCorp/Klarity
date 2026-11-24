import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Shell Aliases & Functions")

    AliasManager {
        id: aliasMgr

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Add Entry")
            icon.name: "list-add"
            tooltip: i18n("Add new alias or function")
            onTriggered: {
                editDialog.isNew = true
                editDialog.targetId = ""
                editDialog.name = ""
                editDialog.command = ""
                editDialog.shell = "Fish"
                editDialog.type = "Function"
                editDialog.open()
            }
        },
        Kirigami.Action {
            text: i18n("Refresh")
            icon.name: "view-refresh"
            tooltip: i18n("Reload shell aliases and functions")
            onTriggered: aliasMgr.load()
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
        warningText: i18n("Are you sure you want to delete this shell entry?")
        property string targetId: ""
        onConfirmed: {
            aliasMgr.deleteEntry(targetId)
        }
    }

    // Dialog for creating or editing an alias / function
    KirigamiDialogs.Dialog {
        id: editDialog
        property bool isNew: true
        property string targetId: ""
        property alias name: nameInput.text
        property alias command: cmdInput.text
        property string shell: "Fish"
        property string type: "Function"

        title: isNew ? i18n("Create Shell Entry") : i18n("Edit Shell Entry: %1", name)
        preferredWidth: Kirigami.Units.gridUnit * 30
        standardButtons: KirigamiDialogs.Dialog.NoButton

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.mediumSpacing

            RowLayout {
                visible: editDialog.isNew
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Label {
                    text: i18n("Shell:")
                    font.bold: true
                }
                QQC2.ComboBox {
                    id: shellBox
                    model: ["Fish", "Bash", "Zsh"]
                    currentIndex: 0
                    onCurrentTextChanged: editDialog.shell = currentText
                }

                QQC2.Label {
                    text: i18n("Type:")
                    font.bold: true
                }
                QQC2.ComboBox {
                    id: typeBox
                    model: ["Function", "Alias"]
                    currentIndex: 0
                    onCurrentTextChanged: editDialog.type = currentText
                }
            }

            QQC2.Label {
                text: i18n("Identifier / Name:")
                font.bold: true
            }
            QQC2.TextField {
                id: nameInput
                Layout.fillWidth: true
                placeholderText: i18n("e.g. ll or my_backup_function")
            }

            QQC2.Label {
                text: i18n("Command or Function Body:")
                font.bold: true
            }
            QQC2.ScrollView {
                Layout.fillWidth: true
                implicitHeight: Kirigami.Units.gridUnit * 8

                QQC2.TextArea {
                    id: cmdInput
                    font.family: "monospace"
                    wrapMode: Text.WrapAnywhere
                    placeholderText: editDialog.type === "Function"
                                     ? i18n("echo 'Executing...'\nrsync -avz /src /dest")
                                     : i18n("ls -la --color=auto")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Button {
                    text: i18n("Test Syntax")
                    icon.name: "tools-check"
                    onClicked: {
                        const res = aliasMgr.validateSyntax(editDialog.shell, editDialog.type, nameInput.text, cmdInput.text)
                        if (res.valid) {
                            validationLabel.text = i18n("✓ Syntax is valid!")
                            validationLabel.color = Kirigami.Theme.positiveTextColor
                        } else {
                            validationLabel.text = i18n("✗ Error: %1", res.error)
                            validationLabel.color = Kirigami.Theme.negativeTextColor
                        }
                    }
                }

                QQC2.Label {
                    id: validationLabel
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }

        customFooterActions: [
            Kirigami.Action {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onTriggered: editDialog.close()
            },
            Kirigami.Action {
                text: editDialog.isNew ? i18n("Create") : i18n("Save")
                icon.name: "dialog-ok"
                enabled: nameInput.text.trim().length > 0 && cmdInput.text.trim().length > 0
                onTriggered: {
                    if (editDialog.isNew) {
                        const ok = aliasMgr.createEntry(editDialog.shell, editDialog.type, nameInput.text, cmdInput.text)
                        if (ok) editDialog.close()
                    } else {
                        const ok = aliasMgr.saveEntry(editDialog.targetId, nameInput.text, cmdInput.text)
                        if (ok) editDialog.close()
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

        // Search & Filter controls
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18n("Filter aliases and functions...")
            }

            QQC2.ComboBox {
                id: shellFilter
                model: [i18n("All Shells"), "Fish", "Bash", "Zsh"]
                currentIndex: 0
            }

            QQC2.ComboBox {
                id: typeFilter
                model: [i18n("All Types"), "Alias", "Function"]
                currentIndex: 0
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // List of entries
        ListView {
            id: aliasList
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            spacing: Kirigami.Units.smallSpacing

            model: {
                const search = searchField.text.toLowerCase().trim()
                const selShell = shellFilter.currentIndex
                const selType = typeFilter.currentIndex

                return aliasMgr.entries.filter(item => {
                    if (search.length > 0) {
                        const nameMatches = item.name.toLowerCase().includes(search)
                        const cmdMatches = item.command.toLowerCase().includes(search)
                        if (!nameMatches && !cmdMatches) return false
                    }
                    if (selShell === 1 && item.shell !== "Fish") return false
                    if (selShell === 2 && item.shell !== "Bash") return false
                    if (selShell === 3 && item.shell !== "Zsh") return false

                    if (selType === 1 && item.type !== "Alias") return false
                    if (selType === 2 && item.type !== "Function") return false

                    return true
                })
            }

            delegate: Kirigami.Card {
                id: card
                width: aliasList.width

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Heading {
                            level: 4
                            text: modelData.name
                        }

                        // Shell Badge
                        Rectangle {
                            implicitWidth: shellLbl.implicitWidth + Kirigami.Units.smallSpacing * 2
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            radius: height / 2
                            color: {
                                if (modelData.shell === "Fish") return "#287233"
                                if (modelData.shell === "Bash") return "#1b4f72"
                                return "#6c3483"
                            }

                            QQC2.Label {
                                id: shellLbl
                                anchors.centerIn: parent
                                text: modelData.shell
                                color: "white"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                            }
                        }

                        // Type Badge
                        Rectangle {
                            implicitWidth: typeLbl.implicitWidth + Kirigami.Units.smallSpacing * 2
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            radius: height / 2
                            color: Kirigami.Theme.alternateBackgroundColor

                            QQC2.Label {
                                id: typeLbl
                                anchors.centerIn: parent
                                text: modelData.type
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                opacity: 0.8
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        QQC2.ToolButton {
                            icon.name: "document-edit"
                            QQC2.ToolTip.text: i18n("Edit entry")
                            QQC2.ToolTip.visible: hovered
                            onClicked: {
                                editDialog.isNew = false
                                editDialog.targetId = modelData.id
                                editDialog.name = modelData.name
                                editDialog.command = modelData.command
                                editDialog.shell = modelData.shell
                                editDialog.type = modelData.type
                                editDialog.open()
                            }
                        }

                        QQC2.ToolButton {
                            icon.name: "edit-delete"
                            QQC2.ToolTip.text: i18n("Delete entry")
                            QQC2.ToolTip.visible: hovered
                            onClicked: {
                                deleteConfirmDlg.targetId = modelData.id
                                deleteConfirmDlg.open()
                            }
                        }
                    }

                    // Monospace preview
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: Math.min(bodyLabel.implicitHeight + Kirigami.Units.smallSpacing * 2, Kirigami.Units.gridUnit * 6)
                        color: Kirigami.Theme.alternateBackgroundColor
                        radius: Kirigami.Units.smallSpacing
                        clip: true

                        QQC2.Label {
                            id: bodyLabel
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.smallSpacing
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: modelData.command
                            elide: Text.ElideRight
                            wrapMode: Text.WrapAnywhere
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: modelData.filePath + (modelData.lineNumber > 0 ? ":" + modelData.lineNumber : "")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                        opacity: 0.5
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12
            Layout.alignment: Qt.AlignCenter
            visible: aliasList.count === 0
            text: i18n("No shell aliases or functions found")
            icon.name: "utilities-terminal"
        }
    }
}
