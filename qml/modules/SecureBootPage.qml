import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.klarity 1.0
import "../components"

Kirigami.ScrollablePage {
    id: root

    title: i18n("Secure Boot Manager")

    SbctlWrapper {
        id: sbctl

        onOperationFinished: (success, message) => {
            if (!success) {
                errorBanner.showError(message)
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: sbctl.isLoading ? i18n("Working...") : i18n("Verify EFI Files")
            icon.name: "system-search"
            enabled: !sbctl.isLoading
            tooltip: i18n("Verify all EFI binaries on /boot to detect unsigned files")
            onTriggered: sbctl.verifyFiles()
        },
        Kirigami.Action {
            text: i18n("Refresh Status")
            icon.name: "view-refresh"
            enabled: !sbctl.isLoading
            tooltip: i18n("Query UEFI Secure Boot status")
            onTriggered: sbctl.refreshStatus()
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

        // Missing dependency card if sbctl is not installed
        MissingDependencyCard {
            Layout.fillWidth: true
            visible: !sbctl.hasSbctl
            moduleTitle: i18n("Secure Boot Manager")
            dependencyName: "sbctl"
            installCommand: "pacman -S sbctl"
            onRecheckRequested: sbctl.refreshStatus()
        }

        // ================= GUIDED SECURE BOOT ACTIONS =================
        Kirigami.Card {
            Layout.fillWidth: true

            header: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "security-high"
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }

                Kirigami.Heading {
                    level: 3
                    text: i18n("Guided Workflow & Actions")
                    Layout.fillWidth: true
                }

                Rectangle {
                    implicitWidth: readinessLabel.implicitWidth + 14
                    implicitHeight: readinessLabel.implicitHeight + 6
                    radius: 4
                    color: sbctl.unsignedCount > 0 ? "#452424" : (sbctl.secureBoot ? "#203b29" : "#3b3420")

                    QQC2.Label {
                        id: readinessLabel
                        anchors.centerIn: parent
                        font.bold: true
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        text: sbctl.unsignedCount > 0
                              ? i18n("%1 Unsigned Files Found", sbctl.unsignedCount)
                              : (sbctl.secureBoot ? i18n("Protected & Enforced") : i18n("Ready to Sign / Enroll"))
                        color: sbctl.unsignedCount > 0 ? "#ff8585" : (sbctl.secureBoot ? "#5af78e" : "#ffcc00")
                    }
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: i18n("Workflow: 1. Scan and find all unsigned kernels/EFI files → 2. Sign all detected boot files with your custom key → 3. Enroll keys and turn on Secure Boot in UEFI.")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    QQC2.Button {
                        text: i18n("1. Find Unsigned Files")
                        icon.name: "system-search"
                        enabled: !sbctl.isLoading
                        onClicked: sbctl.verifyFiles()
                    }

                    QQC2.Button {
                        text: i18n("2. Sign All Files")
                        icon.name: "document-edit-sign"
                        highlighted: sbctl.unsignedCount > 0
                        enabled: !sbctl.isLoading
                        onClicked: sbctl.signAll()
                    }

                    QQC2.Button {
                        text: i18n("3. Generate / Enroll Keys")
                        icon.name: "document-save"
                        enabled: !sbctl.isLoading
                        onClicked: {
                            if (!sbctl.customKeys) {
                                confirmDialog.actionType = "create_keys"
                                confirmDialog.text = i18n("Create new Secure Boot keys in /etc/sbctl/keys?")
                                confirmDialog.dangerous = false
                                confirmDialog.open()
                            } else {
                                confirmDialog.actionType = "enroll_keys_ms"
                                confirmDialog.text = i18n("Enroll custom keys into UEFI firmware NVRAM with Microsoft certificates?")
                                confirmDialog.dangerous = true
                                confirmDialog.open()
                            }
                        }
                    }
                }
            }
        }

        // Status Cards Grid (Adaptive)
        GridLayout {
            Layout.fillWidth: true
            columns: root.width > Kirigami.Units.gridUnit * 46 ? 3 : 1
            rowSpacing: Kirigami.Units.mediumSpacing
            columnSpacing: Kirigami.Units.mediumSpacing

            // Card 1: Keys Installed
            Kirigami.Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: sbctl.installed ? "security-high" : "dialog-warning"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 4
                            text: i18n("Custom Keys")
                        }
                    }

                    Rectangle {
                        implicitWidth: instLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: Kirigami.Units.gridUnit * 1.5
                        radius: Kirigami.Units.smallSpacing
                        color: sbctl.installed ? "#1b4d27" : "#4a4f56"
                        border.color: sbctl.installed ? "#2ca64f" : "#6c7784"
                        border.width: 1

                        QQC2.Label {
                            id: instLabel
                            anchors.centerIn: parent
                            text: sbctl.installed ? i18n("✓ Installed") : i18n("Not Created")
                            font.bold: true
                            color: "#ffffff"
                        }
                    }

                    QQC2.Label {
                        text: sbctl.guid.length > 0 ? i18n("GUID: %1", sbctl.guid) : i18n("No custom key database")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        color: Kirigami.Theme.disabledTextColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            // Card 2: Setup Mode
            Kirigami.Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: sbctl.setupMode ? "document-edit" : "system-lock-screen"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 4
                            text: i18n("Setup Mode")
                        }
                    }

                    Rectangle {
                        implicitWidth: setupLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: Kirigami.Units.gridUnit * 1.5
                        radius: Kirigami.Units.smallSpacing
                        color: sbctl.setupMode ? "#543714" : "#1f3b52"
                        border.color: sbctl.setupMode ? "#e59020" : "#3daee9"
                        border.width: 1

                        QQC2.Label {
                            id: setupLabel
                            anchors.centerIn: parent
                            text: sbctl.setupMode ? i18n("Enabled (Ready)") : i18n("Disabled (Locked)")
                            font.bold: true
                            color: "#ffffff"
                        }
                    }

                    QQC2.Label {
                        text: sbctl.setupMode
                              ? i18n("UEFI is open for custom key enrollment")
                              : i18n("UEFI enrollment is locked. Clear keys in BIOS to enroll")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        color: Kirigami.Theme.disabledTextColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // Card 3: Secure Boot State
            Kirigami.Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 1

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: sbctl.secureBoot ? "security-high" : "dialog-warning"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        Kirigami.Heading {
                            level: 4
                            text: i18n("Secure Boot")
                        }
                    }

                    Rectangle {
                        implicitWidth: secLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: Kirigami.Units.gridUnit * 1.5
                        radius: Kirigami.Units.smallSpacing
                        color: sbctl.secureBoot ? "#1b4d27" : "#542414"
                        border.color: sbctl.secureBoot ? "#2ca64f" : "#e04f5f"
                        border.width: 1

                        QQC2.Label {
                            id: secLabel
                            anchors.centerIn: parent
                            text: sbctl.secureBoot ? i18n("✓ Enforced") : i18n("Disabled")
                            font.bold: true
                            color: "#ffffff"
                        }
                    }

                    QQC2.Label {
                        text: sbctl.secureBoot
                              ? i18n("Hardware signature validation is active")
                              : i18n("Secure Boot is turned off in UEFI settings")
                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                        color: Kirigami.Theme.disabledTextColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // EFI Files Verification & Signing Section
        Kirigami.Card {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        source: "document-edit-sign"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Kirigami.Heading {
                            level: 3
                            text: i18n("Bootloader & Kernel Signing (sbctl verify)")
                        }

                        QQC2.Label {
                            text: i18n("All kernel images, initramfs, and EFI bootloaders must be signed with your custom key before Secure Boot can boot them.")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    QQC2.Button {
                        text: i18n("Verify Files")
                        icon.name: "system-search"
                        enabled: !sbctl.isLoading
                        onClicked: sbctl.verifyFiles()
                    }

                    QQC2.Button {
                        text: i18n("Sign All (sbctl sign-all)")
                        icon.name: "document-edit-sign"
                        highlighted: true
                        enabled: !sbctl.isLoading
                        onClicked: sbctl.signAll()
                    }
                }

                // Unsigned Files List
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: sbctl.unsignedCount > 0
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: unsignedNotice.implicitHeight + Kirigami.Units.mediumSpacing * 2
                        radius: Kirigami.Units.smallSpacing
                        color: "#4d1414"
                        border.color: "#e04f5f"
                        border.width: 1.5

                        RowLayout {
                            id: unsignedNotice
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.mediumSpacing
                            spacing: Kirigami.Units.mediumSpacing

                            Kirigami.Icon {
                                source: "dialog-warning"
                                implicitWidth: Kirigami.Units.iconSizes.medium
                                implicitHeight: Kirigami.Units.iconSizes.medium
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: i18n("WARNING: Detected %1 unsigned EFI binary/binaries! If you enforce Secure Boot, these will fail to boot.", sbctl.unsignedCount)
                                font.bold: true
                                color: "#ffffff"
                            }

                            QQC2.Button {
                                text: i18n("Sign All Files Now")
                                icon.name: "document-edit-sign"
                                highlighted: true
                                onClicked: sbctl.signAll()
                            }
                        }
                    }

                    Repeater {
                        model: sbctl.unsignedFiles

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: Kirigami.Units.gridUnit * 2.5
                            radius: Kirigami.Units.smallSpacing
                            color: Kirigami.Theme.alternateBackgroundColor
                            border.color: "#e04f5f"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.smallSpacing
                                spacing: Kirigami.Units.mediumSpacing

                                Rectangle {
                                    implicitWidth: Kirigami.Units.gridUnit * 4.5
                                    implicitHeight: Kirigami.Units.gridUnit * 1.4
                                    radius: height / 2
                                    color: "#542414"

                                    QQC2.Label {
                                        anchors.centerIn: parent
                                        text: "✗ " + i18n("Unsigned")
                                        font.bold: true
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.8
                                        color: "#ffffff"
                                    }
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: modelData
                                    font.family: "monospace"
                                    font.bold: true
                                    elide: Text.ElideMiddle
                                }

                                QQC2.Button {
                                    text: i18n("Sign File")
                                    icon.name: "document-edit-sign"
                                    onClicked: sbctl.signFile(modelData)
                                }
                            }
                        }
                    }
                }

                // Signed Files Collapsible Summary
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: sbctl.signedCount > 0
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Kirigami.Icon {
                            source: "dialog-ok"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            text: i18n("Verified Signed Files (%1):", sbctl.signedCount)
                            font.bold: true
                            color: "#2ca64f"
                        }
                    }

                    Repeater {
                        model: sbctl.signedFiles

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: Kirigami.Units.gridUnit * 2.2
                            radius: Kirigami.Units.smallSpacing
                            color: Kirigami.Theme.alternateBackgroundColor

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.smallSpacing
                                spacing: Kirigami.Units.mediumSpacing

                                QQC2.Label {
                                    text: "✓"
                                    font.bold: true
                                    color: "#2ca64f"
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: modelData
                                    font.family: "monospace"
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                                    color: Kirigami.Theme.disabledTextColor
                                    elide: Text.ElideMiddle
                                }
                            }
                        }
                    }
                }
            }
        }

        // Danger Zone: Create and Enroll Keys
        Kirigami.Card {
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: "dialog-warning"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    Kirigami.Heading {
                        level: 3
                        text: i18n("Key Management & UEFI Enrollment (Danger Zone)")
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: dangerNotice.implicitHeight + Kirigami.Units.mediumSpacing * 2
                    radius: Kirigami.Units.smallSpacing
                    color: "#3a2512"
                    border.color: "#e59020"
                    border.width: 1.5

                    RowLayout {
                        id: dangerNotice
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: Kirigami.Units.mediumSpacing

                        Kirigami.Icon {
                            source: "dialog-warning"
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: Kirigami.Units.iconSizes.medium
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: i18n("Enrolling custom keys will write your cryptographic certificates directly to UEFI NVRAM. Always keep a bootable Linux Live USB handy before changing Secure Boot keys. Key enrollment requires UEFI Setup Mode to be enabled.")
                            color: "#ffffff"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    QQC2.Button {
                        text: i18n("Create Custom Keys (sbctl create-keys)")
                        icon.name: "document-new"
                        enabled: !sbctl.isLoading
                        onClicked: {
                            confirmDialog.actionType = "create_keys"
                            confirmDialog.title = i18n("Create Custom Keys?")
                            confirmDialog.message = i18n("This will generate a new set of PK, KEK, and DB keys under /usr/share/secureboot/keys.")
                            confirmDialog.warningText = i18n("Command: pkexec sbctl create-keys")
                            confirmDialog.dangerous = false
                            confirmDialog.open()
                        }
                    }

                    QQC2.Button {
                        text: i18n("Enroll Keys + Microsoft (Recommended)")
                        icon.name: "document-save"
                        highlighted: true
                        enabled: !sbctl.isLoading && sbctl.setupMode
                        onClicked: {
                            confirmDialog.actionType = "enroll_keys_ms"
                            confirmDialog.title = i18n("Enroll Keys with Microsoft Support?")
                            confirmDialog.message = i18n("This will write your custom keys along with Microsoft OEM certificates into UEFI NVRAM. This ensures dual-boot Windows and GPU option ROMs continue working.")
                            confirmDialog.warningText = i18n("Command: pkexec sbctl enroll-keys --microsoft\nEnsure you have a rescue Live USB ready!")
                            confirmDialog.dangerous = true
                            confirmDialog.open()
                        }
                    }

                    QQC2.Button {
                        text: i18n("Enroll Keys Only (No Microsoft)")
                        icon.name: "security-high"
                        enabled: !sbctl.isLoading && sbctl.setupMode
                        onClicked: {
                            confirmDialog.actionType = "enroll_keys_noms"
                            confirmDialog.title = i18n("Enroll Custom Keys Only?")
                            confirmDialog.message = i18n("This will write ONLY your custom keys into UEFI NVRAM without Microsoft certificates. Warning: Windows and hardware option ROMs will NOT boot.")
                            confirmDialog.warningText = i18n("Command: pkexec sbctl enroll-keys\nExtreme caution: Make sure you have a rescue Live USB!")
                            confirmDialog.dangerous = true
                            confirmDialog.open()
                        }
                    }
                }
            }
        }

        // Execution Output Log Card
        Kirigami.Card {
            Layout.fillWidth: true
            visible: sbctl.lastOutput.length > 0

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Heading {
                        level: 4
                        text: i18n("Last Command Output")
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.Button {
                        text: i18n("Clear Output")
                        icon.name: "edit-clear"
                        onClicked: sbctl.lastOutput = ""
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Math.min(outputLabel.implicitHeight + Kirigami.Units.mediumSpacing * 2, Kirigami.Units.gridUnit * 12)
                    color: "#1e2227"
                    border.color: "#38414a"
                    border.width: 1
                    radius: Kirigami.Units.smallSpacing

                    QQC2.ScrollView {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.smallSpacing
                        clip: true

                        QQC2.ScrollBar.vertical: StyledScrollBar {
                            active: true
                        }

                        QQC2.Label {
                            id: outputLabel
                            width: parent.width
                            text: sbctl.lastOutput
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.9
                            color: "#e0e6ed"
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    // Confirmation Dialog for Dangerous Actions
    ConfirmDialog {
        id: confirmDialog
        property string actionType: ""

        onConfirmed: {
            if (actionType === "create_keys") {
                sbctl.createKeys()
            } else if (actionType === "enroll_keys_ms") {
                sbctl.enrollKeys(true)
            } else if (actionType === "enroll_keys_noms") {
                sbctl.enrollKeys(false)
            }
        }
    }
}
