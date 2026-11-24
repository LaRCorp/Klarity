#include "ShortcutManager.h"
#include "core/LogModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QKeySequence>
#include <QPointer>
#include <QThread>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

static QVector<ShortcutItem> s_cachedShortcuts;
static int s_cachedConflictCount = 0;
static bool s_hasCachedData = false;

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
{
    if (s_hasCachedData) {
        m_items = s_cachedShortcuts;
        m_conflictCount = s_cachedConflictCount;
    } else {
        load();
    }
}

ShortcutManager::~ShortcutManager()
{
    if (m_loadWorker && m_loadWorker->isRunning()) {
        m_loadWorker->wait();
    }
}

QVariantList ShortcutManager::shortcuts() const
{
    QVariantList list;
    list.reserve(m_items.size());
    for (const auto &item : m_items) {
        list.append(item.toMap());
    }
    return list;
}

QVariantList ShortcutManager::conflicts() const
{
    QVariantList list;
    for (const auto &item : m_items) {
        if (item.isConflict) {
            list.append(item.toMap());
        }
    }
    return list;
}

void ShortcutManager::load()
{
    m_isLoading = true;
    emit isLoadingChanged();

    m_items.clear();
    m_conflictCount = 0;

    loadKGlobalShortcuts();
    loadPlasmaAppletShortcuts();
    loadKWinRcShortcuts();
    loadApplicationRcShortcuts();

    detectConflicts();

    s_cachedShortcuts = m_items;
    s_cachedConflictCount = m_conflictCount;
    s_hasCachedData = true;

    m_isLoading = false;
    emit isLoadingChanged();
    emit shortcutsChanged();
}

void ShortcutManager::loadAsync()
{
    if (m_isLoading) return;
    m_isLoading = true;
    emit isLoadingChanged();

    m_loadWorker = QThread::create([this]() {
        QPointer<ShortcutManager> guard(this);
        ShortcutManager loader;
        loader.load();

        if (guard) {
            QMetaObject::invokeMethod(this, [guard, newItems = loader.m_items, newConflicts = loader.m_conflictCount]() {
                if (!guard) return;
                guard->m_items = newItems;
                guard->m_conflictCount = newConflicts;
                s_cachedShortcuts = newItems;
                s_cachedConflictCount = newConflicts;
                s_hasCachedData = true;
                guard->m_isLoading = false;
                guard->m_loadWorker = nullptr;
                emit guard->isLoadingChanged();
                emit guard->shortcutsChanged();
            }, Qt::QueuedConnection);
        }
    });

    connect(m_loadWorker, &QThread::finished, m_loadWorker, &QObject::deleteLater);
    m_loadWorker->start();
}

void ShortcutManager::loadKGlobalShortcuts()
{
    const QString filePath = QDir::homePath() + QStringLiteral("/.config/kglobalshortcutsrc");
    if (!QFile::exists(filePath)) {
        return;
    }

    KConfig config(filePath, KConfig::SimpleConfig);
    const QStringList groups = config.groupList();

    for (const auto &groupName : groups) {
        if (groupName.endsWith(QStringLiteral("[$InverseAction]"))) {
            continue;
        }

        KConfigGroup grp(&config, groupName);
        const QString friendlyName = grp.readEntry("_k_friendly_name", QString());

        QString source = QStringLiteral("Global");
        QString category = i18n("Global Shortcuts");
        QString icon = QStringLiteral("configure");

        const QString lowerGroup = groupName.toLower();
        if (lowerGroup == QStringLiteral("kwin")) {
            source = QStringLiteral("KWin");
            category = i18n("Window Management");
            icon = QStringLiteral("preferences-system-windows");
        } else if (lowerGroup == QStringLiteral("kmix") || lowerGroup == QStringLiteral("mediacontrol") ||
                   lowerGroup.contains(QStringLiteral("player")) || lowerGroup == QStringLiteral("strawberry")) {
            source = QStringLiteral("Media");
            category = i18n("Audio & Media");
            icon = QStringLiteral("audio-volume-high");
        } else if (lowerGroup == QStringLiteral("ksmserver") || lowerGroup == QStringLiteral("org_kde_powerdevil") ||
                   lowerGroup.contains(QStringLiteral("layout switcher"))) {
            source = QStringLiteral("Session");
            category = i18n("Session & System");
            icon = QStringLiteral("system-lock-screen");
        } else if (lowerGroup.startsWith(QStringLiteral("services")) || lowerGroup.contains(QStringLiteral(".desktop"))) {
            source = QStringLiteral("Applications");
            category = i18n("Application Launchers");
            icon = QStringLiteral("applications-other");
        } else if (lowerGroup == QStringLiteral("plasmashell") || lowerGroup == QStringLiteral("activitymanager")) {
            source = QStringLiteral("Plasma");
            category = i18n("Shell & Workspace");
            icon = QStringLiteral("plasma");
        }

        const QStringList keys = grp.keyList();
        for (const auto &key : keys) {
            if (key.startsWith(QLatin1String("_k_"))) {
                continue;
            }

            const QString rawVal = grp.readEntry(key, QString());
            const QStringList parts = rawVal.split(QLatin1Char(','));

            QString currentKey;
            QString defaultKey;
            QString desc;

            if (parts.size() >= 1) currentKey = parts[0].trimmed();
            if (parts.size() >= 2) defaultKey = parts[1].trimmed();
            if (parts.size() >= 3) {
                desc = parts.mid(2).join(QLatin1Char(',')).trimmed();
            }

            // Normalization of tabs (primary vs secondary shortcut)
            currentKey.replace(QLatin1Char('\t'), QStringLiteral(" / "));
            defaultKey.replace(QLatin1Char('\t'), QStringLiteral(" / "));

            ShortcutItem item;
            item.id = QStringLiteral("global:%1:%2").arg(groupName, key);
            item.action = key;
            item.description = desc.isEmpty() ? (!friendlyName.isEmpty() ? friendlyName : key) : desc;
            item.shortcut = currentKey;
            item.defaultShortcut = defaultKey;
            item.source = source;
            item.category = category;
            item.icon = icon;
            item.filePath = filePath;

            m_items.append(item);
        }
    }
}

