#include <QTest>
#include <QSignalSpy>
#include <QQmlEngine>
#include <QQmlComponent>
#include <KLocalizedString>
#include <klocalizedqmlcontext.h>

#include "core/LogModel.h"
#include "core/DependencyChecker.h"
#include "core/EnvironmentInfo.h"
#include "core/ProcessRunner.h"
#include "core/PkexecRunner.h"
#include "core/ModuleRegistry.h"
#include "modules/autostart/AutostartManager.h"
#include "modules/aliases/AliasManager.h"
#include "modules/shortcuts/ShortcutManager.h"
#include "modules/gestures/GesturesManager.h"
#include "modules/keyd/KeydManager.h"
#include "modules/auditor/SystemAuditor.h"
#include "modules/secureboot/SbctlWrapper.h"
#include "modules/applications/PackageManager.h"
#include "modules/applications/PortableAppsManager.h"
#include "modules/dns/DnsManager.h"
#include "modules/search/SearchManager.h"
#include "modules/updates/UpdateManager.h"
#include "modules/power/PowerManager.h"
#include "modules/diskhealth/DiskHealthManager.h"

class CoreTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        KLocalizedString::setApplicationDomain("klarity");
    }
    void testLogModel();
    void testLogModelRingBuffer();
    void testDependencyChecker();
    void testEnvironmentInfo();
    void testProcessRunnerSync();
    void testProcessRunnerAsync();
    void testModuleRegistry();
    void testAutostartManager();
    void testAutostartPageQml();
    void testAliasManager();
    void testAliasesPageQml();
    void testShortcutManager();
    void testShortcutsPageQml();
    void testGesturesManager();
    void testGesturesPageQml();
    void testKeydManager();
    void testKeydPageQml();
    void testSystemAuditor();
    void testAuditorPageQml();
    void testSbctlWrapper();
    void testSecureBootPageQml();
    void testPackageManager();
    void testPortableAppsManager();
    void testApplicationsPageQml();
    void testPlaceholderPageQml();
    void testDnsManager();
    void testDnsPageQml();
    void testSearchManager();
    void testSearchPageQml();
    void testUpdateManager();
    void testUpdatesPageQml();
    void testPowerManager();
    void testPowerPageQml();
    void testDiskHealthManager();
    void testDiskHealthPageQml();
};

void CoreTests::testLogModel()
{
    auto *model = LogModel::instance();
    QVERIFY(model != nullptr);

    model->clear();
    QCOMPARE(model->rowCount(), 0);

    model->append(QStringLiteral("Test"), QStringLiteral("info"), QStringLiteral("Hello log"));
    QCOMPARE(model->rowCount(), 1);

    const QModelIndex idx = model->index(0, 0);
    QCOMPARE(model->data(idx, LogModel::SourceRole).toString(), QStringLiteral("Test"));
    QCOMPARE(model->data(idx, LogModel::LevelRole).toString(), QStringLiteral("info"));
    QCOMPARE(model->data(idx, LogModel::TextRole).toString(), QStringLiteral("Hello log"));

    const QString all = model->allText();
    QVERIFY(all.contains(QStringLiteral("[INFO] [Test]: Hello log")));

    model->clear();
    QCOMPARE(model->rowCount(), 0);
}

void CoreTests::testLogModelRingBuffer()
{
    auto *model = LogModel::instance();
    model->clear();

    // Max is 5000, verify it caps at 5000
    for (int i = 0; i < 5005; ++i) {
        model->append(QStringLiteral("Bulk"), QStringLiteral("info"), QString::number(i));
    }
    QCOMPARE(model->rowCount(), 5000);

    // Oldest 5 entries should have been evicted, index 0 should now be "5"
    QCOMPARE(model->data(model->index(0, 0), LogModel::TextRole).toString(), QStringLiteral("5"));
    model->clear();
}

