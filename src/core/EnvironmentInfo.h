#pragma once

#include <QObject>
#include <QQmlEngine>

class EnvironmentInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString sessionType READ sessionType CONSTANT)
    Q_PROPERTY(QString distroId READ distroId CONSTANT)
    Q_PROPERTY(QString distroName READ distroName CONSTANT)
    Q_PROPERTY(bool hasSystemdResolved READ hasSystemdResolved CONSTANT)
    Q_PROPERTY(bool hasNetworkManager READ hasNetworkManager CONSTANT)
    Q_PROPERTY(bool hasFlatpak READ hasFlatpak CONSTANT)
    Q_PROPERTY(bool hasSnap READ hasSnap CONSTANT)
    Q_PROPERTY(bool hasFd READ hasFd CONSTANT)
    Q_PROPERTY(bool hasRipgrep READ hasRipgrep CONSTANT)

public:
    static EnvironmentInfo *instance();
    static EnvironmentInfo *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    explicit EnvironmentInfo(QObject *parent = nullptr);
    ~EnvironmentInfo() override = default;

    QString sessionType() const { return m_sessionType; }
    QString distroId() const { return m_distroId; }
    QString distroName() const { return m_distroName; }
    bool hasSystemdResolved() const { return m_hasSystemdResolved; }
    bool hasNetworkManager() const { return m_hasNetworkManager; }
    bool hasFlatpak() const { return m_hasFlatpak; }
    bool hasSnap() const { return m_hasSnap; }
    bool hasFd() const { return m_hasFd; }
    bool hasRipgrep() const { return m_hasRipgrep; }

private:
    void detectEnvironment();

    static EnvironmentInfo *s_instance;
    QString m_sessionType;
    QString m_distroId;
    QString m_distroName;
    bool m_hasSystemdResolved = false;
    bool m_hasNetworkManager = false;
    bool m_hasFlatpak = false;
    bool m_hasSnap = false;
    bool m_hasFd = false;
    bool m_hasRipgrep = false;
};
