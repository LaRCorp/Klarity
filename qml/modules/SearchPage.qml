import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.klarity 1.0
import "../components"

Kirigami.Page {
    id: root

    title: i18n("Global File Search")

    SearchManager {
        id: searchMgr

        onOperationFinished: (success, message) => {
            if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                applicationWindow().showPassiveNotification(message)
            }
        }
    }

    readonly property var scopeModel: [
        { "id": "home", "name": i18n("Home Directory (~)") },
        { "id": "config", "name": i18n("Configuration (/etc, ~/.config)") },
        { "id": "all", "name": i18n("Entire System (/)") },
        { "id": "apps", "name": i18n("Application Desktops (/usr/share/applications)") },
        { "id": "logs", "name": i18n("System Logs (/var/log)") }
    ]

    readonly property var typeFilterModel: [
        { "id": "all", "name": i18n("All Types") },
        { "id": "file", "name": i18n("Regular Files") },
        { "id": "dir", "name": i18n("Directories") },
        { "id": "symlink", "name": i18n("Symbolic Links") },
        { "id": "exec", "name": i18n("Executables & Scripts") }
    ]

    function triggerSearch(customQuery) {
        const q = (typeof customQuery === "string") ? customQuery : searchField.text
        if (q.trim().length === 0) return

        const scopeId = scopeModel[scopeCombo.currentIndex].id
        const typeId = typeFilterModel[typeCombo.currentIndex].id
        const maxDepth = depthCombo.currentValue

        searchMgr.search(q, scopeId, typeId, maxDepth, searchContentCheck.checked, hiddenFilesCheck.checked)
    }

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        // Row 1: Search input + Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18n("Search files or syntax (e.g. *.service, >10M, type:dir)...")

                onAccepted: {
                    searchMgr.commitSearchHistory(searchField.text)
                    root.triggerSearch()
                }
            }

            QQC2.Button {
                text: searchMgr.isSearching ? i18n("Stop") : i18n("Search")
                icon.name: searchMgr.isSearching ? "process-stop" : "system-search"
                highlighted: !searchMgr.isSearching
                display: root.width > Kirigami.Units.gridUnit * 45 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered && display === QQC2.AbstractButton.IconOnly
                onClicked: {
                    if (searchMgr.isSearching) {
                        searchMgr.cancelSearch()
                    } else {
                        searchMgr.commitSearchHistory(searchField.text)
                        root.triggerSearch()
                    }
                }
            }

            QQC2.Button {
                icon.name: "edit-clear"
                text: i18n("Clear")
                enabled: searchField.text.length > 0 || searchMgr.results.length > 0
                display: root.width > Kirigami.Units.gridUnit * 45 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered && display === QQC2.AbstractButton.IconOnly
                onClicked: {
                    searchField.text = ""
                    searchMgr.cancelSearch()
                    searchMgr.search("")
                }
            }
        }

        // Row 2: Real-time syntax / history suggestions chips
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            visible: searchField.text.length > 0 || searchMgr.searchHistory.length > 0

            QQC2.Label {
                text: searchField.text.length === 0 ? i18n("History:") : i18n("Suggestions:")
                font.bold: true
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                opacity: 0.75
            }

            Flickable {
                Layout.fillWidth: true
                implicitHeight: chipRow.implicitHeight + 4
                contentWidth: chipRow.implicitWidth
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                RowLayout {
                    id: chipRow
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: searchMgr.getSuggestions(searchField.text)

                        delegate: Rectangle {
                            required property string modelData
                            implicitHeight: Kirigami.Units.gridUnit * 1.4
                            implicitWidth: chipLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                            radius: 4
                            color: chipHover.hovered ? "#2980b9" : "#252b33"
                            border.color: chipHover.hovered ? "#3498db" : "#38414a"
                            border.width: 1

                            QQC2.Label {
                                id: chipLabel
                                anchors.centerIn: parent
                                text: modelData
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                color: chipHover.hovered ? "#ffffff" : "#bdc3c7"
                            }

                            HoverHandler {
                                id: chipHover
                            }

                            TapHandler {
                                onTapped: {
                                    searchField.text = modelData
                                    searchMgr.commitSearchHistory(modelData)
                                    root.triggerSearch(modelData)
                                }
                            }
                        }
                    }
                }
            }

            QQC2.Button {
                visible: searchField.text.length === 0 && searchMgr.searchHistory.length > 0
                icon.name: "edit-delete"
                flat: true
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Clear search history")
                onClicked: searchMgr.clearHistory()
            }
        }

        // Row 3: Scope, Type and Max Depth Filters
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            QQC2.Label {
                text: i18n("Location:")
                font.bold: true
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            QQC2.ComboBox {
                id: scopeCombo
                textRole: "name"
                model: root.scopeModel
                currentIndex: 0
                Layout.fillWidth: true
                Layout.minimumWidth: Kirigami.Units.gridUnit * 8
            }

            QQC2.Label {
                text: i18n("Type:")
                font.bold: true
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            QQC2.ComboBox {
                id: typeCombo
                textRole: "name"
                model: root.typeFilterModel
                currentIndex: 0
                implicitWidth: Kirigami.Units.gridUnit * 8
            }

            QQC2.Label {
                text: i18n("Max Depth:")
                font.bold: true
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            QQC2.ComboBox {
                id: depthCombo
                textRole: "text"
                valueRole: "value"
                model: [
                    { "text": i18n("4 Levels"), "value": 4 },
                    { "text": i18n("6 Levels (Default)"), "value": 6 },
                    { "text": i18n("10 Levels"), "value": 10 },
                    { "text": i18n("Unlimited"), "value": 0 }
                ]
                currentIndex: 1
                implicitWidth: Kirigami.Units.gridUnit * 8
            }
        }

        // Row 4: Flags & Real-time Status Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            QQC2.CheckBox {
                id: hiddenFilesCheck
                text: i18n("Hidden Files")
                checked: false
            }

            QQC2.CheckBox {
                id: searchContentCheck
                text: i18n("Search in Content (rg)")
                checked: false
                enabled: searchMgr.hasRg
                QQC2.ToolTip.visible: hovered && !searchMgr.hasRg
                QQC2.ToolTip.text: i18n("ripgrep (rg) is required for content search")
            }

            Kirigami.Separator {
                Layout.fillHeight: true
            }

            QQC2.BusyIndicator {
                running: searchMgr.isSearching
                visible: searchMgr.isSearching
                implicitWidth: Kirigami.Units.gridUnit * 1.2
                implicitHeight: Kirigami.Units.gridUnit * 1.2
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: searchMgr.statusMessage.length > 0 ? searchMgr.statusMessage : i18n("Ready to search.")
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                opacity: 0.8
                elide: Text.ElideRight
            }

            // Engine Badge
            Rectangle {
                implicitHeight: Kirigami.Units.gridUnit * 1.3
                implicitWidth: engineLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                radius: 4
                color: "#16283a"
                border.color: "#1d99f3"
                border.width: 1

                QQC2.Label {
                    id: engineLabel
                    anchors.centerIn: parent
                    text: searchContentCheck.checked ? "ripgrep" : (searchMgr.hasFd ? "fd" : "find")
                    color: "#3daee9"
                    font.bold: true
                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                }
            }

            // Results count badge
            Rectangle {
                visible: searchMgr.resultCount > 0
                implicitHeight: Kirigami.Units.gridUnit * 1.3
                implicitWidth: countBadgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                radius: 4
                color: "#15332f"
                border.color: "#16a085"
                border.width: 1

                QQC2.Label {
                    id: countBadgeLabel
                    anchors.centerIn: parent
                    text: i18n("%1 items", searchMgr.resultCount)
                    color: "#1abc9c"
                    font.bold: true
                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                }
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }
    }

    // ================= MAIN RESULTS LIST =================
    ListView {
        id: resultsListView
        anchors.fill: parent
        clip: true
        reuseItems: true
        spacing: Kirigami.Units.smallSpacing
        model: searchMgr.results

        QQC2.ScrollBar.vertical: StyledScrollBar {}

        // Empty state view
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: searchMgr.results.length === 0 && !searchMgr.isSearching
            icon.name: "system-search"
            text: i18n("Quick System-Wide File Search")
            explanation: i18n("Search anywhere on your system with blazing speed using fd and ripgrep.\n\nTips:\n• *.service — find systemd service units\n• > 100M — find large files over 100 megabytes\n• type:dir — search for directories only\n• Toggle 'Search in Content' to find text inside files")
        }

        delegate: Rectangle {
            required property var modelData

            width: parent ? parent.width - 28 : (resultsListView.width - 28)
            anchors.left: parent ? parent.left : undefined
            anchors.leftMargin: Kirigami.Units.smallSpacing
            height: 64
            radius: Kirigami.Units.smallSpacing
            color: itemHover.hovered ? "#2b323c" : "#252b33"
            border.color: itemHover.hovered ? "#1d99f3" : "#38414a"
            border.width: 1

            HoverHandler {
                id: itemHover
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.mediumSpacing
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Icon {
                    source: modelData.icon && modelData.icon.length > 0 ? modelData.icon : "text-plain"
                    implicitWidth: 34
                    implicitHeight: 34
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: modelData.name
                            font.bold: true
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.05
                            color: itemHover.hovered ? "#3daee9" : Kirigami.Theme.textColor
                            elide: Text.ElideRight
                        }

                        // Type Badge
                        Rectangle {
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            implicitWidth: typeBadgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 1.5
                            radius: 3
                            color: {
                                if (modelData.isDir) return "#2980b9"
                                if (modelData.isExecutable) return "#27ae60"
                                if (modelData.isSymLink) return "#8e44ad"
                                return "#34495e"
                            }
                            QQC2.Label {
                                id: typeBadgeLabel
                                anchors.centerIn: parent
                                text: modelData.typeName
                                font.bold: true
                                color: "#ffffff"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.75
                            }
                        }

                        // Size badge
                        Rectangle {
                            visible: !modelData.isDir && modelData.size !== "-"
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            implicitWidth: sizeLabel.implicitWidth + Kirigami.Units.smallSpacing * 1.5
                            radius: 3
                            color: "#343d46"
                            QQC2.Label {
                                id: sizeLabel
                                anchors.centerIn: parent
                                text: modelData.size
                                color: "#d0d7de"
                                font.family: "monospace"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.75
                            }
                        }

                        // Hidden Badge
                        Rectangle {
                            visible: modelData.isHidden
                            implicitHeight: Kirigami.Units.gridUnit * 1.1
                            implicitWidth: hiddenLabel.implicitWidth + Kirigami.Units.smallSpacing * 1.5
                            radius: 3
                            color: "#57606f"
                            QQC2.Label {
                                id: hiddenLabel
                                anchors.centerIn: parent
                                text: i18n("Hidden")
                                color: "#ffffff"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.75
                            }
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: modelData.path
                        font.family: "monospace"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                        opacity: 0.7
                        elide: Text.ElideMiddle
                    }
                }

                // Action Buttons
                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Button {
                        text: i18n("Open")
                        icon.name: "document-open"
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Open file with default system application")
                        onClicked: searchMgr.openFile(modelData.path)
                    }

                    QQC2.Button {
                        text: i18n("In Dolphin")
                        icon.name: "system-file-manager"
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Open containing folder and select file in Dolphin")
                        onClicked: searchMgr.openContainingFolder(modelData.path)
                    }

                    QQC2.Button {
                        icon.name: "edit-copy"
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Copy full path to clipboard")
                        onClicked: searchMgr.copyToClipboard(modelData.path)
                    }
                }
            }
        }
    }
}