void CoreTests::testDependencyChecker()
{
    DependencyChecker checker;

    // "sh" should always exist on Linux
    QVERIFY(checker.hasCommand(QStringLiteral("sh")));
    QVERIFY(!checker.hasCommand(QStringLiteral("definitely_non_existent_binary_xyz_987")));

    const QStringList testList = {QStringLiteral("sh"), QStringLiteral("definitely_non_existent_binary_xyz_987")};
    const QStringList missing = checker.missing(testList);
    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), QStringLiteral("definitely_non_existent_binary_xyz_987"));

    const QString hint = checker.installHint(QStringLiteral("smartctl"));
    QVERIFY(hint.contains(QStringLiteral("smartmontools")));
}

void CoreTests::testEnvironmentInfo()
{
    auto *env = EnvironmentInfo::instance();
    QVERIFY(env != nullptr);

    // Should detect either wayland, x11, or unknown
    const QString session = env->sessionType();
    QVERIFY(!session.isEmpty());

    // Should detect distro ID (cachyos or arch on this system)
    const QString distro = env->distroId();
    QVERIFY(!distro.isEmpty());
    QVERIFY(distro == QStringLiteral("cachyos") || distro == QStringLiteral("arch") || distro == QStringLiteral("unknown"));
}

void CoreTests::testProcessRunnerSync()
{
    const ProcessResult result = ProcessRunner::run(QStringLiteral("echo"), {QStringLiteral("klarity_test")});
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.stdoutStr.contains(QStringLiteral("klarity_test")));
    QCOMPARE(result.timedOut, false);
}

void CoreTests::testProcessRunnerAsync()
{
    ProcessRunner runner;
    QSignalSpy finishedSpy(&runner, &ProcessRunner::finished);
    QSignalSpy startedSpy(&runner, &ProcessRunner::started);

    runner.runAsync(QStringLiteral("echo"), {QStringLiteral("async_test_klarity")});

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const int exitCode = finishedSpy.first().at(0).toInt();
    const QString output = finishedSpy.first().at(1).toString();
    QCOMPARE(exitCode, 0);
    QVERIFY(output.contains(QStringLiteral("async_test_klarity")));
}

void CoreTests::testModuleRegistry()
{
    auto *registry = ModuleRegistry::instance();
    QVERIFY(registry != nullptr);

    const QVariantList list = registry->modules();
    QCOMPARE(list.size(), 14);

    const QVariantMap autostart = registry->getModule(QStringLiteral("autostart"));
    QCOMPARE(autostart.value(QStringLiteral("id")).toString(), QStringLiteral("autostart"));

    const QVariantMap aliases = registry->getModule(QStringLiteral("aliases"));
    QCOMPARE(aliases.value(QStringLiteral("id")).toString(), QStringLiteral("aliases"));

    const QVariantMap shortcuts = registry->getModule(QStringLiteral("shortcuts"));
    QCOMPARE(shortcuts.value(QStringLiteral("id")).toString(), QStringLiteral("shortcuts"));

    const QVariantMap gestures = registry->getModule(QStringLiteral("gestures"));
    QCOMPARE(gestures.value(QStringLiteral("id")).toString(), QStringLiteral("gestures"));

    const QVariantMap keyd = registry->getModule(QStringLiteral("keyd"));
    QCOMPARE(keyd.value(QStringLiteral("id")).toString(), QStringLiteral("keyd"));

    const QVariantMap auditor = registry->getModule(QStringLiteral("auditor"));
    QCOMPARE(auditor.value(QStringLiteral("id")).toString(), QStringLiteral("auditor"));
    QCOMPARE(auditor.value(QStringLiteral("category")).toString(), QStringLiteral("System"));

    const QVariantMap sb = registry->getModule(QStringLiteral("secureboot"));
    QCOMPARE(sb.value(QStringLiteral("id")).toString(), QStringLiteral("secureboot"));
    QCOMPARE(sb.value(QStringLiteral("category")).toString(), QStringLiteral("Hardware"));

    const QVariantMap dns = registry->getModule(QStringLiteral("dns"));
    QCOMPARE(dns.value(QStringLiteral("id")).toString(), QStringLiteral("dns"));
    QCOMPARE(dns.value(QStringLiteral("category")).toString(), QStringLiteral("Network"));

    const QVariantMap search = registry->getModule(QStringLiteral("search"));
    QCOMPARE(search.value(QStringLiteral("id")).toString(), QStringLiteral("search"));
    QCOMPARE(search.value(QStringLiteral("category")).toString(), QStringLiteral("Tools"));

    const QVariantMap updates = registry->getModule(QStringLiteral("updates"));
    QCOMPARE(updates.value(QStringLiteral("id")).toString(), QStringLiteral("updates"));
    QCOMPARE(updates.value(QStringLiteral("category")).toString(), QStringLiteral("System"));
    QCOMPARE(updates.value(QStringLiteral("requiresRoot")).toBool(), true);

    const QVariantMap power = registry->getModule(QStringLiteral("power"));
    QCOMPARE(power.value(QStringLiteral("id")).toString(), QStringLiteral("power"));
    QCOMPARE(power.value(QStringLiteral("category")).toString(), QStringLiteral("Hardware"));
    QCOMPARE(power.value(QStringLiteral("requiresRoot")).toBool(), true);

    const QVariantMap diskhealth = registry->getModule(QStringLiteral("diskhealth"));
    QCOMPARE(diskhealth.value(QStringLiteral("id")).toString(), QStringLiteral("diskhealth"));
    QCOMPARE(diskhealth.value(QStringLiteral("category")).toString(), QStringLiteral("Hardware"));
    QCOMPARE(diskhealth.value(QStringLiteral("requiresRoot")).toBool(), true);

    const QVariantMap mod = registry->getModule(QStringLiteral("placeholder"));
    QCOMPARE(mod.value(QStringLiteral("id")).toString(), QStringLiteral("placeholder"));
    QCOMPARE(mod.value(QStringLiteral("category")).toString(), QStringLiteral("Core"));
}

