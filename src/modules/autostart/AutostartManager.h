#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct AutostartItem
{
    QString id;
    QString name;
    QString command;
    QString icon;
    QString type;       // "Desktop" or "Systemd"
    QString scope;      // "User" or "System"
    QString filePath;
    QString description;
    bool enabled = true;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("command"), command},
            {QStringLiteral("icon"), icon},
            {QStringLiteral("type"), type},
            {QStringLiteral("scope"), scope},
            {QStringLiteral("filePath"), filePath},
            {QStringLiteral("description"), description},
            {QStringLiteral("enabled"), enabled}
        };
    }
};

class AutostartManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit AutostartManager(QObject *parent = nullptr);
    ~AutostartManager() override = default;

    QVariantList entries() const;
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE void load();
    Q_INVOKABLE bool toggleEntry(const QString &id, bool enable);
    Q_INVOKABLE bool addDesktopEntry(const QString &name, const QString &command, const QString &icon, const QString &comment);
    Q_INVOKABLE bool removeEntry(const QString &id);

signals:
    void entriesChanged();
    void isLoadingChanged();
    void operationFinished(bool success, const QString &message);

private:
    void loadDesktopFiles();
    void loadSystemdUserServices();

    QVector<AutostartItem> m_items;
    bool m_isLoading = false;
};
