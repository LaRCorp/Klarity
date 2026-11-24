#include "AutostartManager.h"
#include "core/ProcessRunner.h"
#include "core/LogModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <KDesktopFile>
#include <KConfigGroup>

AutostartManager::AutostartManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QVariantList AutostartManager::entries() const
{
    QVariantList list;
    list.reserve(m_items.size());
    for (const auto &item : m_items) {
        list.append(item.toMap());
    }
    return list;
}

void AutostartManager::load()
{
    m_isLoading = true;
    emit isLoadingChanged();

    m_items.clear();

    loadDesktopFiles();
    loadSystemdUserServices();

    m_isLoading = false;
    emit isLoadingChanged();
    emit entriesChanged();
}

void AutostartManager::loadDesktopFiles()
{
    const QString userAutostartDir = QDir::homePath() + QStringLiteral("/.config/autostart");
    const QString systemAutostartDir = QStringLiteral("/etc/xdg/autostart");

    QDir userDir(userAutostartDir);
    if (!userDir.exists()) {
        userDir.mkpath(QStringLiteral("."));
    }

    QSet<QString> processedUserFiles;

    // Scan user ~/.config/autostart/
    const QFileInfoList userFiles = userDir.entryInfoList({QStringLiteral("*.desktop"), QStringLiteral("*.desktop.disabled")},
                                                          QDir::Files | QDir::Readable);
    for (const auto &fi : userFiles) {
        const QString filePath = fi.absoluteFilePath();
        const QString fileName = fi.fileName();
        const QString baseDesktopName = fileName.endsWith(QLatin1String(".disabled"))
                                            ? fileName.left(fileName.length() - 9)
                                            : fileName;
        processedUserFiles.insert(baseDesktopName);

        KDesktopFile df(filePath);
        QString name = df.readName();
        if (name.isEmpty()) {
            name = fi.completeBaseName();
        }

        const QString cmd = df.desktopGroup().readEntry("Exec");
        QString icon = df.readIcon();
        if (icon.isEmpty()) {
            icon = QStringLiteral("system-run");
        }
        const QString comment = df.readComment();
        const bool isHidden = df.desktopGroup().readEntry("Hidden", false);
        const bool isDisabledByExt = fileName.endsWith(QLatin1String(".disabled"));
        const bool enabled = !isHidden && !isDisabledByExt;

        AutostartItem item;
        item.id = QStringLiteral("desktop:") + filePath;
        item.name = name;
        item.command = cmd;
        item.icon = icon;
        item.type = QStringLiteral("Desktop");
        item.scope = QStringLiteral("User");
        item.filePath = filePath;
        item.description = comment;
        item.enabled = enabled;

        m_items.append(item);
    }

    // Scan system /etc/xdg/autostart/
    QDir sysDir(systemAutostartDir);
    if (sysDir.exists()) {
        const QFileInfoList sysFiles = sysDir.entryInfoList({QStringLiteral("*.desktop")}, QDir::Files | QDir::Readable);
        for (const auto &fi : sysFiles) {
            const QString fileName = fi.fileName();
            // If user has an override in ~/.config/autostart/, skip the system copy
            if (processedUserFiles.contains(fileName)) {
                continue;
            }

            KDesktopFile df(fi.absoluteFilePath());
            if (df.noDisplay()) {
                // Skip invisible background helpers
                continue;
            }

            QString name = df.readName();
            if (name.isEmpty()) {
                name = fi.completeBaseName();
            }
            const QString cmd = df.desktopGroup().readEntry("Exec");
            QString icon = df.readIcon();
            if (icon.isEmpty()) {
                icon = QStringLiteral("system-run");
            }
            const QString comment = df.readComment();
            const bool isHidden = df.desktopGroup().readEntry("Hidden", false);

            AutostartItem item;
            item.id = QStringLiteral("desktop:") + fi.absoluteFilePath();
            item.name = name;
            item.command = cmd;
            item.icon = icon;
            item.type = QStringLiteral("Desktop");
            item.scope = QStringLiteral("System");
            item.filePath = fi.absoluteFilePath();
            item.description = comment;
            item.enabled = !isHidden;

            m_items.append(item);
        }
    }
}

void AutostartManager::loadSystemdUserServices()
{
    // Query user systemd services using systemctl
    const ProcessResult result = ProcessRunner::run(
        QStringLiteral("systemctl"),
        {QStringLiteral("--user"), QStringLiteral("list-unit-files"), QStringLiteral("--type=service"),
         QStringLiteral("--no-legend"), QStringLiteral("--no-pager")}
    );

    if (result.exitCode != 0) {
        return;
    }

    const QStringList lines = result.stdoutStr.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const auto &rawLine : lines) {
        const QString line = rawLine.trimmed();
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            continue;
        }

        const QString unit = parts[0];
        const QString state = parts[1].toLower();

        // Filter out autostart generated wrappers to avoid duplicates
        if (unit.contains(QStringLiteral("@autostart.service")) ||
            unit.startsWith(QStringLiteral("app-")) ||
            unit.startsWith(QStringLiteral("transient-"))) {
            continue;
        }

        // Only include units with an actionable state (enabled, disabled, or static with preset)
        if (state != QStringLiteral("enabled") && state != QStringLiteral("disabled")) {
            continue;
        }

        AutostartItem item;
        item.id = QStringLiteral("systemd:") + unit;
        item.name = unit;
        item.command = QStringLiteral("systemctl --user start %1").arg(unit);
        item.icon = QStringLiteral("system-run");
        item.type = QStringLiteral("Systemd");
        item.scope = QStringLiteral("User");
        item.filePath = QStringLiteral("~/.config/systemd/user/") + unit;
        item.description = QStringLiteral("Systemd User Service (%1)").arg(unit);
        item.enabled = (state == QStringLiteral("enabled"));

        m_items.append(item);
    }
}

