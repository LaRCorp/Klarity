#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct GestureItem
{
    QString id;
    QString type;       // "swipe", "pinch"
    QString direction;  // "left", "right", "up", "down", "in", "out", "clockwise", "anticlockwise"
    int fingers = 3;
    QString command;
    bool enabled = true;
    QString friendlyName;
    QString icon;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("type"), type},
            {QStringLiteral("direction"), direction},
            {QStringLiteral("fingers"), fingers},
            {QStringLiteral("command"), command},
            {QStringLiteral("enabled"), enabled},
            {QStringLiteral("friendlyName"), friendlyName},
            {QStringLiteral("icon"), icon}
        };
    }
};

class GesturesManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList gestures READ gestures NOTIFY gesturesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isServiceRunning READ isServiceRunning NOTIFY serviceStatusChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)

public:
    explicit GesturesManager(QObject *parent = nullptr);
    ~GesturesManager() override = default;

    QVariantList gestures() const;
    bool isLoading() const { return m_isLoading; }
    bool isServiceRunning() const { return m_isServiceRunning; }
    QString configPath() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE bool addGesture(const QString &type, const QString &direction, int fingers, const QString &command);
    Q_INVOKABLE bool editGesture(const QString &id, const QString &type, const QString &direction, int fingers, const QString &command, bool enabled);
    Q_INVOKABLE bool deleteGesture(const QString &id);
    Q_INVOKABLE bool toggleGesture(const QString &id);
    Q_INVOKABLE void restartService();
    Q_INVOKABLE void checkServiceStatus();
    Q_INVOKABLE QVariantList presets() const;

signals:
    void gesturesChanged();
    void isLoadingChanged();
    void serviceStatusChanged();
    void operationFinished(bool success, const QString &message);

private:
    bool saveToFile();
    QString computeFriendlyName(const QString &cmd) const;
    QString computeIcon(const QString &type, const QString &direction) const;

    QVector<GestureItem> m_items;
    bool m_isLoading = false;
    bool m_isServiceRunning = false;
};
