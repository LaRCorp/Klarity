#include "GesturesManager.h"
#include "core/LogModel.h"
#include "core/ProcessRunner.h"

#include <KLocalizedString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>

GesturesManager::GesturesManager(QObject *parent)
    : QObject(parent)
{
    load();
    checkServiceStatus();
}

QString GesturesManager::configPath() const
{
    return QDir::homePath() + QStringLiteral("/.config/libinput-gestures.conf");
}

QVariantList GesturesManager::gestures() const
{
    QVariantList list;
    list.reserve(m_items.size());
    for (const auto &item : m_items) {
        list.append(item.toMap());
    }
    return list;
}

QString GesturesManager::computeFriendlyName(const QString &cmd) const
{
    const QString trimmed = cmd.trimmed();
    if (trimmed.contains(QStringLiteral("previousmedia"))) {
        return i18n("Previous Media Track");
    }
    if (trimmed.contains(QStringLiteral("nextmedia"))) {
        return i18n("Next Media Track");
    }
    if (trimmed.contains(QStringLiteral("playpausemedia"))) {
        return i18n("Play / Pause Media");
    }
    if (trimmed.contains(QStringLiteral("set-sink-volume")) && trimmed.contains(QStringLiteral("+"))) {
        return i18n("Increase Volume (+5%)");
    }
    if (trimmed.contains(QStringLiteral("set-sink-volume")) && trimmed.contains(QStringLiteral("-"))) {
        return i18n("Decrease Volume (-5%)");
    }
    if (trimmed.contains(QStringLiteral("set-sink-mute"))) {
        return i18n("Toggle Mute Audio");
    }
    if (trimmed.contains(QStringLiteral("Switch One Desktop to the Left"))) {
        return i18n("Switch Workspace Left");
    }
    if (trimmed.contains(QStringLiteral("Switch One Desktop to the Right"))) {
        return i18n("Switch Workspace Right");
    }
    if (trimmed.contains(QStringLiteral("Overview"))) {
        return i18n("KWin Overview (Window Grid)");
    }
    if (trimmed.contains(QStringLiteral("Show Desktop"))) {
        return i18n("Show / Toggle Desktop");
    }
    if (trimmed.contains(QStringLiteral("spectacle"))) {
        return i18n("Take Screenshot (Spectacle)");
    }

    return trimmed;
}

QString GesturesManager::computeIcon(const QString &type, const QString &direction) const
{
    const QString d = direction.toLower();
    if (d == QStringLiteral("left")) return QStringLiteral("go-previous");
    if (d == QStringLiteral("right")) return QStringLiteral("go-next");
    if (d == QStringLiteral("up")) return QStringLiteral("go-up");
    if (d == QStringLiteral("down")) return QStringLiteral("go-down");
    if (d == QStringLiteral("in")) return QStringLiteral("zoom-in");
    if (d == QStringLiteral("out")) return QStringLiteral("zoom-out");
    if (type == QStringLiteral("pinch")) return QStringLiteral("input-touchpad");
    return QStringLiteral("input-touchpad");
}

void GesturesManager::load()
{
    m_isLoading = true;
    emit isLoadingChanged();
    m_items.clear();

    const QString path = configPath();
    QFile file(path);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        int index = 0;

        // Matches lines like:
        // gesture swipe left 3 qdbus ...
        // # gesture swipe right 3 qdbus ...
        const QRegularExpression re(QStringLiteral(R"(^(#\s*)?gesture\s+(swipe|pinch)\s+([a-zA-Z_-]+)\s+(\d+)\s+(.+)$)"),
                                    QRegularExpression::CaseInsensitiveOption);

        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }

            const auto match = re.match(line);
            if (match.hasMatch()) {
                const bool isCommented = !match.captured(1).isEmpty();
                const QString type = match.captured(2).toLower();
                const QString direction = match.captured(3).toLower();
                const int fingers = match.captured(4).toInt();
                const QString command = match.captured(5).trimmed();

                GestureItem item;
                item.id = QStringLiteral("gesture_%1").arg(index++);
                item.type = type;
                item.direction = direction;
                item.fingers = fingers;
                item.command = command;
                item.enabled = !isCommented;
                item.friendlyName = computeFriendlyName(command);
                item.icon = computeIcon(type, direction);

                m_items.append(item);
            }
        }
        file.close();
    } else {
        // If file doesn't exist, create an empty one or load defaults
        LogModel::instance()->append(
            QStringLiteral("Gestures"),
            QStringLiteral("info"),
            QStringLiteral("libinput-gestures.conf not found, ready to create at %1").arg(path)
        );
    }

    m_isLoading = false;
    emit isLoadingChanged();
    emit gesturesChanged();
}

bool GesturesManager::saveToFile()
{
    const QString path = configPath();
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit operationFinished(false, QStringLiteral("Failed to open %1 for writing.").arg(path));
        return false;
    }

    QTextStream out(&file);
    out << "# Generated by Klarity - Linux Control Center\n";
    out << "# Configuration for libinput-gestures\n\n";

    for (const auto &item : m_items) {
        if (!item.enabled) {
            out << "# ";
        }
        out << "gesture " << item.type << " " << item.direction << " " << item.fingers << " " << item.command << "\n";
    }

    file.close();

    LogModel::instance()->append(
        QStringLiteral("Gestures"),
        QStringLiteral("info"),
        QStringLiteral("Saved %1 gesture(s) to %2").arg(m_items.size()).arg(path)
    );

    return true;
}