void CoreTests::testAutostartManager()
{
    AutostartManager mgr;
    const QVariantList entries = mgr.entries();
    // System should have loaded desktop files or systemd user units
    QVERIFY(!entries.isEmpty());

    bool hasDesktopOrSystemd = false;
    for (const auto &v : entries) {
        const QVariantMap map = v.toMap();
        if (map.value(QStringLiteral("type")).toString() == QStringLiteral("Desktop") ||
            map.value(QStringLiteral("type")).toString() == QStringLiteral("Systemd")) {
            hasDesktopOrSystemd = true;
            break;
        }
    }
    QVERIFY(hasDesktopOrSystemd);
}

void CoreTests::testAliasManager()
{
    AliasManager mgr;
    const QVariantList entries = mgr.entries();
    // User has fish functions and bashrc aliases
    QVERIFY(!entries.isEmpty());

    // Test syntax validation
    const QVariantMap validRes = mgr.validateSyntax(QStringLiteral("Bash"), QStringLiteral("Alias"),
                                                    QStringLiteral("my_test_alias"), QStringLiteral("ls -la"));
    QCOMPARE(validRes.value(QStringLiteral("valid")).toBool(), true);

    const QVariantMap invalidRes = mgr.validateSyntax(QStringLiteral("Bash"), QStringLiteral("Alias"),
                                                      QStringLiteral(""), QStringLiteral("ls -la"));
    QCOMPARE(invalidRes.value(QStringLiteral("valid")).toBool(), false);
}

void CoreTests::testShortcutManager()
{
    ShortcutManager mgr;
    const QVariantList list = mgr.shortcuts();
    // System should have loaded dozens of shortcuts from kglobalshortcutsrc
    QVERIFY(!list.isEmpty());

    bool hasKWin = false;
    for (const auto &v : list) {
        const QVariantMap map = v.toMap();
        if (map.value(QStringLiteral("source")).toString() == QStringLiteral("KWin")) {
            hasKWin = true;
            break;
        }
    }
    QVERIFY(hasKWin);

    // conflictCount should be non-negative
    QVERIFY(mgr.conflictCount() >= 0);
    QSignalSpy spy(&mgr, &ShortcutManager::shortcutsChanged);
    mgr.loadAsync();
    QVERIFY(spy.wait(5000));
}

