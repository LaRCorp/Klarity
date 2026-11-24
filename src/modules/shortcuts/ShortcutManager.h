#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct ShortcutItem
{
    QString id;
    QString action;
    QString description;
    QString shortcut;
    QString defaultShortcut;
    QString source;       // "KWin", "Global", "Media", "Session", "Plasma", "Applications"
    QString category;     // "Window Management", "Audio & Media", "Session & Power", etc.
    QString icon;
    QString filePath;
    bool isConflict = false;
    QStringList conflictingWith;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("action"), action},
            {QStringLiteral("description"), description},
            {QStringLiteral("shortcut"), shortcut},
            {QStringLiteral("defaultShortcut"), defaultShortcut},
            {QStringLiteral("source"), source},
            {QStringLiteral("category"), category},
            {QStringLiteral("icon"), icon},
            {QStringLiteral("filePath"), filePath},
            {QStringLiteral("isConflict"), isConflict},
            {QStringLiteral("conflictingWith"), conflictingWith}
        };
    }
};

class ShortcutManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList shortcuts READ shortcuts NOTIFY shortcutsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(int conflictCount READ conflictCount NOTIFY shortcutsChanged)

public:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager() override;

    QVariantList shortcuts() const;
    bool isLoading() const { return m_isLoading; }
    int conflictCount() const { return m_conflictCount; }

    Q_INVOKABLE void load();
    Q_INVOKABLE void loadAsync();
    Q_INVOKABLE bool hasConflicts() const { return m_conflictCount > 0; }
    Q_INVOKABLE QVariantList conflicts() const;
    Q_INVOKABLE bool reassignShortcut(const QString &id, const QString &newShortcut);
    Q_INVOKABLE bool unbindShortcut(const QString &id);
    Q_INVOKABLE QString checkCollision(const QString &keyCombo, const QString &excludeId = QString()) const;

signals:
    void shortcutsChanged();
    void isLoadingChanged();
    void operationFinished(bool success, const QString &message);

private:
    void loadKGlobalShortcuts();
    void loadPlasmaAppletShortcuts();
    void loadApplicationRcShortcuts();
    void loadKWinRcShortcuts();
    void detectConflicts();

    QVector<ShortcutItem> m_items;
    bool m_isLoading = false;
    int m_conflictCount = 0;
    class QThread *m_loadWorker = nullptr;
};
