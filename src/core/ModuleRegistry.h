#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct ModuleDescriptor
{
    QString id;
    QString title;
    QString icon;
    QString qmlSource;
    QString category;
    bool requiresRoot = false;
    QStringList dependencies;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("title"), title},
            {QStringLiteral("icon"), icon},
            {QStringLiteral("qmlSource"), qmlSource},
            {QStringLiteral("category"), category},
            {QStringLiteral("requiresRoot"), requiresRoot},
            {QStringLiteral("dependencies"), dependencies}
        };
    }
};

class ModuleRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList modules READ modules NOTIFY modulesChanged)

public:
    static ModuleRegistry *instance();
    static ModuleRegistry *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    explicit ModuleRegistry(QObject *parent = nullptr);
    ~ModuleRegistry() override = default;

    QVariantList modules() const;

    void registerModule(const ModuleDescriptor &descriptor);
    void registerBuiltin();

    Q_INVOKABLE QStringList missingDependencies(const QString &id) const;
    Q_INVOKABLE QVariantMap getModule(const QString &id) const;

signals:
    void modulesChanged();

private:
    static ModuleRegistry *s_instance;
    QVector<ModuleDescriptor> m_registeredModules;
};
