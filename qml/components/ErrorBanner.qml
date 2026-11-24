import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    id: root

    type: Kirigami.MessageType.Error
    showCloseButton: true
    visible: false

    function showError(msg) {
        text = msg
        visible = true
    }

    function clear() {
        text = ""
        visible = false
    }
}
