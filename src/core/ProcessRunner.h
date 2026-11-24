#pragma once

#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantMap>

struct ProcessResult
{
    int exitCode = -1;
    QString stdoutStr;
    QString stderrStr;
    bool timedOut = false;

    QVariantMap toMap() const
    {
        return {
            {QStringLiteral("exitCode"), exitCode},
            {QStringLiteral("stdout"), stdoutStr},
            {QStringLiteral("stderr"), stderrStr},
            {QStringLiteral("timedOut"), timedOut}
        };
    }
};

class ProcessRunner : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    bool isRunning() const;

    static ProcessResult run(const QString &program, const QStringList &args = {}, int timeoutMs = 30000);

    Q_INVOKABLE QVariantMap runSync(const QString &program, const QStringList &args = {}, int timeoutMs = 30000);
    Q_INVOKABLE void runAsync(const QString &program, const QStringList &args = {});
    Q_INVOKABLE void cancel();

signals:
    void started();
    void outputChunk(const QString &chunk);
    void errorChunk(const QString &chunk);
    void finished(int exitCode, const QString &output);
    void logLine(const QString &source, const QString &line);
    void isRunningChanged();

private:
    void handleOutput();
    void handleError();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

    QProcess *m_process = nullptr;
    QString m_accumulatedOutput;
    QString m_currentProgram;
};
