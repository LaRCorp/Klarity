#include "EnvironmentInfo.h"

#include <QByteArray>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

EnvironmentInfo *EnvironmentInfo::s_instance = nullptr;

EnvironmentInfo *EnvironmentInfo::instance()
{
    if (!s_instance) {
        s_instance = new EnvironmentInfo();
    }
    return s_instance;
}

EnvironmentInfo *EnvironmentInfo::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

EnvironmentInfo::EnvironmentInfo(QObject *parent)
    : QObject(parent)
{
    if (!s_instance) {
        s_instance = this;
    }
    detectEnvironment();
}

void EnvironmentInfo::detectEnvironment()
{
    // Detect Wayland vs X11
    const QByteArray session = qgetenv("XDG_SESSION_TYPE").toLower();
    if (!session.isEmpty()) {
        m_sessionType = QString::fromUtf8(session);
    } else if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) {
        m_sessionType = QStringLiteral("wayland");
    } else if (!qgetenv("DISPLAY").isEmpty()) {
        m_sessionType = QStringLiteral("x11");
    } else {
        m_sessionType = QStringLiteral("unknown");
    }

    // Detect Distro from /etc/os-release
    m_distroId = QStringLiteral("unknown");
    m_distroName = QStringLiteral("Linux");

    QFile osRelease(QStringLiteral("/etc/os-release"));
    if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&osRelease);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.startsWith(QStringLiteral("ID="))) {
                QString val = line.mid(3).remove(QLatin1Char('"')).remove(QLatin1Char('\''));
                m_distroId = val.toLower();
            } else if (line.startsWith(QStringLiteral("PRETTY_NAME="))) {
                QString val = line.mid(12).remove(QLatin1Char('"')).remove(QLatin1Char('\''));
                m_distroName = val;
            }
        }
        osRelease.close();
    }

    // Detect system services and tools
    m_hasSystemdResolved = !QStandardPaths::findExecutable(QStringLiteral("resolvectl")).isEmpty();
    m_hasNetworkManager = !QStandardPaths::findExecutable(QStringLiteral("nmcli")).isEmpty();
    m_hasFlatpak = !QStandardPaths::findExecutable(QStringLiteral("flatpak")).isEmpty();
    m_hasSnap = !QStandardPaths::findExecutable(QStringLiteral("snap")).isEmpty();
    m_hasFd = !QStandardPaths::findExecutable(QStringLiteral("fd")).isEmpty();
    m_hasRipgrep = !QStandardPaths::findExecutable(QStringLiteral("rg")).isEmpty();
}
