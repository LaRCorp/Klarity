import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.ScrollBar {
    id: control

    readonly property bool __klarityCustom: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding, 16)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding, 16)

    // Position securely on the far right of the scrollable area without locking anchor trees
    x: parent ? parent.width - width : 0
    y: 0
    height: parent ? parent.height : 0
    z: 9999

    padding: 2
    hoverEnabled: true
    active: true
    interactive: true

    // Vertical Track Groove: dark slate with clear high-contrast border
    background: Rectangle {
        implicitWidth: 16
        radius: width / 2
        color: "#252b33"
        border.color: "#525e6b"
        border.width: 1
        opacity: control.size < 1.0 ? 0.95 : 0.0
    }

    // High-Contrast White Thumb / Slider Handle
    contentItem: Rectangle {
        implicitWidth: 12
        radius: width / 2
        // Pure bright white handle, vivid blue when dragged
        color: control.pressed ? "#3daee9" : (control.hovered ? "#ffffff" : "#f5f7fa")
        border.color: control.pressed || control.hovered ? "#1d99f3" : "#232629"
        border.width: 1.5
        opacity: control.size < 1.0 ? 1.0 : 0.0

        // Center grip line indicator for distinct visual clarity
        Rectangle {
            anchors.centerIn: parent
            width: 2
            height: Math.min(parent.height - 8, 20)
            radius: 1
            color: control.pressed ? "#ffffff" : "#8a96a3"
            visible: parent.height > 24
        }
    }
}