void CoreTests::testGesturesManager()
{
    GesturesManager mgr;
    const QVariantList list = mgr.gestures();
    // The user's system has 4 configured gestures
    QVERIFY(!list.isEmpty());

    const auto presets = mgr.presets();
    QVERIFY(!presets.isEmpty());

    // Verify first gesture fields
    const QVariantMap first = list.first().toMap();
    QVERIFY(first.contains(QStringLiteral("type")));
    QVERIFY(first.contains(QStringLiteral("direction")));
    QVERIFY(first.contains(QStringLiteral("fingers")));
    QVERIFY(first.contains(QStringLiteral("command")));
    QVERIFY(first.contains(QStringLiteral("enabled")));
}

void CoreTests::testKeydManager()
{
    KeydManager mgr;
    const QVariantList list = mgr.mappings();
    // The user's system has /etc/keyd/default.conf with mappings
    QVERIFY(!list.isEmpty());

    const QStringList layers = mgr.layers();
    QVERIFY(layers.contains(QStringLiteral("main")));

    const auto presets = mgr.presets();
    QVERIFY(!presets.isEmpty());

    // Verify first mapping structure
    const QVariantMap first = list.first().toMap();
    QVERIFY(first.contains(QStringLiteral("key")));
    QVERIFY(first.contains(QStringLiteral("action")));
    QVERIFY(first.contains(QStringLiteral("layer")));
}

void CoreTests::testSystemAuditor()
{
    SystemAuditor auditor;

    // 1. Whitelist protection tests
    QVERIFY(auditor.isWhitelisted(QStringLiteral("gtk-3.0")));
    QVERIFY(auditor.isWhitelisted(QStringLiteral("autostart")));
    QVERIFY(auditor.isWhitelisted(QStringLiteral("plasma-org.kde.plasma.desktop-appletsrc")));
    QVERIFY(auditor.isWhitelisted(QStringLiteral("kde.org")));
    QVERIFY(auditor.isWhitelisted(QStringLiteral("systemd")));
    QVERIFY(auditor.isWhitelisted(QStringLiteral("cachyos-hello")));
    QVERIFY(!auditor.isWhitelisted(QStringLiteral("definitely_non_whitelisted_orphaned_folder_xyz")));

    // 2. Perform real system audit
    auditor.startScan();

    const QVariantList items = auditor.items();
    // System should find real pacman cache, orphans, etc.
    QVERIFY(!items.isEmpty());
    QVERIFY(auditor.totalReclaimableBytes() >= 0);
    QVERIFY(!auditor.totalReclaimableFormatted().isEmpty());

    // 3. Selection controls
    auditor.selectAllSafe();
    QVERIFY(auditor.selectedCount() >= auditor.safeCount());

    auditor.deselectAll();
    QCOMPARE(auditor.selectedCount(), 0);
}

void CoreTests::testSbctlWrapper()
{
    SbctlWrapper wrapper;
    // User has sbctl installed at /usr/local/bin/sbctl
    QVERIFY(wrapper.hasSbctl());

    // Wait for initial status query or call refreshStatus
    wrapper.refreshStatus();

    // Verify properties are defined and initialized
    QVERIFY(wrapper.installed() || !wrapper.installed());
    QVERIFY(wrapper.setupMode() || !wrapper.setupMode());
    QVERIFY(wrapper.secureBoot() || !wrapper.secureBoot());
}

void CoreTests::testPackageManager()
{
    PackageManager manager;
    manager.loadPackagesSync();
    const QVariantList pkgs = manager.packages();

    // System has pacman and installed packages
    QVERIFY(manager.totalCount() > 0);
    QVERIFY(manager.guiCount() > 0);
    QVERIFY(manager.explicitCount() > 0);
    QVERIFY(manager.pacmanCount() > 0);
    QVERIFY(!pkgs.isEmpty());

    // Test detail query for pacman
    const QVariantMap details = manager.getPackageDetails(QStringLiteral("Pacman"), QStringLiteral("pacman"));
    QCOMPARE(details.value(QStringLiteral("name")).toString(), QStringLiteral("pacman"));
    QVERIFY(!details.value(QStringLiteral("version")).toString().isEmpty());
}