void ShortcutManager::loadPlasmaAppletShortcuts()
{
    const QString filePath = QDir::homePath() + QStringLiteral("/.config/plasma-org.kde.plasma.desktop-appletsrc");
    if (!QFile::exists(filePath)) {
        return;
    }

    KConfig config(filePath, KConfig::SimpleConfig);
    const QStringList groups = config.groupList();

    for (const auto &groupName : groups) {
        if (!groupName.endsWith(QStringLiteral("[Shortcuts]"))) {
            continue;
        }

        KConfigGroup grp(&config, groupName);
        const QString globalKey = grp.readEntry("global", QString());
        if (globalKey.isEmpty() || globalKey == QStringLiteral("none")) {
            continue;
        }

        ShortcutItem item;
        item.id = QStringLiteral("applet:%1").arg(groupName);
        item.action = QStringLiteral("Applet Activation (%1)").arg(globalKey);
        item.description = i18n("Plasma Desktop Widget Shortcut");
        item.shortcut = globalKey;
        item.defaultShortcut = QStringLiteral("none");
        item.source = QStringLiteral("Plasma");
        item.category = i18n("Widgets & Applets");
        item.icon = QStringLiteral("plasma");
        item.filePath = filePath;

        m_items.append(item);
    }
}

void ShortcutManager::loadKWinRcShortcuts()
{
    const QString filePath = QDir::homePath() + QStringLiteral("/.config/kwinrc");
    if (!QFile::exists(filePath)) {
        return;
    }

    KConfig config(filePath, KConfig::SimpleConfig);
    if (!config.hasGroup(QStringLiteral("Shortcuts"))) {
        return;
    }

    KConfigGroup grp(&config, QStringLiteral("Shortcuts"));
    const QStringList keys = grp.keyList();

    for (const auto &key : keys) {
        const QString val = grp.readEntry(key, QString());
        if (val.isEmpty() || val == QStringLiteral("none")) {
            continue;
        }

        // Avoid exact duplicates if already read from kglobalshortcutsrc
        bool exists = false;
        for (const auto &it : m_items) {
            if (it.action == key && it.source == QStringLiteral("KWin")) {
                exists = true;
                break;
            }
        }
        if (exists) {
            continue;
        }

        ShortcutItem item;
        item.id = QStringLiteral("kwinrc:%1").arg(key);
        item.action = key;
        item.description = QStringLiteral("KWin Window Management (%1)").arg(key);
        item.shortcut = val;
        item.defaultShortcut = QStringLiteral("none");
        item.source = QStringLiteral("KWin");
        item.category = i18n("Window Management");
        item.icon = QStringLiteral("preferences-system-windows");
        item.filePath = filePath;

        m_items.append(item);
    }
}

