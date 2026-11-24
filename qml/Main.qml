import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.klarity 1.0
import "components"

Kirigami.ApplicationWindow {
    id: root

    title: i18n("Klarity - Linux Control Center")
    width: Kirigami.Units.gridUnit * 58
    height: Kirigami.Units.gridUnit * 42
    minimumWidth: Kirigami.Units.gridUnit * 42
    minimumHeight: Kirigami.Units.gridUnit * 28

    Kirigami.PagePool {
        id: appPagePool
    }

    // Right side overlay drawer for logs
    contextDrawer: LogPanel {
        id: logPanel
    }

    // Left Navigation Drawer / Sidebar
    globalDrawer: Kirigami.GlobalDrawer {
        id: navDrawer
        title: i18n("Klarity")
        titleIcon: "preferences-system"
        isMenu: false
        collapsed: false
        modal: false

        actions: [
            Kirigami.PagePoolAction {
                text: i18n("Autostart")
                icon.name: "preferences-system-autostart"
                page: Qt.resolvedUrl("modules/AutostartPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Shell Aliases & Functions")
                icon.name: "utilities-terminal"
                page: Qt.resolvedUrl("modules/AliasesPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Shortcuts & Keybindings")
                icon.name: "preferences-desktop-keyboard-shortcuts"
                page: Qt.resolvedUrl("modules/ShortcutsPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Touchpad Gestures")
                icon.name: "input-touchpad"
                page: Qt.resolvedUrl("modules/GesturesPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Keyboard Dispatcher")
                icon.name: "input-keyboard"
                page: Qt.resolvedUrl("modules/KeydPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("System Auditor")
                icon.name: "security-high"
                page: Qt.resolvedUrl("modules/AuditorPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Secure Boot")
                icon.name: "security-high"
                page: Qt.resolvedUrl("modules/SecureBootPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Applications & Packages")
                icon.name: "applications-other"
                page: Qt.resolvedUrl("modules/ApplicationsPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("DNS Center")
                icon.name: "network-wired"
                page: Qt.resolvedUrl("modules/DnsPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Global File Search")
                icon.name: "system-search"
                page: Qt.resolvedUrl("modules/SearchPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("System Updates")
                icon.name: "system-software-update"
                page: Qt.resolvedUrl("modules/UpdatesPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Power & Performance")
                icon.name: "battery-charging"
                page: Qt.resolvedUrl("modules/PowerPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Disk Health")
                icon.name: "drive-harddisk"
                page: Qt.resolvedUrl("modules/DiskHealthPage.qml")
                pagePool: appPagePool
            },
            Kirigami.PagePoolAction {
                text: i18n("Overview & Diagnostics") + (ModuleRegistry.missingDependencies("placeholder").length > 0 ? " (2)" : "")
                icon.name: "system-run"
                page: Qt.resolvedUrl("modules/PlaceholderPage.qml")
                pagePool: appPagePool
            }
        ]

        footer: ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.mediumSpacing
            spacing: 2

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            QQC2.Label {
                text: i18n("Distro: %1 (%2)", EnvironmentInfo.distroName, EnvironmentInfo.distroId)
                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                opacity: 0.7
            }

            QQC2.Label {
                text: i18n("Session: %1", EnvironmentInfo.sessionType.toUpperCase())
                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                opacity: 0.7
            }
        }
    }

    pageStack.initialPage: Qt.resolvedUrl("modules/AutostartPage.qml")

    Component {
        id: customScrollBarComp
        StyledScrollBar {}
    }

    function stylePageScrollBar(page) {
        if (!page || !page.contentItem) return
        // Do not attach on non-scrollable pages like ApplicationsPage that manage their own ListViews
        if (!page.flickable && typeof page.contentItem.flickableItem === "undefined") {
            return
        }
        try {
            if (page.contentItem.QQC2 && page.contentItem.QQC2.ScrollBar) {
                var currentBar = page.contentItem.QQC2.ScrollBar.vertical
                if (!currentBar || !currentBar.hasOwnProperty("__klarityCustom")) {
                    var newBar = customScrollBarComp.createObject(page.contentItem)
                    if (newBar) {
                        page.contentItem.QQC2.ScrollBar.vertical = newBar
                    }
                }
            }
        } catch (e) {
            // Non-scrollable or custom view
        }
    }

    Component.onCompleted: {
        root.stylePageScrollBar(pageStack.currentItem)
    }

    Connections {
        target: pageStack
        function onCurrentItemChanged() {
            root.stylePageScrollBar(pageStack.currentItem)
        }
    }
}
