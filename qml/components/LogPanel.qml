import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.klarity 1.0

Kirigami.OverlayDrawer {
    id: root

    edge: Qt.RightEdge
    modal: true
    width: Math.min(Kirigami.Units.gridUnit * 28, applicationWindow() ? applicationWindow().width * 0.8 : 500)

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 3
                text: i18n("Execution Logs")
                Layout.fillWidth: true
            }

            QQC2.Button {
                text: i18n("Clear")
                icon.name: "edit-clear"
                onClicked: LogModel.clear()
            }

            QQC2.Button {
                text: i18n("Copy All")
                icon.name: "edit-copy"
                onClicked: {
                    clipboardHelper.copyText(LogModel.allText())
                }
            }

            QQC2.ToolButton {
                icon.name: "window-close"
                onClicked: root.close()
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // Log Entries List
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: LogModel

            ScrollBar.vertical: StyledScrollBar {
                active: true
            }

            onCountChanged: {
                // Auto-scroll to bottom
                listView.positionViewAtEnd()
            }

            delegate: QQC2.ItemDelegate {
                id: itemDelegate
                width: listView.width
                padding: Kirigami.Units.smallSpacing

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: model.timestamp
                        font.family: "monospace"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.6
                    }

                    QQC2.Label {
                        text: "[" + model.source + "]"
                        font.family: "monospace"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: model.text
                        font.family: "monospace"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        wrapMode: Text.WrapAnywhere
                        color: {
                            if (model.level === "error") {
                                return Kirigami.Theme.negativeTextColor
                            } else if (model.level === "warn") {
                                return Kirigami.Theme.neutralTextColor
                            }
                            return Kirigami.Theme.textColor
                        }
                    }
                }
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                visible: listView.count === 0
                text: i18n("No log entries yet")
                icon.name: "text-x-log"
            }
        }
    }

    Item {
        id: clipboardHelper
        function copyText(text) {
            dummyTextEdit.text = text
            dummyTextEdit.selectAll()
            dummyTextEdit.copy()
        }
        TextEdit {
            id: dummyTextEdit
            visible: false
        }
    }
}
