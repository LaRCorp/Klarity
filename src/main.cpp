#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>

#include <KAboutData>
#include <KLocalizedString>
#include <klocalizedqmlcontext.h>

#include "core/ModuleRegistry.h"
#include "core/EnvironmentInfo.h"
#include "core/DependencyChecker.h"
#include "core/ProcessRunner.h"
#include "core/PkexecRunner.h"
#include "core/LogModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("klarity");

    KAboutData aboutData(
        QStringLiteral("klarity"),
        i18n("Klarity"),
        QStringLiteral("1.0"),
        i18n("Modular GUI-first administration tool for Linux / KDE Plasma 6"),
        KAboutLicense::GPL_V3,
        i18n("Copyright 2026 LaRCorp")
    );
    aboutData.addAuthor(i18n("LaRCorp"), i18n("Developer"), QStringLiteral("dev@larcorp.org"));
    aboutData.addCredit(i18n("Morten Linderud"), i18n("Author of sbctl (Secure Boot Key Manager)"), QStringLiteral(), QStringLiteral("https://github.com/Foxboron/sbctl"));
    aboutData.addCredit(i18n("Rahul Rajan"), i18n("Author of keyd (Keyboard remapping daemon)"), QStringLiteral(), QStringLiteral("https://github.com/rvaiya/keyd"));
    aboutData.addCredit(i18n("Adnan Hodzic"), i18n("Author of auto-cpufreq (CPU speed & power optimizer)"), QStringLiteral(), QStringLiteral("https://github.com/AdnanHodzic/auto-cpufreq"));
    aboutData.addCredit(i18n("Mark Blakeney"), i18n("Author of libinput-gestures"), QStringLiteral(), QStringLiteral("https://github.com/bulletmark/libinput-gestures"));
    aboutData.addCredit(i18n("Smartmontools Team"), i18n("S.M.A.R.T. disk monitoring utilities (smartctl)"), QStringLiteral(), QStringLiteral("https://www.smartmontools.org"));
    aboutData.addCredit(i18n("Andrew Gallant (BurntSushi)"), i18n("Author of ripgrep"), QStringLiteral(), QStringLiteral("https://github.com/BurntSushi/ripgrep"));
    aboutData.addCredit(i18n("David Peter (sharkdp)"), i18n("Author of fd"), QStringLiteral(), QStringLiteral("https://github.com/sharkdp/fd"));
    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-system")));

    QQmlApplicationEngine engine;

    // Set up KI18n for QML (makes i18n() available in all QML files)
    KLocalization::setupLocalizedContext(&engine);

    // Load Main.qml from the registered QML module
    const QUrl url(QStringLiteral("qrc:/org/klarity/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        engine.loadFromModule("org.klarity", "Main");
    }

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
