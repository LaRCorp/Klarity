import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs

KirigamiDialogs.Dialog {
    id: root

    property string message: ""
    property alias text: root.message
    property string warningText: ""
    property bool dangerous: false
    property bool requireCheckbox: dangerous
    property string confirmButtonText: i18n("Continue")
    property string cancelButtonText: i18n("Cancel")

    signal confirmed()
    signal cancelled()

    title: dangerous ? i18n("Dangerous Action Confirmation") : i18n("Confirmation")
    preferredWidth: Kirigami.Units.gridUnit * 24

    standardButtons: KirigamiDialogs.Dialog.NoButton

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.dangerous || root.warningText.length > 0
            type: root.dangerous ? Kirigami.MessageType.Error : Kirigami.MessageType.Warning
            text: root.warningText.length > 0 ? root.warningText : i18n("This action might affect system stability. Please proceed with caution.")
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.message
            visible: text.length > 0
        }

        QQC2.CheckBox {
            id: ackCheckBox
            Layout.fillWidth: true
            visible: root.requireCheckbox
            text: i18n("I understand the risks")
            checked: false
        }
    }

    customFooterActions: [
        Kirigami.Action {
            text: root.cancelButtonText
            icon.name: "dialog-cancel"
            onTriggered: {
                root.cancelled()
                root.close()
            }
        },
        Kirigami.Action {
            text: root.confirmButtonText
            icon.name: root.dangerous ? "dialog-warning" : "dialog-ok"
            enabled: !root.requireCheckbox || ackCheckBox.checked
            onTriggered: {
                root.confirmed()
                root.accepted()
                root.close()
            }
        }
    ]

    onClosed: {
        ackCheckBox.checked = false
    }
}
