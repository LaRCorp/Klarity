cmake_minimum_required(VERSION 3.20)

project(klarity VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    Kirigami
    Auth
    Config
    I18n
)

qt_standard_project_setup()

qt_add_executable(klarity
    src/main.cpp
)

qt_add_qml_module(klarity
    URI org.klarity
    VERSION 1.0
    RESOURCE_PREFIX "/"
    SOURCES
        src/core/ModuleRegistry.h
        src/core/ModuleRegistry.cpp
        src/core/ProcessRunner.h
        src/core/ProcessRunner.cpp
        src/core/DependencyChecker.h
        src/core/DependencyChecker.cpp
        src/core/EnvironmentInfo.h
        src/core/EnvironmentInfo.cpp
        src/core/PkexecRunner.h
        src/core/PkexecRunner.cpp
        src/core/LogModel.h
        src/core/LogModel.cpp
        src/modules/autostart/AutostartManager.h
        src/modules/autostart/AutostartManager.cpp
        src/modules/aliases/AliasManager.h
        src/modules/aliases/AliasManager.cpp
        src/modules/shortcuts/ShortcutManager.h
        src/modules/shortcuts/ShortcutManager.cpp
        src/modules/gestures/GesturesManager.h
        src/modules/gestures/GesturesManager.cpp
        src/modules/keyd/KeydManager.h
        src/modules/keyd/KeydManager.cpp
        src/modules/auditor/SystemAuditor.h
        src/modules/auditor/SystemAuditor.cpp
        src/modules/secureboot/SbctlWrapper.h
        src/modules/secureboot/SbctlWrapper.cpp
        src/modules/applications/PackageManager.h
        src/modules/applications/PackageManager.cpp
        src/modules/applications/PortableAppsManager.h
        src/modules/applications/PortableAppsManager.cpp
        src/modules/dns/DnsManager.h
        src/modules/dns/DnsManager.cpp
        src/modules/search/SearchManager.h
        src/modules/search/SearchManager.cpp
        src/modules/updates/UpdateManager.h
        src/modules/updates/UpdateManager.cpp
        src/modules/power/PowerManager.h
        src/modules/power/PowerManager.cpp
        src/modules/diskhealth/DiskHealthManager.h
        src/modules/diskhealth/DiskHealthManager.cpp
    QML_FILES
        qml/Main.qml
        qml/modules/AutostartPage.qml
        qml/modules/AliasesPage.qml
        qml/modules/ShortcutsPage.qml
        qml/modules/GesturesPage.qml
        qml/modules/KeydPage.qml
        qml/modules/AuditorPage.qml
        qml/modules/SecureBootPage.qml
        qml/modules/ApplicationsPage.qml
        qml/modules/DnsPage.qml
        qml/modules/SearchPage.qml
        qml/modules/UpdatesPage.qml
        qml/modules/PowerPage.qml
        qml/modules/DiskHealthPage.qml
        qml/modules/PlaceholderPage.qml
        qml/components/ConfirmDialog.qml
        qml/components/LogPanel.qml
        qml/components/MissingDependencyCard.qml
        qml/components/ScanProgress.qml
        qml/components/ModuleCard.qml
        qml/components/ErrorBanner.qml
        qml/components/StyledSwitch.qml
        qml/components/StyledSlider.qml
        qml/components/StyledScrollBar.qml
)

target_include_directories(klarity PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/autostart
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/aliases
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/shortcuts
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/gestures
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/keyd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/auditor
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/secureboot
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/applications
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/dns
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/search
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/updates
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/power
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/diskhealth
)

target_link_libraries(klarity PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    KF6::CoreAddons
    KF6::Kirigami
    KF6::AuthCore
    KF6::ConfigCore
    KF6::I18n
    KF6::I18nQml
)

target_compile_definitions(klarity PRIVATE
    KLARITY_LOCALE_DIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
)

ki18n_install(po)

install(TARGETS klarity DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES org.kde.klarity.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Unit Tests
enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(klarity_tests
    tests/CoreTests.cpp
    src/core/LogModel.cpp
    src/core/DependencyChecker.cpp
    src/core/EnvironmentInfo.cpp
    src/core/ProcessRunner.cpp
    src/core/PkexecRunner.cpp
    src/core/ModuleRegistry.cpp
    src/modules/autostart/AutostartManager.cpp
    src/modules/aliases/AliasManager.cpp
    src/modules/shortcuts/ShortcutManager.cpp
    src/modules/gestures/GesturesManager.cpp
    src/modules/keyd/KeydManager.cpp
    src/modules/auditor/SystemAuditor.cpp
    src/modules/secureboot/SbctlWrapper.cpp
    src/modules/applications/PackageManager.cpp
    src/modules/applications/PortableAppsManager.cpp
    src/modules/dns/DnsManager.cpp
    src/modules/search/SearchManager.cpp
    src/modules/updates/UpdateManager.cpp
    src/modules/power/PowerManager.cpp
    src/modules/diskhealth/DiskHealthManager.cpp
)

target_include_directories(klarity_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/autostart
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/aliases
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/shortcuts
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/gestures
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/keyd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/auditor
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/secureboot
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/applications
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/dns
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/search
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/updates
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/power
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/diskhealth
)

target_link_libraries(klarity_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::I18n
    KF6::I18nQml
)

add_test(NAME klarity_core_tests COMMAND klarity_tests)