bool AutostartManager::toggleEntry(const QString &id, bool enable)
{
    if (id.startsWith(QStringLiteral("desktop:"))) {
        const QString path = id.mid(8);
        const QFileInfo fi(path);

        if (path.startsWith(QDir::homePath())) {
            // User entry: toggle Hidden property
            KDesktopFile df(path);
            df.desktopGroup().writeEntry("Hidden", !enable);
            df.sync();

            LogModel::instance()->append(QStringLiteral("Autostart"), QStringLiteral("info"),
                                        QStringLiteral("Desktop entry '%1' %2")
                                            .arg(fi.fileName(), enable ? QStringLiteral("enabled") : QStringLiteral("disabled")));
            load();
            emit operationFinished(true, QStringLiteral("Desktop entry status updated."));
            return true;
        } else {
            // System entry: create user override in ~/.config/autostart
            const QString userPath = QDir::homePath() + QStringLiteral("/.config/autostart/") + fi.fileName();
            if (enable) {
                // If enabling and user override exists, remove the override
                if (QFile::exists(userPath)) {
                    QFile::remove(userPath);
                }
            } else {
                // Disabling: copy to user autostart with Hidden=true
                QFile::copy(path, userPath);
                KDesktopFile df(userPath);
                df.desktopGroup().writeEntry("Hidden", true);
                df.sync();
            }

            LogModel::instance()->append(QStringLiteral("Autostart"), QStringLiteral("info"),
                                        QStringLiteral("System desktop entry '%1' %2 via user override")
                                            .arg(fi.fileName(), enable ? QStringLiteral("enabled") : QStringLiteral("disabled")));
            load();
            emit operationFinished(true, QStringLiteral("System entry override updated."));
            return true;
        }
    } else if (id.startsWith(QStringLiteral("systemd:"))) {
        const QString service = id.mid(8);
        const QString action = enable ? QStringLiteral("enable") : QStringLiteral("disable");

        const ProcessResult result = ProcessRunner::run(
            QStringLiteral("systemctl"),
            {QStringLiteral("--user"), action, service}
        );

        const bool ok = (result.exitCode == 0);
        LogModel::instance()->append(
            QStringLiteral("Autostart"),
            ok ? QStringLiteral("info") : QStringLiteral("error"),
            QStringLiteral("Systemd service '%1' %2 (code %3)").arg(service, action, QString::number(result.exitCode))
        );

        load();
        emit operationFinished(ok, ok ? QStringLiteral("Systemd service updated.") : result.stderrStr);
        return ok;
    }

    return false;
}

bool AutostartManager::addDesktopEntry(const QString &name, const QString &command, const QString &icon, const QString &comment)
{
    if (name.trimmed().isEmpty() || command.trimmed().isEmpty()) {
        emit operationFinished(false, QStringLiteral("Name and Command are required."));
        return false;
    }

    QString sanitizedName = name.trimmed().toLower().replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]")), QStringLiteral("_"));
    if (sanitizedName.isEmpty()) {
        sanitizedName = QStringLiteral("custom_autostart");
    }

    const QString userDir = QDir::homePath() + QStringLiteral("/.config/autostart");
    QDir().mkpath(userDir);

    QString filePath = QStringLiteral("%1/%2.desktop").arg(userDir, sanitizedName);
    int counter = 1;
    while (QFile::exists(filePath)) {
        filePath = QStringLiteral("%1/%2_%3.desktop").arg(userDir, sanitizedName, QString::number(counter++));
    }

    KDesktopFile df(filePath);
    auto group = df.desktopGroup();
    group.writeEntry("Type", QStringLiteral("Application"));
    group.writeEntry("Name", name.trimmed());
    group.writeEntry("Exec", command.trimmed());
    group.writeEntry("Icon", icon.trimmed().isEmpty() ? QStringLiteral("system-run") : icon.trimmed());
    if (!comment.trimmed().isEmpty()) {
        group.writeEntry("Comment", comment.trimmed());
    }
    group.writeEntry("Terminal", false);
    group.writeEntry("Hidden", false);
    df.sync();

    LogModel::instance()->append(QStringLiteral("Autostart"), QStringLiteral("info"),
                                QStringLiteral("Created autostart desktop entry: %1").arg(filePath));

    load();
    emit operationFinished(true, QStringLiteral("Application added to autostart."));
    return true;
}

bool AutostartManager::removeEntry(const QString &id)
{
    if (id.startsWith(QStringLiteral("desktop:"))) {
        const QString path = id.mid(8);
        if (path.startsWith(QDir::homePath())) {
            const bool ok = QFile::remove(path);
            LogModel::instance()->append(
                QStringLiteral("Autostart"),
                ok ? QStringLiteral("info") : QStringLiteral("error"),
                QStringLiteral("Deleted user autostart entry: %1").arg(path)
            );
            load();
            emit operationFinished(ok, ok ? QStringLiteral("Entry deleted.") : QStringLiteral("Failed to delete entry."));
            return ok;
        } else {
            // System entry cannot be deleted by user directly; disable it instead
            return toggleEntry(id, false);
        }
    } else if (id.startsWith(QStringLiteral("systemd:"))) {
        // Disable service
        return toggleEntry(id, false);
    }

    return false;
}
