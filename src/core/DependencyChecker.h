#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

class DependencyChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit DependencyChecker(QObject *parent = nullptr);
    ~DependencyChecker() override = default;

    Q_INVOKABLE bool hasCommand(const QString &name) const;
    Q_INVOKABLE QStringList missing(const QStringList &names) const;
    Q_INVOKABLE QString installHint(const QString &name) const;
};
