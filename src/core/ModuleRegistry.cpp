#include "ModuleRegistry.h"
#include "DependencyChecker.h"

ModuleRegistry *ModuleRegistry::s_instance = nullptr;

ModuleRegistry *ModuleRegistry::instance()
{
    if (!s_instance) {
        s_instance = new ModuleRegistry();
    }
    return s_instance;
}

ModuleRegistry *ModuleRegistry::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

ModuleRegistry::ModuleRegistry(QObject *parent)
    : QObject(parent)
{
    if (!s_instance) {
        s_instance = this;
    }
    registerBuiltin();
}

QVariantList ModuleRegistry::modules() const
{
    QVariantList list;
    list.reserve(m_registeredModules.size());
    for (const auto &mod : m_registeredModules) {
        list.append(mod.toMap());
    }
    return list;
}

void ModuleRegistry::registerModule(const ModuleDescriptor &descriptor)
{
    m_registeredModules.append(descriptor);
    emit modulesChanged();
}

void ModuleRegistry::registerBuiltin()
{
    // 1. Autostart Module
    ModuleDescriptor autostart;
    autostart.id = QStringLiteral("autostart");
    autostart.title = QStringLiteral("Autostart");
    autostart.icon = QStringLiteral("preferences-system-autostart");
    autostart.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/AutostartPage.qml");
    autostart.category = QStringLiteral("System");
    autostart.requiresRoot = false;
    autostart.dependencies = QStringList{QStringLiteral("systemctl")};
    registerModule(autostart);

    // 2. Shell Aliases & Functions Module
    ModuleDescriptor aliases;
    aliases.id = QStringLiteral("aliases");
    aliases.title = QStringLiteral("Shell Aliases & Functions");
    aliases.icon = QStringLiteral("utilities-terminal");
    aliases.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/AliasesPage.qml");
    aliases.category = QStringLiteral("Tools");
    aliases.requiresRoot = false;
    aliases.dependencies = QStringList{};
    registerModule(aliases);

    // 3. Shortcuts & Keybindings Module
    ModuleDescriptor shortcuts;
    shortcuts.id = QStringLiteral("shortcuts");
    shortcuts.title = QStringLiteral("Shortcuts & Keybindings");
    shortcuts.icon = QStringLiteral("preferences-desktop-keyboard-shortcuts");
    shortcuts.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/ShortcutsPage.qml");
    shortcuts.category = QStringLiteral("System");
    shortcuts.requiresRoot = false;
    shortcuts.dependencies = QStringList{};
    registerModule(shortcuts);

    // 4. Touchpad Gestures Module (libinput-gestures)
    ModuleDescriptor gestures;
    gestures.id = QStringLiteral("gestures");
    gestures.title = QStringLiteral("Touchpad Gestures");
    gestures.icon = QStringLiteral("input-touchpad");
    gestures.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/GesturesPage.qml");
    gestures.category = QStringLiteral("Hardware");
    gestures.requiresRoot = false;
    gestures.dependencies = QStringList{QStringLiteral("libinput-gestures")};
    registerModule(gestures);

    // 5. Keyboard Dispatcher Module (keyd)
    ModuleDescriptor keyd;
    keyd.id = QStringLiteral("keyd");
    keyd.title = QStringLiteral("Keyboard Dispatcher (keyd)");
    keyd.icon = QStringLiteral("input-keyboard");
    keyd.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/KeydPage.qml");
    keyd.category = QStringLiteral("Hardware");
    keyd.requiresRoot = true;
    keyd.dependencies = QStringList{QStringLiteral("keyd")};
    registerModule(keyd);

    // 6. System Auditor & Cleaner Module
    ModuleDescriptor auditor;
    auditor.id = QStringLiteral("auditor");
    auditor.title = QStringLiteral("System Auditor & Cleaner");
    auditor.icon = QStringLiteral("security-high");
    auditor.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/AuditorPage.qml");
    auditor.category = QStringLiteral("System");
    auditor.requiresRoot = true;
    auditor.dependencies = QStringList{QStringLiteral("pacman")};
    registerModule(auditor);

    // 7. Secure Boot Manager (sbctl)
    ModuleDescriptor secureboot;
    secureboot.id = QStringLiteral("secureboot");
    secureboot.title = QStringLiteral("Secure Boot Manager");
    secureboot.icon = QStringLiteral("security-high");
    secureboot.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/SecureBootPage.qml");
    secureboot.category = QStringLiteral("Hardware");
    secureboot.requiresRoot = true;
    secureboot.dependencies = QStringList{QStringLiteral("sbctl")};
    registerModule(secureboot);

    // 8. Applications Catalog & Manager (Pacman, AUR, Flatpak, Snap, Portable Apps)
    ModuleDescriptor applications;
    applications.id = QStringLiteral("applications");
    applications.title = QStringLiteral("Applications & Packages");
    applications.icon = QStringLiteral("applications-other");
    applications.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/ApplicationsPage.qml");
    applications.category = QStringLiteral("Tools");
    applications.requiresRoot = false;
    applications.dependencies = QStringList{QStringLiteral("pacman")};
    registerModule(applications);

    // 9. DNS Center Module
    ModuleDescriptor dns;
    dns.id = QStringLiteral("dns");
    dns.title = QStringLiteral("DNS Center");
    dns.icon = QStringLiteral("network-wired");
    dns.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/DnsPage.qml");
    dns.category = QStringLiteral("Network");
    dns.requiresRoot = false;
    dns.dependencies = QStringList{QStringLiteral("nmcli"), QStringLiteral("resolvectl")};
    registerModule(dns);

    // 10. Global File Search Module
    ModuleDescriptor search;
    search.id = QStringLiteral("search");
    search.title = QStringLiteral("Global File Search");
    search.icon = QStringLiteral("system-search");
    search.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/SearchPage.qml");
    search.category = QStringLiteral("Tools");
    search.requiresRoot = false;
    search.dependencies = QStringList{QStringLiteral("fd")};
    registerModule(search);

    // 11. System Updates Module
    ModuleDescriptor updates;
    updates.id = QStringLiteral("updates");
    updates.title = QStringLiteral("System Updates");
    updates.icon = QStringLiteral("system-software-update");
    updates.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/UpdatesPage.qml");
    updates.category = QStringLiteral("System");
    updates.requiresRoot = true;
    updates.dependencies = QStringList{QStringLiteral("pacman")};
    registerModule(updates);

    // 12. Power & Performance Module
    ModuleDescriptor power;
    power.id = QStringLiteral("power");
    power.title = QStringLiteral("Power & Performance");
    power.icon = QStringLiteral("battery-charging");
    power.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/PowerPage.qml");
    power.category = QStringLiteral("Hardware");
    power.requiresRoot = true;
    power.dependencies = QStringList{};
    registerModule(power);

    // 13. Disk Health & S.M.A.R.T. Module
    ModuleDescriptor diskhealth;
    diskhealth.id = QStringLiteral("diskhealth");
    diskhealth.title = QStringLiteral("Disk Health");
    diskhealth.icon = QStringLiteral("drive-harddisk");
    diskhealth.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/DiskHealthPage.qml");
    diskhealth.category = QStringLiteral("Hardware");
    diskhealth.requiresRoot = true;
    diskhealth.dependencies = QStringList{QStringLiteral("smartctl")};
    registerModule(diskhealth);

    // 14. Reference / Diagnostics Template
    ModuleDescriptor placeholder;
    placeholder.id = QStringLiteral("placeholder");
    placeholder.title = QStringLiteral("Overview & Diagnostics");
    placeholder.icon = QStringLiteral("system-run");
    placeholder.qmlSource = QStringLiteral("qrc:/org/klarity/qml/modules/PlaceholderPage.qml");
    placeholder.category = QStringLiteral("Core");
    placeholder.requiresRoot = false;
    placeholder.dependencies = QStringList{QStringLiteral("fd"), QStringLiteral("smartctl")};
    registerModule(placeholder);
}

QStringList ModuleRegistry::missingDependencies(const QString &id) const
{
    for (const auto &mod : m_registeredModules) {
        if (mod.id == id) {
            DependencyChecker checker;
            return checker.missing(mod.dependencies);
        }
    }
    return {};
}

QVariantMap ModuleRegistry::getModule(const QString &id) const
{
    for (const auto &mod : m_registeredModules) {
        if (mod.id == id) {
            return mod.toMap();
        }
    }
    return {};
}