bool GesturesManager::addGesture(const QString &type, const QString &direction, int fingers, const QString &command)
{
    if (command.trimmed().isEmpty()) {
        emit operationFinished(false, QStringLiteral("Command cannot be empty."));
        return false;
    }

    GestureItem item;
    item.id = QStringLiteral("gesture_%1").arg(QDateTime::currentMSecsSinceEpoch());
    item.type = type.toLower().trimmed();
    item.direction = direction.toLower().trimmed();
    item.fingers = fingers > 0 ? fingers : 3;
    item.command = command.trimmed();
    item.enabled = true;
    item.friendlyName = computeFriendlyName(item.command);
    item.icon = computeIcon(item.type, item.direction);

    m_items.append(item);

    if (saveToFile()) {
        emit gesturesChanged();
        restartService();
        emit operationFinished(true, QStringLiteral("Gesture added and service restarted."));
        return true;
    }

    return false;
}

bool GesturesManager::editGesture(const QString &id, const QString &type, const QString &direction, int fingers, const QString &command, bool enabled)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            m_items[i].type = type.toLower().trimmed();
            m_items[i].direction = direction.toLower().trimmed();
            m_items[i].fingers = fingers > 0 ? fingers : 3;
            m_items[i].command = command.trimmed();
            m_items[i].enabled = enabled;
            m_items[i].friendlyName = computeFriendlyName(m_items[i].command);
            m_items[i].icon = computeIcon(m_items[i].type, m_items[i].direction);

            if (saveToFile()) {
                emit gesturesChanged();
                restartService();
                emit operationFinished(true, QStringLiteral("Gesture updated and service restarted."));
                return true;
            }
            return false;
        }
    }

    emit operationFinished(false, QStringLiteral("Gesture not found."));
    return false;
}

bool GesturesManager::deleteGesture(const QString &id)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            m_items.removeAt(i);
            if (saveToFile()) {
                emit gesturesChanged();
                restartService();
                emit operationFinished(true, QStringLiteral("Gesture deleted and service restarted."));
                return true;
            }
            return false;
        }
    }

    emit operationFinished(false, QStringLiteral("Gesture not found."));
    return false;
}

bool GesturesManager::toggleGesture(const QString &id)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            m_items[i].enabled = !m_items[i].enabled;
            if (saveToFile()) {
                emit gesturesChanged();
                restartService();
                emit operationFinished(true, m_items[i].enabled ? QStringLiteral("Gesture enabled.") : QStringLiteral("Gesture disabled."));
                return true;
            }
            return false;
        }
    }

    return false;
}

void GesturesManager::checkServiceStatus()
{
    QProcess proc;
    proc.start(QStringLiteral("pgrep"), {QStringLiteral("-x"), QStringLiteral("libinput-gestures")});
    proc.waitForFinished(1000);

    const bool running = (proc.exitCode() == 0);
    if (m_isServiceRunning != running) {
        m_isServiceRunning = running;
        emit serviceStatusChanged();
    }
}

void GesturesManager::restartService()
{
    LogModel::instance()->append(
        QStringLiteral("Gestures"),
        QStringLiteral("info"),
        QStringLiteral("Restarting libinput-gestures service...")
    );

    auto runner = new ProcessRunner(this);
    connect(runner, &ProcessRunner::finished, this, [this](int exitCode, const QString &output) {
        Q_UNUSED(output)
        checkServiceStatus();
        if (exitCode == 0) {
            LogModel::instance()->append(QStringLiteral("Gestures"), QStringLiteral("info"), QStringLiteral("libinput-gestures restarted successfully"));
        } else {
            LogModel::instance()->append(QStringLiteral("Gestures"), QStringLiteral("warn"), QStringLiteral("libinput-gestures-setup restart exited with code %1").arg(exitCode));
        }
    });

    runner->runAsync(QStringLiteral("libinput-gestures-setup"), {QStringLiteral("restart")});
}

QVariantList GesturesManager::presets() const
{
    return {
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Previous Media Track")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/mediacontrol invokeShortcut previousmedia")},
            {QStringLiteral("category"), QStringLiteral("Media")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Next Media Track")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/mediacontrol invokeShortcut nextmedia")},
            {QStringLiteral("category"), QStringLiteral("Media")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Play / Pause Media")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/mediacontrol invokeShortcut playpausemedia")},
            {QStringLiteral("category"), QStringLiteral("Media")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Increase Volume (+5%)")},
            {QStringLiteral("command"), QStringLiteral("pactl set-sink-volume @DEFAULT_SINK@ +5%")},
            {QStringLiteral("category"), QStringLiteral("Audio")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Decrease Volume (-5%)")},
            {QStringLiteral("command"), QStringLiteral("pactl set-sink-volume @DEFAULT_SINK@ -5%")},
            {QStringLiteral("category"), QStringLiteral("Audio")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Toggle Mute Audio")},
            {QStringLiteral("command"), QStringLiteral("pactl set-sink-mute @DEFAULT_SINK@ toggle")},
            {QStringLiteral("category"), QStringLiteral("Audio")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Switch Workspace Left")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/kwin invokeShortcut \"Switch One Desktop to the Left\"")},
            {QStringLiteral("category"), QStringLiteral("KWin")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Switch Workspace Right")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/kwin invokeShortcut \"Switch One Desktop to the Right\"")},
            {QStringLiteral("category"), QStringLiteral("KWin")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Overview (Window Grid)")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/kwin invokeShortcut \"Overview\"")},
            {QStringLiteral("category"), QStringLiteral("KWin")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Show Desktop")},
            {QStringLiteral("command"), QStringLiteral("qdbus org.kde.kglobalaccel /component/kwin invokeShortcut \"Show Desktop\"")},
            {QStringLiteral("category"), QStringLiteral("KWin")}
        },
        QVariantMap{
            {QStringLiteral("name"), QStringLiteral("Take Screenshot (Spectacle)")},
            {QStringLiteral("command"), QStringLiteral("spectacle")},
            {QStringLiteral("category"), QStringLiteral("Tools")}
        }
    };
}
