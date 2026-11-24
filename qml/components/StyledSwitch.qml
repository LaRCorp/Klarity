import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftPadding + rightPadding,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topPadding + bottomPadding,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: Kirigami.Units.smallSpacing
    spacing: Kirigami.Units.mediumSpacing

    indicator: Rectangle {
        id: track
        implicitWidth: Kirigami.Units.gridUnit * 2.8
        implicitHeight: Kirigami.Units.gridUnit * 1.5
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: height / 2

        // High contrast track: bright blue when checked, distinct light slate (#4e5862) with bright outline (#b2bcc6) when unchecked
        color: control.checked ? "#3daee9" : "#4e5862"
        border.color: control.checked ? "#1d99f3" : "#b2bcc6"
        border.width: 2

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        // Thumb / Slider Handle: pure bright white with sharp dark-tinted edge and drop shadow
        Rectangle {
            id: handle
            width: track.height - 4
            height: track.height - 4
            radius: width / 2
            y: 2
            x: control.checked ? track.width - width - 2 : 2

            color: "#ffffff"
            border.color: control.checked ? "#0064a2" : "#31363b"
            border.width: 1.5

            // Crisp shadow for high visibility on dark backgrounds
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "black"
                opacity: 0.25
                z: -1
                anchors.topMargin: 1
            }

            Behavior on x {
                NumberAnimation { duration: 150; easing.type: Easing.InOutQuad }
            }
        }
    }

    contentItem: QQC2.Label {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: Kirigami.Theme.textColor
        verticalAlignment: Text.AlignVCenter
        visible: text.length > 0
    }
}
