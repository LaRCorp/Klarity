import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("System Auditor")

    SystemAuditor {
        id: auditor

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: auditor.isScanning ? i18n("Scanning...") : i18n("Start Audit")
            icon.name: "system-search"
            enabled: !auditor.isScanning && !auditor.isCleaning
            tooltip: i18n("Scan system for broken links, orphan packages, cache, and leftover configs")
            onTriggered: auditor.startScan()
        },
        Kirigami.Action {
            text: auditor.selectedCount > 0
                  ? i18n("Clean Selected (%1)", auditor.selectedCount)
                  : i18n("Clean Selected")
            icon.name: "edit-clear-all"
            enabled: auditor.selectedCount > 0 && !auditor.isScanning && !auditor.isCleaning
            tooltip: i18n("Delete selected garbage and reclaim disk space")
            onTriggered: openConfirmCleanDialog()
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

        // Summary & Scan Control Card
        Kirigami.Card {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: "security-high"
                        implicitWidth: Kirigami.Units.iconSizes.large
                        implicitHeight: Kirigami.Units.iconSizes.large
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Kirigami.Heading {
                            level: 3
                            text: i18n("System Cleanup & Hygiene Audit")
                        }

                        QQC2.Label {
                            text: i18n("Scan for obsolete package archives, orphan packages, dead symlinks, and leftover application data.")
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }

                    // Total Reclaimable Space Badge
                    Rectangle {
                        implicitWidth: spaceBadgeCol.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: spaceBadgeCol.implicitHeight + Kirigami.Units.mediumSpacing * 2
                        radius: Kirigami.Units.smallSpacing
                        color: auditor.totalReclaimableBytes > 0 ? "#123a1d" : Kirigami.Theme.alternateBackgroundColor
                        border.color: auditor.totalReclaimableBytes > 0 ? "#2ca64f" : Kirigami.Theme.focusColor
                        border.width: 1.5

                        ColumnLayout {
                            id: spaceBadgeCol
                            anchors.centerIn: parent
                            spacing: 1

                            QQC2.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: auditor.totalReclaimableFormatted
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
                                color: auditor.totalReclaimableBytes > 0 ? "#a8f5ba" : Kirigami.Theme.textColor
                            }

                            QQC2.Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: i18n("Reclaimable")
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    QQC2.Button {
                        text: i18n("Start Scan")
                        icon.name: "system-search"
                        highlighted: true
                        enabled: !auditor.isScanning && !auditor.isCleaning
                        onClicked: auditor.startScan()
                    }
                }

                // Progress Indicator
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: auditor.isScanning
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        QQC2.Label {
                            text: auditor.scanStatusText
                            font.bold: true
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                        Item { Layout.fillWidth: true }
                        QQC2.Label {
                            text: auditor.scanProgress + "%"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }

                    QQC2.ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: auditor.scanProgress
                    }
                }
            }
        }

        // Filter and Selection Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18n("Filter audit items...")
            }

            QQC2.ComboBox {
                id: categoryFilter
                model: [
                    i18n("All Categories"),
                    i18n("Broken Symlinks"),
                    i18n("Orphan Packages"),
                    i18n("Package Cache"),
                    i18n("Orphaned Configs"),
                    i18n("Failed Services")
                ]
                currentIndex: 0
            }

            QQC2.ComboBox {
                id: riskFilter
                model: [
                    i18n("All Risk Levels"),
                    i18n("Safe to Remove"),
                    i18n("Review Before Removing")
                ]
                currentIndex: 0
            }

            QQC2.Button {
                text: i18n("Select Safe")
                icon.name: "edit-select-all"
                onClicked: auditor.selectAllSafe()
            }

            QQC2.Button {
                text: i18n("Deselect All")
                icon.name: "edit-select-none"
                onClicked: auditor.deselectAll()
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // List of Audit Items
        ListView {
            id: itemsList
            Layout.fillWidth: true
            implicitHeight: contentHeight
            interactive: false
            spacing: Kirigami.Units.smallSpacing

            model: {
                const search = searchField.text.toLowerCase().trim()
                const catIdx = categoryFilter.currentIndex
                const riskIdx = riskFilter.currentIndex

                return auditor.items.filter(item => {
                    if (catIdx === 1 && item.category !== "symlinks") return false
                    if (catIdx === 2 && item.category !== "orphans") return false
                    if (catIdx === 3 && item.category !== "cache") return false
                    if (catIdx === 4 && item.category !== "configs") return false
                    if (catIdx === 5 && item.category !== "services") return false

                    if (riskIdx === 1 && item.riskLevel !== "safe") return false
                    if (riskIdx === 2 && item.riskLevel !== "review") return false

                    if (search.length > 0) {
                        const titleMatch = item.title.toLowerCase().includes(search)
                        const subMatch = item.subtitle.toLowerCase().includes(search)
                        const pathMatch = item.path.toLowerCase().includes(search)
                        if (!titleMatch && !subMatch && !pathMatch) return false
                    }

                    return true
                })
            }

            delegate: Kirigami.Card {
                id: card
                width: itemsList.width

                contentItem: RowLayout {
                    spacing: Kirigami.Units.mediumSpacing

                    // Checkbox
                    QQC2.CheckBox {
                        checked: modelData.selected
                        onToggled: auditor.setItemSelected(modelData.id, checked)
                    }

                    // Category Icon
                    Kirigami.Icon {
                        source: getCategoryIcon(modelData.category)
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    // Info Column
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Heading {
                                level: 4
                                text: modelData.title
                            }

                            // Risk Badge
                            Rectangle {
                                implicitWidth: riskBadgeText.implicitWidth + Kirigami.Units.mediumSpacing * 2
                                implicitHeight: Kirigami.Units.gridUnit * 1.1
                                radius: height / 2
                                color: modelData.riskLevel === "safe" ? "#1b4d27" : "#543714"
                                border.color: modelData.riskLevel === "safe" ? "#2ca64f" : "#e59020"
                                border.width: 1

                                QQC2.Label {
                                    id: riskBadgeText
                                    anchors.centerIn: parent
                                    text: modelData.riskLevel === "safe" ? i18n("Safe") : i18n("Review")
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                    color: "#ffffff"
                                }
                            }

                            // Category Badge
                            Rectangle {
                                implicitWidth: catBadgeText.implicitWidth + Kirigami.Units.smallSpacing * 2
                                implicitHeight: Kirigami.Units.gridUnit * 1.1
                                radius: height / 2
                                color: Kirigami.Theme.alternateBackgroundColor

                                QQC2.Label {
                                    id: catBadgeText
                                    anchors.centerIn: parent
                                    text: formatCategoryName(modelData.category)
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                    opacity: 0.8
                                }
                            }
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.subtitle
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                        }
                    }

                    // Size Pill
                    Rectangle {
                        implicitWidth: sizeLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: Kirigami.Units.gridUnit * 1.8
                        radius: Kirigami.Units.smallSpacing
                        color: Kirigami.Theme.alternateBackgroundColor
                        border.color: Kirigami.Theme.focusColor
                        border.width: 1

                        QQC2.Label {
                            id: sizeLabel
                            anchors.centerIn: parent
                            text: modelData.formattedSize
                            font.bold: true
                            font.family: "monospace"
                        }
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12
            Layout.alignment: Qt.AlignCenter
            visible: itemsList.count === 0 && !auditor.isScanning
            text: auditor.items.length === 0
                  ? i18n("Click 'Start Scan' to analyze your system for reclaimable space")
                  : i18n("No items match the current filter")
            icon.name: "security-high"
        }
    }

    // Confirmation Dialog before Cleaning
    ConfirmDialog {
        id: confirmCleanDialog
        title: i18n("Clean Selected Items?")
        dangerous: hasReviewItemsSelected()
        message: i18n("You are about to remove %1 item(s). This will reclaim approximately %2 of disk space.",
                      auditor.selectedCount,
                      calculateSelectedSpaceFormatted())
        warningText: hasReviewItemsSelected()
                     ? i18n("Warning: Some selected items require review (e.g. leftover configuration folders). Please ensure you have backups.")
                     : ""
        onConfirmed: {
            auditor.cleanSelected()
        }
    }

    function getCategoryIcon(cat) {
        if (cat === "symlinks") return "folder-symlink"
        if (cat === "orphans") return "package-x-generic"
        if (cat === "cache") return "drive-harddisk"
        if (cat === "configs") return "user-trash"
        if (cat === "services") return "preferences-system-services"
        return "system-run"
    }

    function formatCategoryName(cat) {
        if (cat === "symlinks") return i18n("Symlink")
        if (cat === "orphans") return i18n("Orphan Package")
        if (cat === "cache") return i18n("Cache")
        if (cat === "configs") return i18n("Config Folder")
        if (cat === "services") return i18n("Service")
        return cat
    }

    function hasReviewItemsSelected() {
        for (let i = 0; i < auditor.items.length; ++i) {
            if (auditor.items[i].selected && auditor.items[i].riskLevel === "review") {
                return true
            }
        }
        return false
    }

    function calculateSelectedSpaceFormatted() {
        let total = 0
        for (let i = 0; i < auditor.items.length; ++i) {
            if (auditor.items[i].selected) {
                total += auditor.items[i].sizeBytes
            }
        }
        if (total <= 0) return i18n("0 B")
        const b = total
        if (b < 1024 * 1024) return i18n("%1 KiB", (b / 1024).toFixed(1))
        if (b < 1024 * 1024 * 1024) return i18n("%1 MiB", (b / (1024 * 1024)).toFixed(1))
        return i18n("%1 GiB", (b / (1024 * 1024 * 1024)).toFixed(2))
    }

    function openConfirmCleanDialog() {
        confirmCleanDialog.open()
    }
}
