#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct AliasItem
{
    QString id;
    QString name;
    QString command;
    QString shell;       // "Fish", "Bash", "Zsh"
    QString type;        // "Alias", "Function"
    QString filePath;
    int lineNumber = -1; // 1-based, or -1 if whole file
    QString description;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("command"), command},
            {QStringLiteral("shell"), shell},
            {QStringLiteral("type"), type},
            {QStringLiteral("filePath"), filePath},
            {QStringLiteral("lineNumber"), lineNumber},
            {QStringLiteral("description"), description}
        };
    }
};

class AliasManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit AliasManager(QObject *parent = nullptr);
    ~AliasManager() override = default;

    QVariantList entries() const;
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE void load();
    Q_INVOKABLE QVariantMap validateSyntax(const QString &shell, const QString &type, const QString &name, const QString &command);
    Q_INVOKABLE bool createEntry(const QString &shell, const QString &type, const QString &name, const QString &command);
    Q_INVOKABLE bool saveEntry(const QString &id, const QString &name, const QString &command);
    Q_INVOKABLE bool deleteEntry(const QString &id);

signals:
    void entriesChanged();
    void isLoadingChanged();
    void operationFinished(bool success, const QString &message);

private:
    void loadFish();
    void loadBash();
    void loadZsh();

    QVector<AliasItem> m_items;
    bool m_isLoading = false;
};
