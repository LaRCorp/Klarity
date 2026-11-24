import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftPadding + rightPadding,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topPadding + bottomPadding,
                             implicitHandleHeight + topPadding + bottomPadding)

    padding: 6

    background: Rectangle {
        readonly property bool isVertical: control.orientation === Qt.Vertical

        x: isVertical ? control.leftPadding + control.availableWidth / 2 - width / 2 : control.leftPadding
        y: isVertical ? control.topPadding : control.topPadding + control.availableHeight / 2 - height / 2

        implicitWidth: isVertical ? 8 : 200
        implicitHeight: isVertical ? 200 : 8
        width: isVertical ? 8 : control.availableWidth
        height: isVertical ? control.availableHeight : 8
        radius: 4
        color: "#4e5862"
        border.color: "#8a95a0"
        border.width: 1.5

        // Filled active track
        Rectangle {
            x: 0
            y: parent.isVertical ? control.visualPosition * parent.height : 0
            width: parent.isVertical ? parent.width : control.visualPosition * parent.width
            height: parent.isVertical ? parent.height - (control.visualPosition * parent.height) : parent.height
            color: "#3daee9"
            radius: 4
        }
    }

    handle: Rectangle {
        readonly property bool isVertical: control.orientation === Qt.Vertical

        x: isVertical
           ? control.leftPadding + control.availableWidth / 2 - width / 2
           : control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: isVertical
           ? control.topPadding + control.visualPosition * (control.availableHeight - height)
           : control.topPadding + control.availableHeight / 2 - height / 2

        implicitWidth: 22
        implicitHeight: 22
        radius: 11
        color: "#ffffff"
        border.color: control.pressed ? "#1d99f3" : "#31363b"
        border.width: 2

        // Glow ring when hovered or pressed
        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 6
            height: parent.height + 6
            radius: width / 2
            color: "#3daee9"
            opacity: control.hovered || control.pressed ? 0.4 : 0
            z: -1
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }
}