void ShortcutManager::loadApplicationRcShortcuts()
{
    const QDir configDir(QDir::homePath() + QStringLiteral("/.config"));
    const QFileInfoList rcFiles = configDir.entryInfoList({QStringLiteral("*rc")}, QDir::Files | QDir::Readable);

    for (const auto &fi : rcFiles) {
        const QString fileName = fi.fileName();
        if (fileName == QStringLiteral("kglobalshortcutsrc") ||
            fileName == QStringLiteral("kwinrc") ||
            fileName == QStringLiteral("plasma-org.kde.plasma.desktop-appletsrc") ||
            fileName == QStringLiteral("gtkrc")) {
            continue;
        }

        KConfig config(fi.absoluteFilePath(), KConfig::SimpleConfig);
        const QStringList groups = config.groupList();

        for (const auto &groupName : groups) {
            if (!groupName.endsWith(QStringLiteral("Shortcuts")) &&
                !groupName.endsWith(QStringLiteral("Shortcut"))) {
                continue;
            }

            KConfigGroup grp(&config, groupName);
            const QStringList keys = grp.keyList();
            for (const auto &key : keys) {
                const QString val = grp.readEntry(key, QString());
                if (val.isEmpty() || val == QStringLiteral("none")) {
                    continue;
                }

                const QString appName = fi.baseName().remove(QStringLiteral("rc"));

                ShortcutItem item;
                item.id = QStringLiteral("app:%1:%2").arg(appName, key);
                item.action = key;
                item.description = QStringLiteral("%1 action in %2").arg(key, appName);
                item.shortcut = val;
                item.defaultShortcut = QStringLiteral("none");
                item.source = QStringLiteral("App: ") + appName;
                item.category = QStringLiteral("Applications");
                item.icon = QStringLiteral("preferences-desktop-keyboard-shortcuts");
                item.filePath = fi.absoluteFilePath();

                m_items.append(item);
            }
        }
    }
}

void ShortcutManager::detectConflicts()
{
    // Map normalized sequence string to list of item indices
    QHash<QString, QVector<int>> seqMap;

    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i].isConflict = false;
        m_items[i].conflictingWith.clear();

        const QString &s = m_items[i].shortcut;
        if (s.isEmpty() || s == QStringLiteral("none")) {
            continue;
        }

        // Handle combinations separated by " / "
        const QStringList subKeys = s.split(QStringLiteral(" / "));
        for (const auto &subKey : subKeys) {
            const QString trimmed = subKey.trimmed();
            if (trimmed.isEmpty() || trimmed == QStringLiteral("none")) {
                continue;
            }

            const QKeySequence ks(trimmed);
            const QString normalized = ks.toString(QKeySequence::PortableText);
            if (!normalized.isEmpty()) {
                seqMap[normalized].append(i);
            }
        }
    }

    m_conflictCount = 0;
    for (auto it = seqMap.cbegin(); it != seqMap.cend(); ++it) {
        const auto &indices = it.value();
        if (indices.size() > 1) {
            m_conflictCount++;

            for (int i = 0; i < indices.size(); ++i) {
                const int currentIdx = indices[i];
                m_items[currentIdx].isConflict = true;

                for (int j = 0; j < indices.size(); ++j) {
                    if (i != j) {
                        const int otherIdx = indices[j];
                        const QString desc = m_items[otherIdx].description.isEmpty()
                                                 ? m_items[otherIdx].action
                                                 : m_items[otherIdx].description;
                        const QString label = QStringLiteral("%1 [%2]").arg(desc, m_items[otherIdx].source);
                        if (!m_items[currentIdx].conflictingWith.contains(label)) {
                            m_items[currentIdx].conflictingWith.append(label);
                        }
                    }
                }
            }
        }
    }

    if (m_conflictCount > 0) {
        LogModel::instance()->append(
            QStringLiteral("Shortcuts"),
            QStringLiteral("warn"),
            QStringLiteral("Detected %1 shortcut key conflicts across system configuration").arg(m_conflictCount)
        );
    }
}

