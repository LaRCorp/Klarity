import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.Card {
    id: root

    property alias title: root.banner.title
    property string subtitle: ""
    property string iconName: ""
    property list<Kirigami.Action> cardActions

    banner.titleIcon: iconName

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.subtitle
            visible: text.length > 0
            color: Kirigami.Theme.disabledTextColor
        }
    }

    actions: cardActions
}
