#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>

class SbctlWrapper : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasSbctl READ hasSbctl CONSTANT)
    Q_PROPERTY(bool installed READ installed NOTIFY statusUpdated)
    Q_PROPERTY(bool setupMode READ setupMode NOTIFY statusUpdated)
    Q_PROPERTY(bool secureBoot READ secureBoot NOTIFY statusUpdated)
    Q_PROPERTY(QString guid READ guid NOTIFY statusUpdated)
    Q_PROPERTY(QStringList vendors READ vendors NOTIFY statusUpdated)
    Q_PROPERTY(QStringList unsignedFiles READ unsignedFiles NOTIFY verifyUpdated)
    Q_PROPERTY(QStringList signedFiles READ signedFiles NOTIFY verifyUpdated)
    Q_PROPERTY(int unsignedCount READ unsignedCount NOTIFY verifyUpdated)
    Q_PROPERTY(int signedCount READ signedCount NOTIFY verifyUpdated)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString lastOutput READ lastOutput NOTIFY commandFinished)

public:
    explicit SbctlWrapper(QObject *parent = nullptr);
    ~SbctlWrapper() override = default;

    bool hasSbctl() const;
    bool installed() const { return m_installed; }
    bool setupMode() const { return m_setupMode; }
    bool secureBoot() const { return m_secureBoot; }
    QString guid() const { return m_guid; }
    QStringList vendors() const { return m_vendors; }
    QStringList unsignedFiles() const { return m_unsignedFiles; }
    QStringList signedFiles() const { return m_signedFiles; }
    int unsignedCount() const { return m_unsignedFiles.size(); }
    int signedCount() const { return m_signedFiles.size(); }
    bool isLoading() const { return m_isLoading; }
    QString lastOutput() const { return m_lastOutput; }

    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void verifyFiles();
    Q_INVOKABLE void signAll();
    Q_INVOKABLE void signFile(const QString &filePath);
    Q_INVOKABLE void createKeys();
    Q_INVOKABLE void enrollKeys(bool withMicrosoft = true);

signals:
    void statusUpdated(const QJsonObject &status);
    void verifyUpdated(const QStringList &unsignedFiles, const QStringList &signedFiles);
    void commandFinished(int exitCode, const QString &output);
    void isLoadingChanged();
    void operationFinished(bool success, const QString &message);

private:
    void parseStatusJson(const QString &jsonStr);
    void parseVerifyOutput(const QString &output);

    bool m_installed = false;
    bool m_setupMode = false;
    bool m_secureBoot = false;
    QString m_guid;
    QStringList m_vendors;
    QStringList m_unsignedFiles;
    QStringList m_signedFiles;
    bool m_isLoading = false;
    QString m_lastOutput;
};