QString ShortcutManager::checkCollision(const QString &keyCombo, const QString &excludeId) const
{
    if (keyCombo.trimmed().isEmpty() || keyCombo.trimmed().toLower() == QStringLiteral("none")) {
        return QString();
    }

    const QKeySequence targetKs(keyCombo.trimmed());
    const QString targetNorm = targetKs.toString(QKeySequence::PortableText);
    if (targetNorm.isEmpty()) {
        return QString();
    }

    for (const auto &item : m_items) {
        if (!excludeId.isEmpty() && item.id == excludeId) {
            continue;
        }

        const QStringList subKeys = item.shortcut.split(QStringLiteral(" / "));
        for (const auto &subKey : subKeys) {
            const QKeySequence ks(subKey.trimmed());
            if (ks.toString(QKeySequence::PortableText) == targetNorm) {
                return QStringLiteral("%1 (%2)").arg(
                    item.description.isEmpty() ? item.action : item.description,
                    item.source
                );
            }
        }
    }

    return QString();
}

bool ShortcutManager::reassignShortcut(const QString &id, const QString &newShortcut)
{
    const QString trimmedShortcut = newShortcut.trimmed().isEmpty() ? QStringLiteral("none") : newShortcut.trimmed();

    if (id.startsWith(QStringLiteral("global:"))) {
        const QStringList idParts = id.mid(7).split(QLatin1Char(':'));
        if (idParts.size() < 2) {
            emit operationFinished(false, QStringLiteral("Invalid shortcut identifier."));
            return false;
        }

        const QString groupName = idParts[0];
        const QString keyName = idParts.mid(1).join(QLatin1Char(':'));

        const QString filePath = QDir::homePath() + QStringLiteral("/.config/kglobalshortcutsrc");
        KConfig config(filePath, KConfig::SimpleConfig);
        if (!config.hasGroup(groupName)) {
            emit operationFinished(false, QStringLiteral("Configuration group not found."));
            return false;
        }

        KConfigGroup grp(&config, groupName);
        const QString oldVal = grp.readEntry(keyName, QString());
        const QStringList parts = oldVal.split(QLatin1Char(','));

        QString defaultKey = QStringLiteral("none");
        QString desc;
        if (parts.size() >= 2) defaultKey = parts[1].trimmed();
        if (parts.size() >= 3) desc = parts.mid(2).join(QLatin1Char(',')).trimmed();

        const QString newVal = QStringLiteral("%1,%2,%3").arg(trimmedShortcut, defaultKey, desc);
        grp.writeEntry(keyName, newVal);
        config.sync();

        LogModel::instance()->append(
            QStringLiteral("Shortcuts"),
            QStringLiteral("info"),
            QStringLiteral("Reassigned shortcut for '%1' to '%2'").arg(keyName, trimmedShortcut)
        );

        load();
        emit operationFinished(true, i18n("Shortcut updated successfully."));
        return true;
    }

    if (id.startsWith(QStringLiteral("kwinrc:"))) {
        const QString keyName = id.mid(7);
        const QString filePath = QDir::homePath() + QStringLiteral("/.config/kwinrc");
        KConfig config(filePath, KConfig::SimpleConfig);
        KConfigGroup grp(&config, QStringLiteral("Shortcuts"));

        if (trimmedShortcut == QStringLiteral("none")) {
            grp.deleteEntry(keyName);
        } else {
            grp.writeEntry(keyName, trimmedShortcut);
        }
        config.sync();

        LogModel::instance()->append(
            QStringLiteral("Shortcuts"),
            QStringLiteral("info"),
            QStringLiteral("Reassigned KWin shortcut for '%1' to '%2'").arg(keyName, trimmedShortcut)
        );

        load();
        emit operationFinished(true, i18n("Shortcut updated successfully."));
        return true;
    }

    if (id.startsWith(QStringLiteral("applet:"))) {
        const QString groupName = id.mid(7);
        const QString filePath = QDir::homePath() + QStringLiteral("/.config/plasma-org.kde.plasma.desktop-appletsrc");
        KConfig config(filePath, KConfig::SimpleConfig);
        KConfigGroup grp(&config, groupName);

        if (trimmedShortcut == QStringLiteral("none")) {
            grp.deleteEntry(QStringLiteral("global"));
        } else {
            grp.writeEntry(QStringLiteral("global"), trimmedShortcut);
        }
        config.sync();

        LogModel::instance()->append(
            QStringLiteral("Shortcuts"),
            QStringLiteral("info"),
            QStringLiteral("Reassigned applet shortcut for '%1' to '%2'").arg(groupName, trimmedShortcut)
        );

        load();
        emit operationFinished(true, i18n("Shortcut updated successfully."));
        return true;
    }

    emit operationFinished(false, i18n("This shortcut source cannot be modified directly."));
    return false;
}

bool ShortcutManager::unbindShortcut(const QString &id)
{
    return reassignShortcut(id, QStringLiteral("none"));
}