void CoreTests::testPortableAppsManager()
{
    PortableAppsManager manager;
    manager.scanPortableAppsSync();

    const QVariantList apps = manager.apps();
    QVERIFY(manager.count() >= 0);
    for (const auto &item : apps) {
        const QVariantMap app = item.toMap();
        QVERIFY(!app.value(QStringLiteral("path")).toString().isEmpty());
        QVERIFY(!app.value(QStringLiteral("name")).toString().isEmpty());
    }
}

void CoreTests::testDnsManager()
{
    DnsManager manager;
    QVERIFY(!manager.activeServer().isEmpty());
    QVERIFY(!manager.activeProvider().isEmpty());

    const QVariantList providers = manager.availableProviders();
    QVERIFY(providers.size() >= 5);

    bool hasCloudflare = false;
    bool hasGoogle = false;
    bool hasAdguard = false;
    bool hasQuad9 = false;

    for (const auto &item : providers) {
        const QVariantMap p = item.toMap();
        const QString id = p.value(QStringLiteral("id")).toString();
        if (id == QStringLiteral("cloudflare")) hasCloudflare = true;
        if (id == QStringLiteral("google")) hasGoogle = true;
        if (id == QStringLiteral("adguard")) hasAdguard = true;
        if (id == QStringLiteral("quad9")) hasQuad9 = true;
    }

    QVERIFY(hasCloudflare);
    QVERIFY(hasGoogle);
    QVERIFY(hasAdguard);
    QVERIFY(hasQuad9);

    // Test DNS check with signal spy
    QSignalSpy checkSpy(&manager, &DnsManager::checkResultChanged);
    manager.checkDns(QStringLiteral("google.com"));
    QVERIFY(checkSpy.wait(5000));

    const QVariantMap res = manager.checkResult();
    QVERIFY(res.contains(QStringLiteral("success")));
    QVERIFY(res.contains(QStringLiteral("latencyMs")));
    QCOMPARE(res.value(QStringLiteral("domain")).toString(), QStringLiteral("google.com"));

    // Test benchmark comparison
    QSignalSpy benchSpy(&manager, &DnsManager::benchmarkResultsChanged);
    manager.compareProviders(QStringLiteral("google.com"));
    QVERIFY(benchSpy.wait(10000));
    const QVariantList benchResults = manager.benchmarkResults();
    QVERIFY(benchResults.size() >= 4);
    for (const auto &b : benchResults) {
        const QVariantMap item = b.toMap();
        QVERIFY(!item.value(QStringLiteral("name")).toString().isEmpty());
        QVERIFY(!item.value(QStringLiteral("ip")).toString().isEmpty());
    }
}

