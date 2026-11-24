import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    property string statusText: i18n("Operation in progress...")
    property alias text: root.statusText
    property bool running: false

    signal cancelled()

    spacing: Kirigami.Units.mediumSpacing
    visible: running

    QQC2.BusyIndicator {
        running: root.running
        implicitWidth: Kirigami.Units.gridUnit * 2
        implicitHeight: Kirigami.Units.gridUnit * 2
    }

    QQC2.Label {
        Layout.fillWidth: true
        text: root.statusText
        elide: Text.ElideRight
    }

    QQC2.Button {
        text: i18n("Cancel")
        icon.name: "process-stop"
        onClicked: {
            root.cancelled()
        }
    }
}
