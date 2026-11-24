#include "PkexecRunner.h"
#include "ProcessRunner.h"
#include "LogModel.h"

PkexecRunner::PkexecRunner(QObject *parent)
    : QObject(parent)
    , m_runner(new ProcessRunner(this))
{
    connect(m_runner, &ProcessRunner::started, this, &PkexecRunner::started);
    connect(m_runner, &ProcessRunner::isRunningChanged, this, &PkexecRunner::isRunningChanged);
    connect(m_runner, &ProcessRunner::outputChunk, this, &PkexecRunner::outputChunk);
    connect(m_runner, &ProcessRunner::errorChunk, this, &PkexecRunner::errorChunk);

    connect(m_runner, &ProcessRunner::finished, this, &PkexecRunner::onProcessFinished);
}

bool PkexecRunner::isRunning() const
{
    return m_runner && m_runner->isRunning();
}

void PkexecRunner::runRoot(const QString &program, const QStringList &args)
{
    m_currentProgram = program;

    QStringList pkexecArgs;
    pkexecArgs.append(program);
    pkexecArgs.append(args);

    LogModel::instance()->append(QStringLiteral("pkexec"), QStringLiteral("info"),
                                QStringLiteral("Requesting root privilege for: %1 %2")
                                    .arg(program, args.join(QLatin1Char(' '))));

    // Only pkexec is used for elevated privilege commands
    m_runner->runAsync(QStringLiteral("pkexec"), pkexecArgs);
}

void PkexecRunner::cancel()
{
    if (m_runner) {
        m_runner->cancel();
    }
}

void PkexecRunner::onProcessFinished(int exitCode, const QString &output)
{
    // Polkit / pkexec exit code 126: authorization dismissed by user
    // Polkit / pkexec exit code 127: authorization could not be obtained / pkexec error
    if (exitCode == 126 || exitCode == 127) {
        LogModel::instance()->append(QStringLiteral("pkexec"), QStringLiteral("warn"),
                                    QStringLiteral("Root authorization cancelled or denied (code %1)").arg(exitCode));
        emit cancelled();
        return;
    }

    if (exitCode != 0) {
        const QString reason = QStringLiteral("Root command '%1' failed with exit code %2")
                                  .arg(m_currentProgram, QString::number(exitCode));
        LogModel::instance()->append(QStringLiteral("pkexec"), QStringLiteral("error"), reason);
        emit errorOccurred(reason);
    }

    emit finished(exitCode, output);
}
