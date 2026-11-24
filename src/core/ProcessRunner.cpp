#include "ProcessRunner.h"
#include "LogModel.h"

#include <QCoreApplication>

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, [this]() {
        emit started();
        emit isRunningChanged();
    });

    connect(m_process, &QProcess::readyReadStandardOutput, this, &ProcessRunner::handleOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &ProcessRunner::handleError);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessRunner::handleFinished);
}

ProcessRunner::~ProcessRunner()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

bool ProcessRunner::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

ProcessResult ProcessRunner::run(const QString &program, const QStringList &args, int timeoutMs)
{
    ProcessResult result;
    QProcess proc;
    proc.start(program, args);

    LogModel::instance()->append(program, QStringLiteral("info"),
                                QStringLiteral("Starting (sync): %1 %2").arg(program, args.join(QLatin1Char(' '))));

    if (!proc.waitForStarted(timeoutMs)) {
        result.stderrStr = QStringLiteral("Failed to start process: %1").arg(proc.errorString());
        LogModel::instance()->append(program, QStringLiteral("error"), result.stderrStr);
        return result;
    }

    if (!proc.waitForFinished(timeoutMs)) {
        result.timedOut = true;
        proc.kill();
        proc.waitForFinished(500);
        result.stderrStr = QStringLiteral("Process timed out after %1 ms").arg(timeoutMs);
        LogModel::instance()->append(program, QStringLiteral("error"), result.stderrStr);
        return result;
    }

    result.exitCode = proc.exitCode();
    result.stdoutStr = QString::fromUtf8(proc.readAllStandardOutput());
    result.stderrStr = QString::fromUtf8(proc.readAllStandardError());

    if (result.exitCode == 0) {
        LogModel::instance()->append(program, QStringLiteral("info"),
                                    QStringLiteral("Finished successfully (code 0)"));
    } else {
        LogModel::instance()->append(program, QStringLiteral("error"),
                                    QStringLiteral("Finished with error code %1: %2").arg(QString::number(result.exitCode), result.stderrStr.trimmed()));
    }

    return result;
}

QVariantMap ProcessRunner::runSync(const QString &program, const QStringList &args, int timeoutMs)
{
    return run(program, args, timeoutMs).toMap();
}

void ProcessRunner::runAsync(const QString &program, const QStringList &args)
{
    if (isRunning()) {
        cancel();
    }

    m_currentProgram = program;
    m_accumulatedOutput.clear();

    const QString cmdStr = QStringLiteral("%1 %2").arg(program, args.join(QLatin1Char(' '))).trimmed();
    LogModel::instance()->append(program, QStringLiteral("info"), QStringLiteral("Starting (async): %1").arg(cmdStr));
    emit logLine(program, QStringLiteral("Starting: %1").arg(cmdStr));

    m_process->start(program, args);
}

void ProcessRunner::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        LogModel::instance()->append(m_currentProgram, QStringLiteral("warn"), QStringLiteral("Terminating process..."));
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }
}

void ProcessRunner::handleOutput()
{
    const QString chunk = QString::fromUtf8(m_process->readAllStandardOutput());
    if (chunk.isEmpty()) {
        return;
    }

    m_accumulatedOutput.append(chunk);
    emit outputChunk(chunk);

    const QStringList lines = chunk.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        LogModel::instance()->append(m_currentProgram, QStringLiteral("info"), line);
        emit logLine(m_currentProgram, line);
    }
}

void ProcessRunner::handleError()
{
    const QString chunk = QString::fromUtf8(m_process->readAllStandardError());
    if (chunk.isEmpty()) {
        return;
    }

    emit errorChunk(chunk);

    const QStringList lines = chunk.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        LogModel::instance()->append(m_currentProgram, QStringLiteral("warn"), line);
        emit logLine(m_currentProgram, line);
    }
}

void ProcessRunner::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    emit isRunningChanged();

    if (exitCode == 0) {
        LogModel::instance()->append(m_currentProgram, QStringLiteral("info"),
                                    QStringLiteral("Async process finished successfully (exit code 0)"));
    } else {
        LogModel::instance()->append(m_currentProgram, QStringLiteral("error"),
                                    QStringLiteral("Async process exited with code %1").arg(exitCode));
    }

    emit finished(exitCode, m_accumulatedOutput);
}