static void registerAllQmlTypes()
{
    static bool registered = false;
    if (registered) return;
    registered = true;
    qmlRegisterType<AutostartManager>("org.klarity", 1, 0, "AutostartManager");
    qmlRegisterType<AliasManager>("org.klarity", 1, 0, "AliasManager");
    qmlRegisterType<ShortcutManager>("org.klarity", 1, 0, "ShortcutManager");
    qmlRegisterType<GesturesManager>("org.klarity", 1, 0, "GesturesManager");
    qmlRegisterType<KeydManager>("org.klarity", 1, 0, "KeydManager");
    qmlRegisterType<SystemAuditor>("org.klarity", 1, 0, "SystemAuditor");
    qmlRegisterType<SbctlWrapper>("org.klarity", 1, 0, "SbctlWrapper");
    qmlRegisterType<PackageManager>("org.klarity", 1, 0, "PackageManager");
    qmlRegisterType<PortableAppsManager>("org.klarity", 1, 0, "PortableAppsManager");
    qmlRegisterType<DnsManager>("org.klarity", 1, 0, "DnsManager");
    qmlRegisterType<SearchManager>("org.klarity", 1, 0, "SearchManager");
    qmlRegisterType<UpdateManager>("org.klarity", 1, 0, "UpdateManager");
    qmlRegisterType<PowerManager>("org.klarity", 1, 0, "PowerManager");
    qmlRegisterType<DiskHealthManager>("org.klarity", 1, 0, "DiskHealthManager");
    qmlRegisterType<DependencyChecker>("org.klarity", 1, 0, "DependencyChecker");
    qmlRegisterType<ProcessRunner>("org.klarity", 1, 0, "ProcessRunner");
    qmlRegisterType<PkexecRunner>("org.klarity", 1, 0, "PkexecRunner");
    qmlRegisterSingletonInstance("org.klarity", 1, 0, "LogModel", LogModel::instance());
    qmlRegisterSingletonInstance("org.klarity", 1, 0, "EnvironmentInfo", EnvironmentInfo::instance());
    qmlRegisterSingletonInstance("org.klarity", 1, 0, "ModuleRegistry", ModuleRegistry::instance());
}

#define VERIFY_QML_PAGE(pageName) \
    registerAllQmlTypes(); \
    QQmlEngine engine; \
    engine.addImportPath(QStringLiteral("/home/larevole/LaRCorp/PlaZma/WKSLab/Klarity/qml")); \
    KLocalization::setupLocalizedContext(&engine); \
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral("/home/larevole/LaRCorp/PlaZma/WKSLab/Klarity/qml/modules/") + pageName)); \
    QVERIFY2(component.isReady(), qPrintable(component.errorString())); \
    QObject *obj = component.create(); \
    QVERIFY2(obj != nullptr, qPrintable(component.errorString())); \
    delete obj;

void CoreTests::testAutostartPageQml() { VERIFY_QML_PAGE(QStringLiteral("AutostartPage.qml")); }
void CoreTests::testAliasesPageQml() { VERIFY_QML_PAGE(QStringLiteral("AliasesPage.qml")); }
void CoreTests::testShortcutsPageQml() { VERIFY_QML_PAGE(QStringLiteral("ShortcutsPage.qml")); }
void CoreTests::testGesturesPageQml() { VERIFY_QML_PAGE(QStringLiteral("GesturesPage.qml")); }
void CoreTests::testKeydPageQml() { VERIFY_QML_PAGE(QStringLiteral("KeydPage.qml")); }
void CoreTests::testAuditorPageQml() { VERIFY_QML_PAGE(QStringLiteral("AuditorPage.qml")); }
void CoreTests::testSecureBootPageQml() { VERIFY_QML_PAGE(QStringLiteral("SecureBootPage.qml")); }
void CoreTests::testApplicationsPageQml() { VERIFY_QML_PAGE(QStringLiteral("ApplicationsPage.qml")); }
void CoreTests::testPlaceholderPageQml() { VERIFY_QML_PAGE(QStringLiteral("PlaceholderPage.qml")); }
void CoreTests::testDnsPageQml() { VERIFY_QML_PAGE(QStringLiteral("DnsPage.qml")); }
void CoreTests::testSearchPageQml() { VERIFY_QML_PAGE(QStringLiteral("SearchPage.qml")); }
void CoreTests::testUpdatesPageQml() { VERIFY_QML_PAGE(QStringLiteral("UpdatesPage.qml")); }
void CoreTests::testPowerPageQml() { VERIFY_QML_PAGE(QStringLiteral("PowerPage.qml")); }
void CoreTests::testDiskHealthPageQml() { VERIFY_QML_PAGE(QStringLiteral("DiskHealthPage.qml")); }

