#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QProcess>

class UpdateManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)
    Q_PROPERTY(QString currentOperation READ currentOperation NOTIFY currentOperationChanged)
    Q_PROPERTY(QString logOutput READ logOutput NOTIFY logOutputChanged)
    Q_PROPERTY(int pacmanUpdatesCount READ pacmanUpdatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(int flatpakUpdatesCount READ flatpakUpdatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(int aurUpdatesCount READ aurUpdatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(int totalUpdatesCount READ totalUpdatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(QString lastCheckTimeStr READ lastCheckTimeStr NOTIFY updatesCountChanged)
    Q_PROPERTY(int exitCode READ exitCode NOTIFY processFinished)
    Q_PROPERTY(bool lastOperationSuccess READ lastOperationSuccess NOTIFY processFinished)
    Q_PROPERTY(bool hasYay READ hasYay CONSTANT)
    Q_PROPERTY(bool hasParu READ hasParu CONSTANT)
    Q_PROPERTY(bool hasFlatpak READ hasFlatpak CONSTANT)
    Q_PROPERTY(QString aurHelper READ aurHelper CONSTANT)

public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override;

    bool isRunning() const { return m_isRunning; }
    bool isChecking() const { return m_isChecking; }
    QString currentOperation() const { return m_currentOperation; }
    QString logOutput() const { return m_logOutput; }
    int pacmanUpdatesCount() const { return m_pacmanUpdatesCount; }
    int flatpakUpdatesCount() const { return m_flatpakUpdatesCount; }
    int aurUpdatesCount() const { return m_aurUpdatesCount; }
    int totalUpdatesCount() const { return m_pacmanUpdatesCount + m_flatpakUpdatesCount + m_aurUpdatesCount; }
    QString lastCheckTimeStr() const;
    int exitCode() const { return m_exitCode; }
    bool lastOperationSuccess() const { return m_lastOperationSuccess; }
    bool hasYay() const { return m_hasYay; }
    bool hasParu() const { return m_hasParu; }
    bool hasFlatpak() const { return m_hasFlatpak; }
    QString aurHelper() const { return m_aurHelper; }

    Q_INVOKABLE void checkUpdates(bool force = false);
    Q_INVOKABLE void forceCheckUpdates() { checkUpdates(true); }
    Q_INVOKABLE void runSystemUpgrade();
    Q_INVOKABLE void runPackageUpdate();
    Q_INVOKABLE void runFlatpakUpdate();
    Q_INVOKABLE void runAurUpdate();
    Q_INVOKABLE void cancelOperation();
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void openArchNews();
    Q_INVOKABLE void copyToClipboard(const QString &text);

signals:
    void isRunningChanged();
    void isCheckingChanged();
    void currentOperationChanged();
    void logOutputChanged();
    void updatesCountChanged();
    void outputReceived(const QString &text);
    void finished(int exitCode, bool success);
    void processFinished();

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void appendLog(const QString &text);
    void startCommand(const QString &operationName, const QString &program, const QStringList &args);

    QProcess *m_process = nullptr;
    bool m_isRunning = false;
    bool m_isChecking = false;
    QString m_currentOperation;
    QString m_logOutput;

    int m_pacmanUpdatesCount = 0;
    int m_flatpakUpdatesCount = 0;
    int m_aurUpdatesCount = 0;
    int m_exitCode = 0;
    bool m_lastOperationSuccess = true;

    bool m_hasYay = false;
    bool m_hasParu = false;
    bool m_hasFlatpak = false;
    QString m_aurHelper;
};
