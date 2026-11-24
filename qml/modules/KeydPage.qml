import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Keyboard Dispatcher (keyd)")

    KeydManager {
        id: keydMgr

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            } else {
                keydDialog.close()
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Add Remapping")
            icon.name: "list-add"
            tooltip: i18n("Create a new key remapping or macro")
            onTriggered: openAddDialog()
        },
        Kirigami.Action {
            text: keydMgr.isServiceActive ? i18n("Daemon Active") : i18n("Reload keyd")
            icon.name: keydMgr.isServiceActive ? "system-run" : "view-refresh"
            tooltip: i18n("Save and reload keyd system daemon")
            onTriggered: keydMgr.saveAndReload()
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

        // Service & Config Info Card
        Kirigami.Card {
            Layout.fillWidth: true

            contentItem: RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Icon {
                    source: "input-keyboard"
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
                            text: i18n("keyd Hardware Remapper")
                        }

                        Rectangle {
                            implicitWidth: statusBadgeText.implicitWidth + Kirigami.Units.smallSpacing * 2
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            radius: height / 2
                            color: keydMgr.isServiceActive ? "#1d5c2e" : "#5c1d1d"

                            QQC2.Label {
                                id: statusBadgeText
                                anchors.centerIn: parent
                                text: keydMgr.isServiceActive ? i18n("Active") : i18n("Inactive")
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                color: "#ffffff"
                            }
                        }
                    }

                    QQC2.Label {
                        text: i18n("Config: %1 (Requires elevated privileges to apply)", keydMgr.configPath)
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        color: Kirigami.Theme.disabledTextColor
                    }
                }

                QQC2.Button {
                    text: i18n("Reload Daemon")
                    icon.name: "view-refresh"
                    onClicked: keydMgr.saveAndReload()
                }

                QQC2.Button {
                    text: i18n("Add Remapping")
                    icon.name: "list-add"
                    highlighted: true
                    onClicked: openAddDialog()
                }
            }
        }

        // Filter / Layer Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18n("Search keys, actions, layers, macros...")
            }

            QQC2.ComboBox {
                id: layerFilter
                model: [i18n("All Layers")].concat(keydMgr.layers)
                currentIndex: 0
            }

            QQC2.CheckBox {
                id: onlyMacrosCheck
                text: i18n("Only Macros")
                checked: false
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // List of Mappings
        ListView {
            id: keydList
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            spacing: Kirigami.Units.mediumSpacing

            model: {
                const search = searchField.text.toLowerCase().trim()
                const selLayer = layerFilter.currentIndex
                const onlyMacros = onlyMacrosCheck.checked

                return keydMgr.mappings.filter(item => {
                    if (onlyMacros && !item.isMacro) return false

                    if (selLayer > 0) {
                        const targetLayer = keydMgr.layers[selLayer - 1]
                        if (item.layer !== targetLayer) return false
                    }

                    if (search.length > 0) {
                        const keyMatch = item.key.toLowerCase().includes(search)
                        const actMatch = item.action.toLowerCase().includes(search)
                        const layerMatch = item.layer.toLowerCase().includes(search)
                        const descMatch = item.description.toLowerCase().includes(search)
                        if (!keyMatch && !actMatch && !layerMatch && !descMatch) return false
                    }

                    return true
                })
            }

            delegate: Kirigami.Card {
                id: card
                width: keydList.width

                contentItem: RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    // Layer Badge
                    Rectangle {
                        implicitWidth: layerLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: Kirigami.Units.gridUnit * 2
                        radius: Kirigami.Units.smallSpacing
                        color: Kirigami.Theme.alternateBackgroundColor
                        border.color: Kirigami.Theme.focusColor
                        border.width: 1

                        QQC2.Label {
                            id: layerLabel
                            anchors.centerIn: parent
                            text: "[" + modelData.layer + "]"
                            font.bold: true
                            font.family: "monospace"
                        }
                    }

                    // Key Remap Visual Flow
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        // Intercepted Key
                        Rectangle {
                            implicitWidth: sourceKeyText.implicitWidth + Kirigami.Units.mediumSpacing * 2
                            implicitHeight: Kirigami.Units.gridUnit * 1.8
                            radius: Kirigami.Units.smallSpacing
                            color: Kirigami.Theme.backgroundColor
                            border.color: Kirigami.Theme.textColor
                            border.width: 1

                            QQC2.Label {
                                id: sourceKeyText
                                anchors.centerIn: parent
                                text: modelData.key
                                font.family: "monospace"
                                font.bold: true
                            }
                        }

                        Kirigami.Icon {
                            source: "go-next"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }

                        // Target Action
                        Rectangle {
                            implicitWidth: targetActionText.implicitWidth + Kirigami.Units.mediumSpacing * 2
                            implicitHeight: Kirigami.Units.gridUnit * 1.8
                            radius: Kirigami.Units.smallSpacing
                            color: modelData.isMacro ? "#3d2252" : "#1f3b52"
                            border.color: modelData.isMacro ? "#8a4fba" : "#3daee9"
                            border.width: 1.5

                            QQC2.Label {
                                id: targetActionText
                                anchors.centerIn: parent
                                text: modelData.action
                                font.family: "monospace"
                                font.bold: true
                                color: "#ffffff"
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        QQC2.Label {
                            text: modelData.description
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: Kirigami.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }

                    // Action buttons
                    QQC2.Button {
                        icon.name: "document-edit"
                        text: i18n("Edit")
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Edit key mapping")
                        onClicked: openEditDialog(modelData)
                    }

                    QQC2.Button {
                        icon.name: "edit-delete"
                        text: i18n("Delete")
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Delete key mapping")
                        onClicked: {
                            deleteConfirmDialog.mappingId = modelData.id
                            deleteConfirmDialog.mappingDesc = modelData.key + " = " + modelData.action
                            deleteConfirmDialog.open()
                        }
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12
            Layout.alignment: Qt.AlignCenter
            visible: keydList.count === 0
            text: i18n("No key remappings configured yet")
            icon.name: "input-keyboard"
        }
    }

    // Add / Edit Keyd Mapping Dialog
    KirigamiDialogs.Dialog {
        id: keydDialog
        title: isEditMode ? i18n("Edit Key Remapping") : i18n("Add Key Remapping")
        width: Kirigami.Units.gridUnit * 30
        standardButtons: KirigamiDialogs.Dialog.NoButton

        property bool isEditMode: false
        property string mappingId: ""

        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            width: parent.width

            // Layer and Key selection
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                ColumnLayout {
                    implicitWidth: Kirigami.Units.gridUnit * 10
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label { text: i18n("Layer:"); font.bold: true }
                    QQC2.ComboBox {
                        id: layerInput
                        Layout.fillWidth: true
                        editable: true
                        model: keydMgr.layers
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label { text: i18n("Key to Intercept:"); font.bold: true }
                    QQC2.TextField {
                        id: keyInputField
                        Layout.fillWidth: true
                        placeholderText: i18n("e.g. capslock, space, f24, rightshift")
                        font.family: "monospace"
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // Action Mode
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label { text: i18n("Action / Behavior:"); font.bold: true }

                RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    QQC2.RadioButton {
                        id: presetRadio
                        text: i18n("Presets (Overload / Swap)")
                        checked: true
                    }

                    QQC2.RadioButton {
                        id: customRadio
                        text: i18n("Custom Action / Macro")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: presetRadio.checked
                    spacing: 4

                    QQC2.ComboBox {
                        id: presetCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        model: keydMgr.presets()
                        onCurrentIndexChanged: {
                            if (presetRadio.checked && model && model.length > currentIndex) {
                                actionInputField.text = model[currentIndex].action
                                if (!keydDialog.isEditMode) {
                                    keyInputField.text = model[currentIndex].key
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    QQC2.Label {
                        text: i18n("Target Action Expression:")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }

                    QQC2.TextField {
                        id: actionInputField
                        Layout.fillWidth: true
                        placeholderText: i18n("e.g. overload(control, esc), rightcontrol, macro(...)")
                        font.family: "monospace"
                    }
                }
            }

            // Preview box
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: previewLabel.implicitHeight + Kirigami.Units.mediumSpacing * 2
                radius: Kirigami.Units.smallSpacing
                color: Kirigami.Theme.alternateBackgroundColor
                border.color: Kirigami.Theme.focusColor
                border.width: 1

                QQC2.Label {
                    id: previewLabel
                    anchors.centerIn: parent
                    text: i18n("Result: [%1] %2 = %3",
                               layerInput.currentText.length > 0 ? layerInput.currentText : "main",
                               keyInputField.text.trim().length > 0 ? keyInputField.text.trim() : "?",
                               actionInputField.text.trim().length > 0 ? actionInputField.text.trim() : "?")
                    font.family: "monospace"
                    font.bold: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                Item { Layout.fillWidth: true }

                QQC2.Button {
                    text: i18n("Cancel")
                    icon.name: "dialog-cancel"
                    onClicked: keydDialog.close()
                }

                QQC2.Button {
                    text: keydDialog.isEditMode ? i18n("Save & Apply") : i18n("Add & Apply")
                    icon.name: "document-save"
                    highlighted: true
                    enabled: keyInputField.text.trim().length > 0 && actionInputField.text.trim().length > 0
                    onClicked: {
                        const layer = layerInput.currentText.trim()
                        const key = keyInputField.text.trim()
                        const action = actionInputField.text.trim()

                        if (keydDialog.isEditMode) {
                            keydMgr.editMapping(keydDialog.mappingId, layer, key, action)
                        } else {
                            keydMgr.addMapping(layer, key, action)
                        }
                    }
                }
            }
        }
    }

    // Delete Confirmation Dialog
    ConfirmDialog {
        id: deleteConfirmDialog
        property string mappingId: ""
        property string mappingDesc: ""
        title: i18n("Delete Key Remapping?")
        message: i18n("Are you sure you want to remove the rule '%1'?", mappingDesc)
        onConfirmed: {
            keydMgr.deleteMapping(mappingId)
        }
    }

    function openAddDialog() {
        keydDialog.isEditMode = false
        keydDialog.mappingId = ""
        layerInput.currentIndex = 0
        presetRadio.checked = true
        presetCombo.currentIndex = 0
        if (keydMgr.presets().length > 0) {
            keyInputField.text = keydMgr.presets()[0].key
            actionInputField.text = keydMgr.presets()[0].action
        } else {
            keyInputField.text = ""
            actionInputField.text = ""
        }
        keydDialog.open()
    }

    function openEditDialog(item) {
        keydDialog.isEditMode = true
        keydDialog.mappingId = item.id

        const layers = keydMgr.layers
        const idx = layers.indexOf(item.layer)
        if (idx >= 0) {
            layerInput.currentIndex = idx
        } else {
            layerInput.editText = item.layer
        }

        keyInputField.text = item.key
        actionInputField.text = item.action

        let matchPreset = false
        const presets = keydMgr.presets()
        for (let i = 0; i < presets.length; ++i) {
            if (presets[i].action.trim() === item.action.trim()) {
                presetCombo.currentIndex = i
                presetRadio.checked = true
                matchPreset = true
                break
            }
        }
        if (!matchPreset) {
            customRadio.checked = true
        }

        keydDialog.open()
    }
}
