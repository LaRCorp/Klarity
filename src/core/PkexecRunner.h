#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

class ProcessRunner;

class PkexecRunner : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
    explicit PkexecRunner(QObject *parent = nullptr);
    ~PkexecRunner() override = default;

    bool isRunning() const;

    Q_INVOKABLE void runRoot(const QString &program, const QStringList &args = {});
    Q_INVOKABLE void cancel();

signals:
    void started();
    void cancelled();
    void errorOccurred(const QString &reason);
    void outputChunk(const QString &chunk);
    void errorChunk(const QString &chunk);
    void finished(int exitCode, const QString &output);
    void isRunningChanged();

private:
    void onProcessFinished(int exitCode, const QString &output);

    ProcessRunner *m_runner = nullptr;
    QString m_currentProgram;
};
