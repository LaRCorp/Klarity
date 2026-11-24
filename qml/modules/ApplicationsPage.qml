import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as QD
import org.kde.kirigami as Kirigami
import org.klarity 1.0
import "../components"

Kirigami.Page {
    id: root

    title: i18n("Applications & Packages")

    PackageManager {
        id: packageMgr
        onOperationFinished: (success, message) => {
            if (success) {
                if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                    applicationWindow().showPassiveNotification(message)
                }
            } else {
                errorMessage = message
                errorBanner.showError(message)
            }
        }
    }

    PortableAppsManager {
        id: portableMgr
        onOperationFinished: (success, message) => {
            if (success) {
                if (typeof applicationWindow !== "undefined" && applicationWindow()) {
                    applicationWindow().showPassiveNotification(message)
                }
            } else {
                errorMessage = message
                errorBanner.showError(message)
            }
        }
    }

    property string errorMessage: ""
    property var selectedPackageDetails: null
    property string pendingDeletePath: ""
    property string pendingDeleteName: ""
    property string pendingDeleteSource: ""

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.TabBar {
            id: appTabBar
            Layout.fillWidth: true

            QQC2.TabButton {
                text: i18n("Applications (%1)", packageMgr.guiCount + portableMgr.count)
                icon.name: "applications-other"
            }

            QQC2.TabButton {
                text: i18n("Portable Apps (%1)", portableMgr.count)
                icon.name: "application-x-executable"
            }

            QQC2.TabButton {
                text: i18n("System Packages (%1)", packageMgr.totalCount)
                icon.name: "system-software-install"
            }
        }

        // TOOLBAR TAB 0: USER APPLICATIONS
        ColumnLayout {
            Layout.fillWidth: true
            visible: appTabBar.currentIndex === 0
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.SearchField {
                    id: userAppsSearchField
                    Layout.fillWidth: true
                    placeholderText: i18n("Search applications by name or description...")
                }

                QQC2.ComboBox {
                    id: userAppsCategoryCombo
                    model: [
                        i18n("All Categories"),
                        i18n("Internet"),
                        i18n("Development"),
                        i18n("Graphics"),
                        i18n("Multimedia"),
                        i18n("Office"),
                        i18n("Games"),
                        i18n("System"),
                        i18n("Utilities")
                    ]
                    implicitWidth: Kirigami.Units.gridUnit * 8
                }

                QQC2.ComboBox {
                    id: userAppsTypeCombo
                    model: [
                        i18n("User Installed (Explicit)"),
                        i18n("All GUI Applications"),
                        i18n("System / Desktop Components")
                    ]
                    implicitWidth: Kirigami.Units.gridUnit * 10
                }

                QQC2.ComboBox {
                    id: userAppsSourceCombo
                    model: [i18n("All Sources"), "Pacman", "AUR", "Flatpak", "Portable"]
                    implicitWidth: Kirigami.Units.gridUnit * 7
                }

                QQC2.Button {
                    icon.name: "view-refresh"
                    text: i18n("Reload")
                    display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: i18n("Force refresh applications cache")
                    onClicked: {
                        packageMgr.loadPackages(true)
                        portableMgr.scanPortableApps(true)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Rectangle {
                    color: "#16283a"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#1d99f3"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: userTotalLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: userTotalLabel
                        anchors.centerIn: parent
                        text: i18n("User Apps: %1", packageMgr.guiCount + portableMgr.count)
                        color: "#3daee9"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Rectangle {
                    color: "#15332f"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#16a085"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: userFlatpakLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: userFlatpakLabel
                        anchors.centerIn: parent
                        text: i18n("Flatpak: %1", packageMgr.flatpakCount)
                        color: "#1abc9c"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Rectangle {
                    color: "#2a1c38"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#9b59b6"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: userPortableLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: userPortableLabel
                        anchors.centerIn: parent
                        text: i18n("Portable: %1", portableMgr.count)
                        color: "#c39bd3"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Item { Layout.fillWidth: true }

                QQC2.Label {
                    text: i18n("Showing %1 application(s)", root.filteredUserApps.length)
                    opacity: 0.7
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
            }
        }

        // TOOLBAR TAB 1: PORTABLE APPS
        ColumnLayout {
            Layout.fillWidth: true
            visible: appTabBar.currentIndex === 1
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.SearchField {
                    id: portableSearchField
                    Layout.fillWidth: true
                    placeholderText: i18n("Search portable apps by name or path...")
                }

                QQC2.ComboBox {
                    id: portableTypeCombo
                    model: [i18n("All Types"), "AppImage", i18n("ELF Binaries")]
                    implicitWidth: Kirigami.Units.gridUnit * 7
                }

                QQC2.ComboBox {
                    id: portableMenuCombo
                    model: [i18n("All Status"), i18n("In Desktop Menu"), i18n("Not in Menu")]
                    implicitWidth: Kirigami.Units.gridUnit * 8
                }

                QQC2.ComboBox {
                    id: portableLocationCombo
                    model: [i18n("All Locations"), i18n("~/Applications"), i18n("~/.local/bin"), "/opt"]
                    implicitWidth: Kirigami.Units.gridUnit * 8
                }

                QQC2.Button {
                    text: i18n("Add File...")
                    icon.name: "list-add"
                    display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                    onClicked: fileDialog.open()
                }

                QQC2.Button {
                    text: i18n("Scan")
                    icon.name: "view-refresh"
                    display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: i18n("Rescan filesystem for portable AppImages and binaries")
                    onClicked: portableMgr.scanPortableApps(true)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Rectangle {
                    color: "#2a1c38"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#9b59b6"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: portStatsLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: portStatsLabel
                        anchors.centerIn: parent
                        text: i18n("Portable Apps: %1", portableMgr.count)
                        color: "#c39bd3"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Item { Layout.fillWidth: true }

                QQC2.Label {
                    text: i18n("Showing %1 portable app(s)", root.filteredPortableApps.length)
                    opacity: 0.7
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
            }
        }

        // TOOLBAR TAB 2: SYSTEM PACKAGES & LIBRARIES
        ColumnLayout {
            Layout.fillWidth: true
            visible: appTabBar.currentIndex === 2
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.SearchField {
                    id: systemSearchField
                    Layout.fillWidth: true
                    placeholderText: i18n("Search all packages, libraries and tools...")
                }

                QQC2.ComboBox {
                    id: systemCategoryCombo
                    model: [
                        i18n("Explicitly Installed (pacman -Qe)"),
                        i18n("Dependencies & Libraries (pacman -Qd)"),
                        i18n("All System Packages"),
                        "AUR"
                    ]
                    implicitWidth: Kirigami.Units.gridUnit * 12
                }

                QQC2.ComboBox {
                    id: systemSourceCombo
                    model: [i18n("All Sources"), "Pacman", "AUR", "Flatpak", "Snap"]
                    implicitWidth: Kirigami.Units.gridUnit * 7
                }

                QQC2.Button {
                    text: i18n("Check Updates")
                    icon.name: "view-refresh"
                    display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                    onClicked: packageMgr.checkUpdates()
                }

                QQC2.Button {
                    icon.name: "view-refresh"
                    text: i18n("Reload")
                    display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                    onClicked: packageMgr.loadPackages(true)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Rectangle {
                    color: "#252b33"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#38414a"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: sysTotalLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: sysTotalLabel
                        anchors.centerIn: parent
                        text: i18n("Total Packages: %1", packageMgr.totalCount)
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Rectangle {
                    color: "#16283a"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#1d99f3"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: sysExplicitLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: sysExplicitLabel
                        anchors.centerIn: parent
                        text: i18n("Explicit: %1", packageMgr.explicitCount)
                        color: "#3daee9"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Rectangle {
                    color: "#2b2a1a"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#d4ac0d"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: sysDepsLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: sysDepsLabel
                        anchors.centerIn: parent
                        text: i18n("Dependencies: %1", packageMgr.totalCount - packageMgr.explicitCount)
                        color: "#f1c40f"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Rectangle {
                    visible: packageMgr.updateCount > 0
                    color: "#3a2512"
                    radius: Kirigami.Units.smallSpacing
                    border.color: "#e67e22"
                    border.width: 1
                    implicitHeight: Kirigami.Units.gridUnit * 1.5
                    implicitWidth: sysUpdateLabel.implicitWidth + Kirigami.Units.mediumSpacing * 2
                    QQC2.Label {
                        id: sysUpdateLabel
                        anchors.centerIn: parent
                        text: i18n("Updates: %1", packageMgr.updateCount)
                        color: "#f39c12"
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                }

                Item { Layout.fillWidth: true }

                QQC2.Label {
                    text: i18n("Showing %1 package(s)", root.filteredSystemPackages.length)
                    opacity: 0.7
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
            }
        }
    }

    // ================= FILTER MODELS =================

    // 1. User Applications Model (GUI apps + Flatpaks + Portable Apps)
    readonly property var filteredUserApps: {
        const search = userAppsSearchField.text.trim().toLowerCase()
        const src = userAppsSourceCombo.currentText
        const typeIdx = userAppsTypeCombo ? userAppsTypeCombo.currentIndex : 0
        const catIdx = userAppsCategoryCombo ? userAppsCategoryCombo.currentIndex : 0
        const categories = ["", "Internet", "Development", "Graphics", "Multimedia", "Office", "Games", "System", "Utilities"]
        const targetCategory = (catIdx > 0 && catIdx < categories.length) ? categories[catIdx] : ""
        const pkgs = packageMgr.packages || []
        const ports = portableMgr.apps || []

        let list = []

        // A. Desktop GUI Packages (Pacman, AUR, Flatpak)
        for (let i = 0; i < pkgs.length; ++i) {
            const p = pkgs[i]
            if (!p.isGui) continue
            // 0: User Installed (Explicit only)
            // 1: All GUI applications (both explicit and system)
            // 2: System / Desktop Components (!p.isExplicit)
            if (typeIdx === 0 && !p.isExplicit) continue
            if (typeIdx === 2 && p.isExplicit) continue
            if (src !== i18n("All") && src !== i18n("All Sources") && p.source !== src) continue
            if (targetCategory.length > 0 && p.category && p.category !== targetCategory) continue
            if (search.length > 0) {
                const nameMatch = p.name.toLowerCase().indexOf(search) !== -1
                const descMatch = (p.description || "").toLowerCase().indexOf(search) !== -1
                if (!nameMatch && !descMatch) continue
            }
            list.push({
                "id": p.id,
                "name": p.name,
                "version": p.version,
                "source": p.source,
                "size": p.size,
                "description": p.description,
                "icon": p.icon,
                "category": p.category,
                "isPortable": false,
                "hasUpdate": p.hasUpdate
            })
        }

        // B. Portable Applications (only when looking at User Installed or All GUI)
        if ((typeIdx === 0 || typeIdx === 1) && (src === i18n("All") || src === i18n("All Sources") || src === "Portable")) {
            if (targetCategory.length === 0 || targetCategory === "Utilities") {
                for (let j = 0; j < ports.length; ++j) {
                    const a = ports[j]
                    if (search.length > 0) {
                        const nameMatch = a.name.toLowerCase().indexOf(search) !== -1
                        const pathMatch = a.path.toLowerCase().indexOf(search) !== -1
                        if (!nameMatch && !pathMatch) continue
                    }
                    list.push({
                        "id": a.id,
                        "name": a.name,
                        "version": a.isAppImage ? "AppImage" : "Binary",
                        "source": "Portable",
                        "size": a.size,
                        "description": a.path,
                        "icon": a.icon,
                        "path": a.path,
                        "category": "Utilities",
                        "isPortable": true,
                        "hasUpdate": false
                    })
                }
            }
        }

        return list
    }

    // 2. Portable Apps Model
    readonly property var filteredPortableApps: {
        const search = portableSearchField.text.trim().toLowerCase()
        const typeIdx = portableTypeCombo ? portableTypeCombo.currentIndex : 0
        const menuIdx = portableMenuCombo ? portableMenuCombo.currentIndex : 0
        const locIdx = portableLocationCombo ? portableLocationCombo.currentIndex : 0
        const list = portableMgr.apps || []

        return list.filter(app => {
            if (typeIdx === 1 && !app.isAppImage) return false
            if (typeIdx === 2 && app.isAppImage) return false

            if (menuIdx === 1 && !app.isInMenu) return false
            if (menuIdx === 2 && app.isInMenu) return false

            if (locIdx === 1 && !app.path.includes("/Applications/")) return false
            if (locIdx === 2 && !app.path.includes("/.local/bin/")) return false
            if (locIdx === 3 && !app.path.startsWith("/opt/")) return false

            if (search.length > 0) {
                const nameMatch = app.name.toLowerCase().indexOf(search) !== -1
                const pathMatch = app.path.toLowerCase().indexOf(search) !== -1
                if (!nameMatch && !pathMatch) return false
            }
            return true
        })
    }

    // 3. System Packages & Libraries Model
    readonly property var filteredSystemPackages: {
        const search = systemSearchField.text.trim().toLowerCase()
        const catIdx = systemCategoryCombo.currentIndex
        const src = systemSourceCombo.currentText
        const pkgs = packageMgr.packages || []

        return pkgs.filter(pkg => {
            // Category filter: 0=Explicit, 1=Dependencies, 2=All, 3=AUR
            if (catIdx === 0 && !pkg.isExplicit) return false
            if (catIdx === 1 && pkg.isExplicit) return false
            if (catIdx === 3 && pkg.source !== "AUR") return false

            // Source filter
            if (src !== i18n("All Sources") && pkg.source !== src) return false

            // Search text
            if (search.length > 0) {
                const nameMatch = pkg.name.toLowerCase().indexOf(search) !== -1
                const descMatch = (pkg.description || "").toLowerCase().indexOf(search) !== -1
                if (!nameMatch && !descMatch) return false
            }
            return true
        })
    }

    // ================= MAIN BODY =================

    Item {
        anchors.fill: parent

        ErrorBanner {
            id: errorBanner
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            z: 99
        }

        ScanProgress {
            anchors.top: errorBanner.visible ? errorBanner.bottom : parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.smallSpacing
            z: 98
            running: packageMgr.isLoading || portableMgr.isScanning
            text: packageMgr.isLoading ? i18n("Refreshing packages database...") : i18n("Scanning filesystem for portable apps...")
        }

        // ================= VIEW 0: USER APPLICATIONS (VIRTUALIZED LISTVIEW) =================
        ListView {
            id: userAppsListView
            anchors.fill: parent
            visible: appTabBar.currentIndex === 0
            clip: true
            reuseItems: true
            cacheBuffer: 1000
            spacing: Kirigami.Units.smallSpacing
            model: root.filteredUserApps

            QQC2.ScrollBar.vertical: StyledScrollBar {}

            delegate: Rectangle {
                required property var modelData

                width: parent ? parent.width - 28 : (userAppsListView.width - 28)
                anchors.left: parent ? parent.left : undefined
                anchors.leftMargin: Kirigami.Units.smallSpacing
                height: 72
                color: "#252b33"
                border.color: modelData.hasUpdate ? "#e67e22" : "#38414a"
                border.width: modelData.hasUpdate ? 1.5 : 1
                radius: Kirigami.Units.smallSpacing

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.mediumSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: modelData.icon && modelData.icon.length > 0 ? modelData.icon : "applications-other"
                        implicitWidth: 42
                        implicitHeight: 42
                        fallback: "applications-other"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label {
                                text: modelData.name
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                            }

                            QQC2.Label {
                                text: "v" + modelData.version
                                font.family: "monospace"
                                opacity: 0.7
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.description && modelData.description.length > 0 ? modelData.description : i18n("User Application")
                            elide: Text.ElideRight
                            opacity: 0.75
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                    }

                    // Badges
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Rectangle {
                            implicitHeight: Kirigami.Units.gridUnit * 1.3
                            implicitWidth: srcBadgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                            radius: 4
                            color: {
                                if (modelData.source === "Pacman") return "#1d99f3"
                                if (modelData.source === "AUR") return "#9b59b6"
                                if (modelData.source === "Flatpak") return "#16a085"
                                if (modelData.source === "Portable") return "#8e44ad"
                                return "#525e6b"
                            }
                            QQC2.Label {
                                id: srcBadgeLabel
                                anchors.centerIn: parent
                                text: modelData.source
                                color: "#ffffff"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            }
                        }

                        Rectangle {
                            visible: modelData.size && modelData.size !== "-"
                            implicitHeight: Kirigami.Units.gridUnit * 1.3
                            implicitWidth: sizeBadgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                            radius: 4
                            color: "#343d46"
                            QQC2.Label {
                                id: sizeBadgeLabel
                                anchors.centerIn: parent
                                text: modelData.size
                                color: "#d0d7de"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            }
                        }
                    }

                    // Action buttons
                    QQC2.Button {
                        text: i18n("Launch")
                        icon.name: "system-run"
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Launch %1", modelData.name)
                        onClicked: {
                            if (modelData.isPortable) {
                                portableMgr.launchApp(modelData.path)
                            } else {
                                packageMgr.launchPackage(modelData.source, modelData.name)
                            }
                        }
                    }

                    QQC2.Button {
                        text: i18n("Details")
                        icon.name: "help-about"
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Package metadata & details")
                        visible: !modelData.isPortable
                        onClicked: {
                            root.selectedPackageDetails = packageMgr.getPackageDetails(modelData.source, modelData.name)
                            detailsDialog.open()
                        }
                    }

                    QQC2.Button {
                        icon.name: "edit-delete"
                        hoverEnabled: true
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Uninstall application %1", modelData.name)
                        onClicked: {
                            if (modelData.isPortable) {
                                root.pendingDeletePath = modelData.path
                                deletePortableAppConfirmDialog.text = i18n("Are you sure you want to permanently delete the file '%1'?", modelData.path)
                                deletePortableAppConfirmDialog.open()
                            } else {
                                root.pendingDeleteSource = modelData.source
                                root.pendingDeleteName = modelData.name
                                removePackageConfirmDialog.text = i18n("Are you sure you want to remove '%1' (%2)?", modelData.name, modelData.source)
                                removePackageConfirmDialog.open()
                            }
                        }
                    }
                }
            }
        }

        // ================= VIEW 1: PORTABLE APPS (VIRTUALIZED LISTVIEW) =================
        ListView {
            id: portableAppsListView
            anchors.fill: parent
            visible: appTabBar.currentIndex === 1
            clip: true
            reuseItems: true
            cacheBuffer: 1000
            spacing: Kirigami.Units.smallSpacing
            model: root.filteredPortableApps

            QQC2.ScrollBar.vertical: StyledScrollBar {}

            delegate: Rectangle {
                required property var modelData

                width: parent ? parent.width - 28 : (portableAppsListView.width - 28)
                anchors.left: parent ? parent.left : undefined
                anchors.leftMargin: Kirigami.Units.smallSpacing
                height: 76
                color: "#252b33"
                border.color: modelData.isInMenu ? "#27ae60" : "#38414a"
                border.width: 1
                radius: Kirigami.Units.smallSpacing

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.mediumSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: modelData.icon && modelData.icon.length > 0 ? modelData.icon : "application-x-executable"
                        implicitWidth: 42
                        implicitHeight: 42
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
                            }

                            Rectangle {
                                implicitHeight: Kirigami.Units.gridUnit * 1.2
                                implicitWidth: portTypeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                radius: 4
                                color: modelData.isAppImage ? "#8e44ad" : "#34495e"
                                QQC2.Label {
                                    id: portTypeLabel
                                    anchors.centerIn: parent
                                    text: modelData.isAppImage ? "AppImage" : "ELF Binary"
                                    color: "#ffffff"
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                }
                            }

                            Rectangle {
                                implicitHeight: Kirigami.Units.gridUnit * 1.2
                                implicitWidth: portMenuLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                                radius: 4
                                color: modelData.isInMenu ? "#1e4620" : "#2f3640"
                                border.color: modelData.isInMenu ? "#27ae60" : "#57606f"
                                border.width: 1
                                QQC2.Label {
                                    id: portMenuLabel
                                    anchors.centerIn: parent
                                    text: modelData.isInMenu ? i18n("In KDE Menu") : i18n("Not in Menu")
                                    color: modelData.isInMenu ? "#2ecc71" : "#a4b0be"
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                }
                            }
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.path
                            font.family: "monospace"
                            opacity: 0.75
                            elide: Text.ElideMiddle
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }

                        RowLayout {
                            spacing: Kirigami.Units.largeSpacing
                            QQC2.Label {
                                text: i18n("Size: %1", modelData.size)
                                opacity: 0.6
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            }
                            QQC2.Label {
                                text: i18n("Modified: %1", modelData.lastModified)
                                opacity: 0.6
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                            }
                        }
                    }

                    // Action buttons
                    QQC2.Button {
                        text: i18n("Run")
                        icon.name: "system-run"
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Run portable executable")
                        onClicked: portableMgr.launchApp(modelData.path)
                    }

                    QQC2.Button {
                        text: modelData.isInMenu ? i18n("Remove from Menu") : i18n("Add to Menu")
                        icon.name: modelData.isInMenu ? "edit-delete-remove" : "bookmark-new"
                        display: root.width > Kirigami.Units.gridUnit * 52 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: modelData.isInMenu ? i18n("Remove from KDE Application Menu") : i18n("Add to KDE Application Menu")
                        onClicked: {
                            if (modelData.isInMenu) {
                                portableMgr.removeFromMenu(modelData.path)
                            } else {
                                portableMgr.addToMenu(modelData.path)
                            }
                        }
                    }

                    QQC2.Button {
                        icon.name: "edit-delete"
                        hoverEnabled: true
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Delete file from filesystem")
                        onClicked: {
                            root.pendingDeletePath = modelData.path
                            deletePortableAppConfirmDialog.text = i18n("Are you sure you want to permanently delete '%1'?", modelData.path)
                            deletePortableAppConfirmDialog.open()
                        }
                    }
                }
            }
        }

        // ================= VIEW 2: SYSTEM PACKAGES & LIBRARIES (VIRTUALIZED LISTVIEW) =================
        ListView {
            id: systemPackagesListView
            anchors.fill: parent
            visible: appTabBar.currentIndex === 2
            clip: true
            reuseItems: true
            cacheBuffer: 1000
            spacing: Kirigami.Units.smallSpacing
            model: root.filteredSystemPackages

            QQC2.ScrollBar.vertical: StyledScrollBar {}

            delegate: Rectangle {
                required property var modelData

                width: parent ? parent.width - 28 : (systemPackagesListView.width - 28)
                anchors.left: parent ? parent.left : undefined
                anchors.leftMargin: Kirigami.Units.smallSpacing
                height: 60
                color: "#252b33"
                border.color: modelData.hasUpdate ? "#e67e22" : "#38414a"
                border.width: modelData.hasUpdate ? 1.5 : 1
                radius: Kirigami.Units.smallSpacing

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.mediumSpacing
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: modelData.icon && modelData.icon.length > 0 ? modelData.icon : "package-x-generic"
                        implicitWidth: 32
                        implicitHeight: 32
                        fallback: "package-x-generic"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label {
                                text: modelData.name
                                font.bold: true
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize
                            }

                            QQC2.Label {
                                text: "v" + modelData.version
                                font.family: "monospace"
                                opacity: 0.7
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                            }
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.description && modelData.description.length > 0 ? modelData.description : (modelData.isExplicit ? i18n("User package") : i18n("System library / dependency"))
                            elide: Text.ElideRight
                            opacity: 0.65
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                        }
                    }

                    // Badges
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Rectangle {
                            implicitHeight: Kirigami.Units.gridUnit * 1.2
                            implicitWidth: sysSrcBadge.implicitWidth + Kirigami.Units.smallSpacing * 2
                            radius: 4
                            color: {
                                if (modelData.source === "Pacman") return "#1d99f3"
                                if (modelData.source === "AUR") return "#9b59b6"
                                if (modelData.source === "Flatpak") return "#16a085"
                                if (modelData.source === "Snap") return "#27ae60"
                                return "#525e6b"
                            }
                            QQC2.Label {
                                id: sysSrcBadge
                                anchors.centerIn: parent
                                text: modelData.source
                                color: "#ffffff"
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                            }
                        }

                        Rectangle {
                            implicitHeight: Kirigami.Units.gridUnit * 1.2
                            implicitWidth: expBadge.implicitWidth + Kirigami.Units.smallSpacing * 2
                            radius: 4
                            color: modelData.isExplicit ? "#1f3a24" : "#2a2d32"
                            border.color: modelData.isExplicit ? "#2ecc71" : "#4f5b66"
                            border.width: 1
                            QQC2.Label {
                                id: expBadge
                                anchors.centerIn: parent
                                text: modelData.isExplicit ? i18n("Explicit") : i18n("Dependency")
                                color: modelData.isExplicit ? "#2ecc71" : "#a4b0be"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                            }
                        }

                        Rectangle {
                            visible: modelData.size && modelData.size !== "-"
                            implicitHeight: Kirigami.Units.gridUnit * 1.2
                            implicitWidth: sysSizeBadge.implicitWidth + Kirigami.Units.smallSpacing * 2
                            radius: 4
                            color: "#343d46"
                            QQC2.Label {
                                id: sysSizeBadge
                                anchors.centerIn: parent
                                text: modelData.size
                                color: "#d0d7de"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                            }
                        }
                    }

                    // Actions
                    QQC2.Button {
                        text: i18n("Details")
                        icon.name: "help-about"
                        display: root.width > Kirigami.Units.gridUnit * 48 ? QQC2.AbstractButton.TextBesideIcon : QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Package metadata & details")
                        onClicked: {
                            root.selectedPackageDetails = packageMgr.getPackageDetails(modelData.source, modelData.name)
                            detailsDialog.open()
                        }
                    }

                    QQC2.Button {
                        icon.name: "edit-delete"
                        hoverEnabled: true
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Remove package %1", modelData.name)
                        onClicked: {
                            root.pendingDeleteSource = modelData.source
                            root.pendingDeleteName = modelData.name
                            removePackageConfirmDialog.text = i18n("Are you sure you want to remove '%1' (%2)?", modelData.name, modelData.source)
                            removePackageConfirmDialog.open()
                        }
                    }
                }
            }
        }
    }

    // ================= DIALOGS =================

    // Package Details Dialog
    Kirigami.Dialog {
        id: detailsDialog
        title: root.selectedPackageDetails ? (root.selectedPackageDetails.name || i18n("Package Details")) : i18n("Package Details")
        preferredWidth: Kirigami.Units.gridUnit * 30

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.mediumSpacing

            RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Label {
                    text: i18n("Source:")
                    font.bold: true
                }
                QQC2.Label {
                    text: root.selectedPackageDetails ? (root.selectedPackageDetails.source || "-") : "-"
                }

                Item { Layout.fillWidth: true }

                QQC2.Label {
                    text: i18n("Version:")
                    font.bold: true
                }
                QQC2.Label {
                    text: root.selectedPackageDetails ? (root.selectedPackageDetails.version || "-") : "-"
                    font.family: "monospace"
                }
            }

            RowLayout {
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Label {
                    text: i18n("Installed Size:")
                    font.bold: true
                }
                QQC2.Label {
                    text: root.selectedPackageDetails ? (root.selectedPackageDetails.size || "-") : "-"
                }

                Item { Layout.fillWidth: true }

                QQC2.Label {
                    text: i18n("License:")
                    font.bold: true
                }
                QQC2.Label {
                    text: root.selectedPackageDetails ? (root.selectedPackageDetails.license || "-") : "-"
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            QQC2.Label {
                text: i18n("Description:")
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: Kirigami.Units.gridUnit * 6
                color: "#1e2227"
                border.color: "#38414a"
                border.width: 1
                radius: Kirigami.Units.smallSpacing

                QQC2.ScrollView {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing
                    clip: true

                    QQC2.Label {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: root.selectedPackageDetails ? (root.selectedPackageDetails.description || i18n("No description provided")) : ""
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: root.selectedPackageDetails && root.selectedPackageDetails.dependsOn
                spacing: 2

                QQC2.Label {
                    text: i18n("Dependencies:")
                    font.bold: true
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                    opacity: 0.8
                    text: root.selectedPackageDetails ? (root.selectedPackageDetails.dependsOn || "-") : "-"
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: root.selectedPackageDetails && root.selectedPackageDetails.url
                spacing: 2

                QQC2.Label {
                    text: i18n("Upstream URL:")
                    font.bold: true
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: root.selectedPackageDetails ? (root.selectedPackageDetails.url || "-") : "-"
                    color: "#3daee9"
                }
            }
        }

        customFooterActions: [
            Kirigami.Action {
                text: i18n("Close")
                icon.name: "dialog-close"
                onTriggered: detailsDialog.close()
            }
        ]
    }

    // Removal confirmation
    ConfirmDialog {
        id: removePackageConfirmDialog
        dangerous: true
        confirmButtonText: i18n("Uninstall")
        onConfirmed: {
            packageMgr.removePackage(root.pendingDeleteSource, root.pendingDeleteName)
        }
    }

    // Portable deletion confirmation
    ConfirmDialog {
        id: deletePortableAppConfirmDialog
        dangerous: true
        confirmButtonText: i18n("Delete Permanently")
        onConfirmed: {
            portableMgr.deleteApp(root.pendingDeletePath)
        }
    }

    // File Dialog for manual portable app addition
    QD.FileDialog {
        id: fileDialog
        title: i18n("Select Portable Application Executable")
        onAccepted: {
            const rawPath = String(fileDialog.selectedFile).replace(/^file:\/\//, "")
            portableMgr.addManualApp(rawPath)
        }
    }
}