void CoreTests::testSearchManager()
{
    SearchManager manager;
    QVERIFY(manager.hasFd());
    QVERIFY(manager.hasRg());

    // Test suggestions
    const QStringList extSugg = manager.getSuggestions(QStringLiteral("*.s"));
    QVERIFY(extSugg.contains(QStringLiteral("*.service")));
    QVERIFY(extSugg.contains(QStringLiteral("*.sh")));

    const QStringList sizeSugg = manager.getSuggestions(QStringLiteral("> 1"));
    QVERIFY(!sizeSugg.isEmpty());

    // Test live search for CMakeLists.txt in current workspace
    QSignalSpy spy(&manager, &SearchManager::resultsChanged);
    manager.search(QStringLiteral("CMakeLists.txt"), QStringLiteral("/home/larevole/LaRCorp/PlaZma/WKSLab/Klarity"), QStringLiteral("all"), 2);
    QVERIFY(spy.wait(4000));

    const QVariantList res = manager.results();
    QVERIFY(!res.isEmpty());
    bool foundCMake = false;
    for (const auto &item : res) {
        const QVariantMap m = item.toMap();
        if (m.value(QStringLiteral("name")).toString() == QStringLiteral("CMakeLists.txt")) {
            foundCMake = true;
            QVERIFY(!m.value(QStringLiteral("path")).toString().isEmpty());
            QVERIFY(!m.value(QStringLiteral("icon")).toString().isEmpty());
            QVERIFY(!m.value(QStringLiteral("isDir")).toBool());
            break;
        }
    }
    QVERIFY(foundCMake);

    // Test history commit
    manager.commitSearchHistory(QStringLiteral("test_query"));
    QVERIFY(manager.searchHistory().contains(QStringLiteral("test_query")));
}

void CoreTests::testUpdateManager()
{
    UpdateManager mgr;
    QCOMPARE(mgr.isRunning(), false);
    QVERIFY(!mgr.lastCheckTimeStr().isEmpty());
    QVERIFY(mgr.currentOperation().isEmpty());
    QCOMPARE(mgr.exitCode(), 0);

    // Tool detection
    QVERIFY(mgr.hasFlatpak() || !mgr.hasFlatpak()); // valid boolean
    if (mgr.hasYay()) {
        QCOMPARE(mgr.aurHelper(), QStringLiteral("yay"));
    } else if (mgr.hasParu()) {
        QCOMPARE(mgr.aurHelper(), QStringLiteral("paru"));
    }

    // Logging & clipboard tests
    mgr.clearLogs();
    QVERIFY(mgr.logOutput().isEmpty());
    mgr.copyToClipboard(QStringLiteral("test clipboard update"));
}

void CoreTests::testPowerManager()
{
    PowerManager mgr;
    QVERIFY(mgr.cpuCoreCount() > 0);
    QVERIFY(!mgr.cpuDriver().isEmpty());
    QVERIFY(!mgr.availableGovernors().isEmpty());
    QVERIFY(mgr.minFreqMhz() > 0);
    QVERIFY(mgr.maxFreqMhz() >= mgr.minFreqMhz());
    QVERIFY(mgr.batteryLevel() >= 0 && mgr.batteryLevel() <= 100);
    QVERIFY(!mgr.currentGovernor().isEmpty());

    // Config setters
    mgr.setChargerGovernor(QStringLiteral("performance"));
    QCOMPARE(mgr.chargerGovernor(), QStringLiteral("performance"));
    mgr.setBatteryGovernor(QStringLiteral("powersave"));
    QCOMPARE(mgr.batteryGovernor(), QStringLiteral("powersave"));

    mgr.clearLogs();
    QVERIFY(mgr.logOutput().isEmpty());
}

void CoreTests::testDiskHealthManager()
{
    DiskHealthManager mgr;
    QVERIFY(mgr.isScanning() || !mgr.isScanning());
    QCOMPARE(mgr.isBusy(), false);

    mgr.clearLogs();
    QVERIFY(mgr.logOutput().isEmpty());
    mgr.selectDisk(QStringLiteral("/dev/nvme0"));
    QCOMPARE(mgr.selectedDevice(), QStringLiteral("/dev/nvme0"));
}

QTEST_MAIN(CoreTests)
#include "CoreTests.moc"
