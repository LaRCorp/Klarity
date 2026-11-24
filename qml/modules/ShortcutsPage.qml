import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Shortcuts & Keybindings")

    ShortcutManager {
        id: shortcutMgr

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            } else {
                reassignDialog.close()
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Refresh")
            icon.name: "view-refresh"
            tooltip: i18n("Reload all shortcuts from configuration files")
            onTriggered: shortcutMgr.loadAsync()
        },
        Kirigami.Action {
            text: shortcutMgr.conflictCount > 0
                  ? i18n("Conflicts (%1)", shortcutMgr.conflictCount)
                  : i18n("No Conflicts")
            icon.name: shortcutMgr.conflictCount > 0 ? "dialog-warning" : "dialog-ok"
            checkable: true
            checked: onlyConflictsCheck.checked
            onTriggered: {
                onlyConflictsCheck.checked = !onlyConflictsCheck.checked
            }
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

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        width: root.width

        ErrorBanner {
            id: errorBanner
            Layout.fillWidth: true
        }

        // High-contrast Conflict Notification Header
        Rectangle {
            id: conflictSummaryBox
            Layout.fillWidth: true
            visible: shortcutMgr.conflictCount > 0
            implicitHeight: conflictSummaryLayout.implicitHeight + Kirigami.Units.mediumSpacing * 2
            radius: Kirigami.Units.smallSpacing
            color: "#4d1414"
            border.color: "#e04f5f"
            border.width: 2

            RowLayout {
                id: conflictSummaryLayout
                anchors.fill: parent
                anchors.margins: Kirigami.Units.mediumSpacing
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    source: "dialog-warning"
                    implicitWidth: Kirigami.Units.iconSizes.large
                    implicitHeight: Kirigami.Units.iconSizes.large
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    QQC2.Label {
                        text: i18n("Detected %1 Shortcut Conflict(s)!", shortcutMgr.conflictCount)
                        font.bold: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                        color: "#ffffff"
                    }

                    QQC2.Label {
                        text: i18n("Multiple system or desktop actions share identical key combinations. Use the 'Reassign' or 'Unbind' buttons on the cards below to resolve collisions.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: "#f0b8b8"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                QQC2.Button {
                    text: onlyConflictsCheck.checked ? i18n("Show All") : i18n("View Conflicts Only")
                    icon.name: "dialog-warning"
                    onClicked: {
                        onlyConflictsCheck.checked = !onlyConflictsCheck.checked
                    }
                }
            }
        }

        // Filter and Search Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18n("Search action, description, or key combo (e.g. Meta+L, Volume, Lock)...")
            }

            QQC2.ComboBox {
                id: sourceFilter
                model: [
                    i18n("All Sources"),
                    "KWin",
                    "Global",
                    "Media",
                    "Session",
                    "Plasma",
                    "Applications"
                ]
                currentIndex: 0
            }

            QQC2.CheckBox {
                id: onlyConflictsCheck
                text: i18n("Only Conflicts")
                checked: false
            }

            QQC2.CheckBox {
                id: hideUnsetCheck
                text: i18n("Hide Unset")
                checked: true
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // List of aggregated shortcuts
        ListView {
            id: shortcutsList
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            spacing: Kirigami.Units.mediumSpacing

            model: {
                const search = searchField.text.toLowerCase().trim()
                const selSource = sourceFilter.currentIndex
                const onlyConf = onlyConflictsCheck.checked
                const hideUnset = hideUnsetCheck.checked

                return shortcutMgr.shortcuts.filter(item => {
                    const isUnset = (item.shortcut.length === 0 || item.shortcut === "none")
                    if (hideUnset && isUnset) return false
                    if (onlyConf && !item.isConflict) return false

                    if (search.length > 0) {
                        const actMatch = item.action.toLowerCase().includes(search)
                        const descMatch = item.description.toLowerCase().includes(search)
                        const keyMatch = item.shortcut.toLowerCase().includes(search)
                        if (!actMatch && !descMatch && !keyMatch) return false
                    }

                    if (selSource === 1 && item.source !== "KWin") return false
                    if (selSource === 2 && item.source !== "Global") return false
                    if (selSource === 3 && item.source !== "Media") return false
                    if (selSource === 4 && item.source !== "Session") return false
                    if (selSource === 5 && item.source !== "Plasma") return false
                    if (selSource === 6 && item.source !== "Applications" && !item.source.startsWith("App:")) return false

                    return true
                })
            }

            delegate: Kirigami.Card {
                id: card
                width: shortcutsList.width

                // Bright highlight border for conflicting cards
                Rectangle {
                    anchors.fill: parent
                    radius: Kirigami.Units.smallSpacing
                    color: "transparent"
                    border.color: modelData.isConflict ? "#e04f5f" : "transparent"
                    border.width: modelData.isConflict ? 2 : 0
                    z: 1
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    RowLayout {
                        Layout.fillWidth: true
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
                                    text: modelData.description.length > 0 ? modelData.description : modelData.action
                                }

                                // Source Badge
                                Rectangle {
                                    implicitWidth: srcLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                    implicitHeight: Kirigami.Units.gridUnit * 1.1
                                    radius: height / 2
                                    color: Kirigami.Theme.alternateBackgroundColor

                                    QQC2.Label {
                                        id: srcLabel
                                        anchors.centerIn: parent
                                        text: modelData.source
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                        opacity: 0.8
                                    }
                                }

                                // Category Badge
                                Rectangle {
                                    implicitWidth: catLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                    implicitHeight: Kirigami.Units.gridUnit * 1.1
                                    radius: height / 2
                                    color: Kirigami.Theme.backgroundColor
                                    border.color: Kirigami.Theme.disabledTextColor
                                    border.width: 1

                                    QQC2.Label {
                                        id: catLabel
                                        anchors.centerIn: parent
                                        text: modelData.category
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                        opacity: 0.7
                                    }
                                }
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: i18n("Identifier: %1", modelData.action)
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                                color: Kirigami.Theme.disabledTextColor
                                elide: Text.ElideRight
                            }
                        }

                        // Keyboard Key Visual Display
                        Rectangle {
                            implicitWidth: Math.max(keyLabel.implicitWidth + Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 5)
                            implicitHeight: Kirigami.Units.gridUnit * 2.2
                            radius: Kirigami.Units.smallSpacing
                            color: modelData.isConflict ? "#521616" : Kirigami.Theme.alternateBackgroundColor
                            border.color: modelData.isConflict ? "#f05a69" : Kirigami.Theme.focusColor
                            border.width: modelData.isConflict ? 2 : 1

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: Kirigami.Units.smallSpacing

                                Kirigami.Icon {
                                    source: "dialog-warning"
                                    visible: modelData.isConflict
                                    implicitWidth: Kirigami.Units.iconSizes.small
                                    implicitHeight: Kirigami.Units.iconSizes.small
                                }

                                QQC2.Label {
                                    id: keyLabel
                                    text: (modelData.shortcut.length > 0 && modelData.shortcut !== "none")
                                          ? modelData.shortcut
                                          : i18n("None")
                                    font.family: "monospace"
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.defaultFont.pointSize
                                    color: modelData.isConflict
                                           ? "#ffffff"
                                           : ((modelData.shortcut.length > 0 && modelData.shortcut !== "none")
                                              ? Kirigami.Theme.textColor
                                              : Kirigami.Theme.disabledTextColor)
                                }
                            }
                        }

                        // Reassign Shortcut Button on card
                        QQC2.Button {
                            text: i18n("Edit")
                            icon.name: "document-edit"
                            display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                            QQC2.ToolTip.visible: hovered
                            QQC2.ToolTip.text: i18n("Reassign shortcut")
                            onClicked: {
                                openReassignDialog(modelData)
                            }
                        }

                        // Clear / Unbind Shortcut Button
                        QQC2.Button {
                            text: i18n("Clear")
                            icon.name: "edit-clear"
                            display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                            QQC2.ToolTip.visible: hovered
                            QQC2.ToolTip.text: i18n("Unbind shortcut")
                            visible: modelData.shortcut.length > 0 && modelData.shortcut !== "none"
                            onClicked: {
                                shortcutMgr.unbindShortcut(modelData.id)
                            }
                        }
                    }

                    // High-contrast, crystal-clear conflict resolution box
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: conflictBoxCol.implicitHeight + Kirigami.Units.mediumSpacing * 2
                        visible: modelData.isConflict
                        color: "#3f1414"
                        border.color: "#e24c4c"
                        border.width: 1.5
                        radius: Kirigami.Units.smallSpacing

                        ColumnLayout {
                            id: conflictBoxCol
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.smallSpacing

                            RowLayout {
                                spacing: Kirigami.Units.smallSpacing

                                Kirigami.Icon {
                                    source: "dialog-warning"
                                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                                    implicitHeight: Kirigami.Units.iconSizes.smallMedium
                                }

                                QQC2.Label {
                                    text: i18n("CONFLICTING KEY COMBINATION: «%1»", modelData.shortcut)
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.defaultFont.pointSize
                                    color: "#ffffff"
                                }
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: i18n("Clashes with: %1", modelData.conflictingWith.join("  |  "))
                                color: "#f8c0c0"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.95
                            }

                            RowLayout {
                                spacing: Kirigami.Units.mediumSpacing

                                QQC2.Button {
                                    text: i18n("Reassign This Shortcut")
                                    icon.name: "document-edit"
                                    highlighted: true
                                    onClicked: {
                                        openReassignDialog(modelData)
                                    }
                                }

                                QQC2.Button {
                                    text: i18n("Unbind (Clear) Shortcut")
                                    icon.name: "edit-clear"
                                    onClicked: {
                                        shortcutMgr.unbindShortcut(modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                visible: shortcutsList.count === 0
                text: i18n("No shortcuts match the current filter")
                icon.name: "preferences-desktop-keyboard-shortcuts"
            }
        }
    }

    // Reassign Shortcut Dialog
    KirigamiDialogs.Dialog {
        id: reassignDialog
        title: i18n("Edit Keyboard Shortcut")
        width: Kirigami.Units.gridUnit * 28
        standardButtons: KirigamiDialogs.Dialog.NoButton

        property string currentId: ""
        property string currentAction: ""
        property string currentSource: ""
        property string currentShortcut: ""

        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            width: parent.width

            ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true

                Kirigami.Heading {
                    level: 4
                    text: reassignDialog.currentAction
                }

                QQC2.Label {
                    text: i18n("Source: %1", reassignDialog.currentSource)
                    opacity: 0.7
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true

                QQC2.Label {
                    text: i18n("Key Combination (e.g. Meta+T, Ctrl+Alt+F, Super+Left):")
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.TextField {
                        id: keyInputField
                        Layout.fillWidth: true
                        placeholderText: i18n("Enter combination or 'none'")
                        font.family: "monospace"
                        font.bold: true
                    }

                    QQC2.Button {
                        text: i18n("Clear (None)")
                        icon.name: "edit-clear"
                        onClicked: {
                            keyInputField.text = "none"
                        }
                    }
                }
            }

            // Live Collision Check Warning Card
            Rectangle {
                id: collisionNotice
                Layout.fillWidth: true
                implicitHeight: colNoticeLayout.implicitHeight + Kirigami.Units.mediumSpacing * 2
                radius: Kirigami.Units.smallSpacing

                readonly property string collisionTarget: shortcutMgr.checkCollision(keyInputField.text, reassignDialog.currentId)
                readonly property bool hasCollision: collisionTarget.length > 0

                visible: keyInputField.text.trim().length > 0 && keyInputField.text.trim().toLowerCase() !== "none"
                color: hasCollision ? "#4d1414" : "#143a1a"
                border.color: hasCollision ? "#e04f5f" : "#2ca64f"
                border.width: 1.5

                RowLayout {
                    id: colNoticeLayout
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.mediumSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: collisionNotice.hasCollision ? "dialog-warning" : "dialog-ok"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: collisionNotice.hasCollision
                              ? i18n("Warning: Combination is already in use by: %1", collisionNotice.collisionTarget)
                              : i18n("Combination is available.")
                        color: collisionNotice.hasCollision ? "#ffffff" : "#a8f5ba"
                        font.bold: true
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                Item { Layout.fillWidth: true }

                QQC2.Button {
                    text: i18n("Cancel")
                    icon.name: "dialog-cancel"
                    onClicked: reassignDialog.close()
                }

                QQC2.Button {
                    text: i18n("Save Shortcut")
                    icon.name: "document-save"
                    highlighted: true
                    enabled: keyInputField.text.trim().length > 0
                    onClicked: {
                        shortcutMgr.reassignShortcut(reassignDialog.currentId, keyInputField.text.trim())
                    }
                }
            }
        }
    }

    function openReassignDialog(item) {
        reassignDialog.currentId = item.id
        reassignDialog.currentAction = item.description.length > 0 ? item.description : item.action
        reassignDialog.currentSource = item.source
        reassignDialog.currentShortcut = item.shortcut
        keyInputField.text = (item.shortcut.length > 0 && item.shortcut !== "none") ? item.shortcut : ""
        reassignDialog.open()
    }
}
