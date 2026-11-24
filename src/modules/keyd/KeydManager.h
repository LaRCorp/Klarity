#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct KeydMapping
{
    QString id;
    QString layer;       // e.g. "main", "nav", "symbols"
    QString key;         // e.g. "capslock", "leftmeta+leftshift+f23"
    QString action;      // e.g. "overload(control, esc)", "rightcontrol", "macro(...)"
    QString description;
    bool isMacro = false;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("layer"), layer},
            {QStringLiteral("key"), key},
            {QStringLiteral("action"), action},
            {QStringLiteral("description"), description},
            {QStringLiteral("isMacro"), isMacro}
        };
    }
};

class KeydManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList mappings READ mappings NOTIFY mappingsChanged)
    Q_PROPERTY(QStringList layers READ layers NOTIFY mappingsChanged)
    Q_PROPERTY(QStringList deviceIds READ deviceIds NOTIFY mappingsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isServiceActive READ isServiceActive NOTIFY serviceStatusChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)

public:
    explicit KeydManager(QObject *parent = nullptr);
    ~KeydManager() override = default;

    QVariantList mappings() const;
    QStringList layers() const;
    QStringList deviceIds() const { return m_deviceIds; }
    bool isLoading() const { return m_isLoading; }
    bool isServiceActive() const { return m_isServiceActive; }
    QString configPath() const { return QStringLiteral("/etc/keyd/default.conf"); }

    Q_INVOKABLE void load();
    Q_INVOKABLE bool addMapping(const QString &layer, const QString &key, const QString &action, const QString &description = QString());
    Q_INVOKABLE bool editMapping(const QString &id, const QString &layer, const QString &key, const QString &action, const QString &description = QString());
    Q_INVOKABLE bool deleteMapping(const QString &id);
    Q_INVOKABLE void saveAndReload();
    Q_INVOKABLE void checkServiceStatus();
    Q_INVOKABLE QVariantList presets() const;

signals:
    void mappingsChanged();
    void isLoadingChanged();
    void serviceStatusChanged();
    void operationFinished(bool success, const QString &message);

private:
    QString generateConfigContent() const;
    QString computeFriendlyDescription(const QString &key, const QString &action) const;

    QVector<KeydMapping> m_items;
    QStringList m_deviceIds;
    bool m_isLoading = false;
    bool m_isServiceActive = false;
};
