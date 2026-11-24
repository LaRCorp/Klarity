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
        QStringLiteral("0.1.0"),
        i18n("Modular GUI-first administration tool for Linux / KDE Plasma 6"),
        KAboutLicense::GPL_V3,
        i18n("Copyright 2026 LaRCorp")
    );
    aboutData.addAuthor(i18n("LaRCorp"), i18n("Developer"), QStringLiteral("dev@larcorp.org"));
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
