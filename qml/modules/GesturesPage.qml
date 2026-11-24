import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Touchpad Gestures")

    GesturesManager {
        id: gesturesMgr

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            } else {
                gestureDialog.close()
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Add Gesture")
            icon.name: "list-add"
            tooltip: i18n("Create a new touchpad swipe or pinch gesture")
            onTriggered: openAddDialog()
        },
        Kirigami.Action {
            text: gesturesMgr.isServiceRunning ? i18n("Service Running") : i18n("Restart Service")
            icon.name: gesturesMgr.isServiceRunning ? "system-run" : "view-refresh"
            tooltip: i18n("Restart libinput-gestures daemon")
            onTriggered: gesturesMgr.restartService()
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
                    source: "input-touchpad"
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
                            text: i18n("libinput-gestures Configuration")
                        }

                        Rectangle {
                            implicitWidth: statusBadgeText.implicitWidth + Kirigami.Units.smallSpacing * 2
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            radius: height / 2
                            color: gesturesMgr.isServiceRunning ? "#1d5c2e" : "#5c1d1d"

                            QQC2.Label {
                                id: statusBadgeText
                                anchors.centerIn: parent
                                text: gesturesMgr.isServiceRunning ? i18n("Active") : i18n("Inactive")
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                color: "#ffffff"
                            }
                        }
                    }

                    QQC2.Label {
                        text: gesturesMgr.configPath
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        color: Kirigami.Theme.disabledTextColor
                    }
                }

                QQC2.Button {
                    text: i18n("Restart Daemon")
                    icon.name: "view-refresh"
                    onClicked: gesturesMgr.restartService()
                }

                QQC2.Button {
                    text: i18n("Add Gesture")
                    icon.name: "list-add"
                    highlighted: true
                    onClicked: openAddDialog()
                }
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // List of Gestures
        ListView {
            id: gesturesList
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            spacing: Kirigami.Units.mediumSpacing

            model: gesturesMgr.gestures

            delegate: Kirigami.Card {
                id: card
                width: gesturesList.width

                contentItem: RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    // Gesture Icon Box
                    Rectangle {
                        implicitWidth: Kirigami.Units.gridUnit * 3
                        implicitHeight: Kirigami.Units.gridUnit * 3
                        radius: Kirigami.Units.smallSpacing
                        color: Kirigami.Theme.alternateBackgroundColor
                        border.color: Kirigami.Theme.focusColor
                        border.width: 1

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 1

                            Kirigami.Icon {
                                source: modelData.icon
                                Layout.alignment: Qt.AlignHCenter
                                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                                implicitHeight: Kirigami.Units.iconSizes.smallMedium
                            }

                            QQC2.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.fingers + "F"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                            }
                        }
                    }

                    // Gesture Description
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Heading {
                                level: 4
                                text: formatGestureTitle(modelData.type, modelData.direction, modelData.fingers)
                            }

                            Rectangle {
                                implicitWidth: typeBadge.implicitWidth + Kirigami.Units.smallSpacing * 2
                                implicitHeight: Kirigami.Units.gridUnit * 1.1
                                radius: height / 2
                                color: Kirigami.Theme.alternateBackgroundColor

                                QQC2.Label {
                                    id: typeBadge
                                    anchors.centerIn: parent
                                    text: modelData.type.toUpperCase()
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                    opacity: 0.8
                                }
                            }
                        }

                        QQC2.Label {
                            text: modelData.friendlyName
                            font.bold: true
                            color: Kirigami.Theme.textColor
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.command
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            color: Kirigami.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }

                    // High Contrast Switch
                    StyledSwitch {
                        checked: modelData.enabled
                        onToggled: gesturesMgr.toggleGesture(modelData.id)
                    }

                    // Edit Button
                    QQC2.Button {
                        icon.name: "document-edit"
                        text: i18n("Edit")
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Edit gesture")
                        onClicked: openEditDialog(modelData)
                    }

                    // Delete Button
                    QQC2.Button {
                        icon.name: "edit-delete"
                        text: i18n("Delete")
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Delete gesture")
                        onClicked: {
                            deleteConfirmDialog.gestureId = modelData.id
                            deleteConfirmDialog.gestureDesc = formatGestureTitle(modelData.type, modelData.direction, modelData.fingers)
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
            visible: gesturesList.count === 0
            text: i18n("No touchpad gestures configured yet")
            icon.name: "input-touchpad"
        }
    }

    // Add / Edit Gesture Dialog
    KirigamiDialogs.Dialog {
        id: gestureDialog
        title: isEditMode ? i18n("Edit Touchpad Gesture") : i18n("Add Touchpad Gesture")
        width: Kirigami.Units.gridUnit * 30
        standardButtons: KirigamiDialogs.Dialog.NoButton

        property bool isEditMode: false
        property string gestureId: ""

        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            width: parent.width

            // Gesture Parameters
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label { text: i18n("Gesture Type:"); font.bold: true }
                    QQC2.ComboBox {
                        id: typeCombo
                        Layout.fillWidth: true
                        model: [i18n("Swipe"), i18n("Pinch")]
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label { text: i18n("Direction:"); font.bold: true }
                    QQC2.ComboBox {
                        id: directionCombo
                        Layout.fillWidth: true
                        model: [
                            i18n("Left"),
                            i18n("Right"),
                            i18n("Up"),
                            i18n("Down"),
                            i18n("In (Pinch in)"),
                            i18n("Out (Pinch out)")
                        ]
                    }
                }

                ColumnLayout {
                    implicitWidth: Kirigami.Units.gridUnit * 7
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label { text: i18n("Fingers:"); font.bold: true }
                    QQC2.SpinBox {
                        id: fingersSpin
                        from: 2
                        to: 5
                        value: 3
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // Action Selection Mode
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: i18n("Assigned Action:")
                    font.bold: true
                }

                RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    QQC2.RadioButton {
                        id: presetRadio
                        text: i18n("Predefined KDE Action")
                        checked: true
                    }

                    QQC2.RadioButton {
                        id: customRadio
                        text: i18n("Custom Command")
                    }
                }

                // Preset selector
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: presetRadio.checked
                    spacing: 4

                    QQC2.ComboBox {
                        id: presetCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        model: gesturesMgr.presets()
                        onCurrentIndexChanged: {
                            if (presetRadio.checked && model && model.length > currentIndex) {
                                cmdField.text = model[currentIndex].command
                            }
                        }
                    }
                }

                // Custom command text field
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    QQC2.Label {
                        text: i18n("Command to execute:")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }

                    QQC2.TextField {
                        id: cmdField
                        Layout.fillWidth: true
                        placeholderText: i18n("e.g. pactl set-sink-volume @DEFAULT_SINK@ +5%")
                        font.family: "monospace"
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
                    onClicked: gestureDialog.close()
                }

                QQC2.Button {
                    text: gestureDialog.isEditMode ? i18n("Save Changes") : i18n("Add Gesture")
                    icon.name: "document-save"
                    highlighted: true
                    enabled: cmdField.text.trim().length > 0
                    onClicked: {
                        const rawType = typeCombo.currentIndex === 0 ? "swipe" : "pinch"
                        const dirMap = ["left", "right", "up", "down", "in", "out"]
                        const rawDir = dirMap[directionCombo.currentIndex]
                        const fingers = fingersSpin.value
                        const cmd = cmdField.text.trim()

                        if (gestureDialog.isEditMode) {
                            gesturesMgr.editGesture(gestureDialog.gestureId, rawType, rawDir, fingers, cmd, true)
                        } else {
                            gesturesMgr.addGesture(rawType, rawDir, fingers, cmd)
                        }
                    }
                }
            }
        }
    }

    // Delete Confirmation Dialog
    ConfirmDialog {
        id: deleteConfirmDialog
        property string gestureId: ""
        property string gestureDesc: ""
        title: i18n("Delete Gesture?")
        message: i18n("Are you sure you want to remove the gesture '%1'?", gestureDesc)
        onConfirmed: {
            gesturesMgr.deleteGesture(gestureId)
        }
    }

    function formatGestureTitle(type, direction, fingers) {
        const typeStr = type === "swipe" ? i18n("Swipe") : i18n("Pinch")
        return i18n("%1 Fingers: %2 %3", fingers, typeStr, direction.toUpperCase())
    }

    function openAddDialog() {
        gestureDialog.isEditMode = false
        gestureDialog.gestureId = ""
        typeCombo.currentIndex = 0
        directionCombo.currentIndex = 0
        fingersSpin.value = 3
        presetRadio.checked = true
        presetCombo.currentIndex = 0
        cmdField.text = (gesturesMgr.presets().length > 0) ? gesturesMgr.presets()[0].command : ""
        gestureDialog.open()
    }

    function openEditDialog(item) {
        gestureDialog.isEditMode = true
        gestureDialog.gestureId = item.id
        typeCombo.currentIndex = item.type === "pinch" ? 1 : 0

        const dirMap = ["left", "right", "up", "down", "in", "out"]
        const idx = dirMap.indexOf(item.direction.toLowerCase())
        directionCombo.currentIndex = (idx >= 0) ? idx : 0

        fingersSpin.value = item.fingers
        cmdField.text = item.command

        // Check if command matches any preset
        let matchPreset = false
        const presets = gesturesMgr.presets()
        for (let i = 0; i < presets.length; ++i) {
            if (presets[i].command.trim() === item.command.trim()) {
                presetCombo.currentIndex = i
                presetRadio.checked = true
                matchPreset = true
                break
            }
        }
        if (!matchPreset) {
            customRadio.checked = true
        }

        gestureDialog.open()
    }
}
