#include "UpdateManager.h"
#include "core/LogModel.h"

#include <QDesktopServices>
#include <QDateTime>
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <KLocalizedString>

static int s_cachedPacmanCount = -1;
static int s_cachedFlatpakCount = -1;
static int s_cachedAurCount = -1;
static QDateTime s_lastCheckTime;

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
{
    m_hasFlatpak = !QStandardPaths::findExecutable(QStringLiteral("flatpak")).isEmpty();
    m_hasYay = !QStandardPaths::findExecutable(QStringLiteral("yay")).isEmpty();
    m_hasParu = !QStandardPaths::findExecutable(QStringLiteral("paru")).isEmpty();

    if (m_hasYay) {
        m_aurHelper = QStringLiteral("yay");
    } else if (m_hasParu) {
        m_aurHelper = QStringLiteral("paru");
    }

    if (s_lastCheckTime.isValid() && s_lastCheckTime.secsTo(QDateTime::currentDateTime()) < 1800 && s_cachedPacmanCount >= 0) {
        m_pacmanUpdatesCount = s_cachedPacmanCount;
        m_flatpakUpdatesCount = s_cachedFlatpakCount;
        m_aurUpdatesCount = s_cachedAurCount;
        m_isChecking = false;
    } else {
        checkUpdates(false);
    }
}

UpdateManager::~UpdateManager()
{
    cancelOperation();
}

void UpdateManager::checkUpdates(bool force)
{
    if (m_isChecking || m_isRunning) return;

    if (!force && s_lastCheckTime.isValid() && s_lastCheckTime.secsTo(QDateTime::currentDateTime()) < 1800 && s_cachedPacmanCount >= 0) {
        m_pacmanUpdatesCount = s_cachedPacmanCount;
        m_flatpakUpdatesCount = s_cachedFlatpakCount;
        m_aurUpdatesCount = s_cachedAurCount;
        m_isChecking = false;
        emit isCheckingChanged();
        emit updatesCountChanged();
        return;
    }

    m_isChecking = true;
    emit isCheckingChanged();

    const bool hasFlatpak = m_hasFlatpak;
    const QString aurHelper = m_aurHelper;

    QThread *thread = QThread::create([this, hasFlatpak, aurHelper]() {
        int pacmanCount = 0;
        int flatpakCount = 0;
        int aurCount = 0;

        // 1. Check Pacman updates (via checkupdates or pacman -Qu)
        QProcess pacProc;
        pacProc.start(QStringLiteral("checkupdates"), {});
        if (pacProc.waitForFinished(10000) && pacProc.exitCode() == 0) {
            const QString out = QString::fromUtf8(pacProc.readAllStandardOutput());
            const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            pacmanCount = lines.size();
        } else {
            // Fallback to pacman -Qu
            QProcess pQu;
            pQu.start(QStringLiteral("pacman"), {QStringLiteral("-Qu")});
            if (pQu.waitForFinished(4000) && pQu.exitCode() == 0) {
                const QString out = QString::fromUtf8(pQu.readAllStandardOutput());
                const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                pacmanCount = lines.size();
            }
        }

        // 2. Check Flatpak updates
        if (hasFlatpak) {
            QProcess fpProc;
            fpProc.start(QStringLiteral("flatpak"), {QStringLiteral("remote-ls"), QStringLiteral("--updates")});
            if (fpProc.waitForFinished(10000) && fpProc.exitCode() == 0) {
                const QString out = QString::fromUtf8(fpProc.readAllStandardOutput());
                const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                flatpakCount = lines.size();
            }
        }

        // 3. Check AUR updates
        if (!aurHelper.isEmpty()) {
            QProcess aurProc;
            aurProc.start(aurHelper, {QStringLiteral("-Qua")});
            if (aurProc.waitForFinished(15000) && aurProc.exitCode() == 0) {
                const QString out = QString::fromUtf8(aurProc.readAllStandardOutput());
                const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                for (const QString &l : lines) {
                    if (l.contains(QStringLiteral("->"))) {
                        aurCount++;
                    }
                }
            }
        }

        QMetaObject::invokeMethod(this, [this, pacmanCount, flatpakCount, aurCount]() {
            m_pacmanUpdatesCount = pacmanCount;
            m_flatpakUpdatesCount = flatpakCount;
            m_aurUpdatesCount = aurCount;
            m_isChecking = false;

            s_cachedPacmanCount = pacmanCount;
            s_cachedFlatpakCount = flatpakCount;
            s_cachedAurCount = aurCount;
            s_lastCheckTime = QDateTime::currentDateTime();

            emit isCheckingChanged();
            emit updatesCountChanged();

            LogModel::instance()->append(
                QStringLiteral("UpdateManager"),
                QStringLiteral("info"),
                QStringLiteral("Update check finished. Pacman: %1, Flatpak: %2, AUR: %3")
                    .arg(pacmanCount).arg(flatpakCount).arg(aurCount)
            );
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void UpdateManager::startCommand(const QString &operationName, const QString &program, const QStringList &args)
{
    cancelOperation();

    m_currentOperation = operationName;
    emit currentOperationChanged();

    m_isRunning = true;
    emit isRunningChanged();

    appendLog(QStringLiteral("============================================================\n"));
    appendLog(QStringLiteral(">>> Starting: %1\n").arg(operationName));
    appendLog(QStringLiteral(">>> Command: %1 %2\n").arg(program, args.join(QLatin1Char(' '))));
    appendLog(QStringLiteral("============================================================\n\n"));

    m_process = new QProcess(this);

    // Combine stdout and stderr into unified terminal stream
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &UpdateManager::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &UpdateManager::onReadyReadStandardError);
    connect(m_process, &QProcess::finished, this, &UpdateManager::onProcessFinished);

    LogModel::instance()->append(
        QStringLiteral("UpdateManager"),
        QStringLiteral("info"),
        QStringLiteral("Running %1: %2 %3").arg(operationName, program, args.join(QLatin1Char(' ')))
    );

    m_process->start(program, args);
}

void UpdateManager::runSystemUpgrade()
{
    startCommand(
        i18n("Full System Upgrade (pacman -Syu)"),
        QStringLiteral("pkexec"),
        {QStringLiteral("pacman"), QStringLiteral("-Syu"), QStringLiteral("--noconfirm")}
    );
}

void UpdateManager::runPackageUpdate()
{
    startCommand(
        i18n("Package Update (pacman -Syu --needed)"),
        QStringLiteral("pkexec"),
        {QStringLiteral("pacman"), QStringLiteral("-Syu"), QStringLiteral("--needed"), QStringLiteral("--noconfirm")}
    );
}

void UpdateManager::runFlatpakUpdate()
{
    if (!m_hasFlatpak) {
        appendLog(i18n("Error: Flatpak is not installed on this system.\n"));
        return;
    }

    startCommand(
        i18n("Flatpak Applications Update"),
        QStringLiteral("flatpak"),
        {QStringLiteral("update"), QStringLiteral("-y")}
    );
}

void UpdateManager::runAurUpdate()
{
    if (m_aurHelper.isEmpty()) {
        appendLog(i18n("Error: Neither 'yay' nor 'paru' AUR helpers are installed on this system.\n"));
        return;
    }

    startCommand(
        i18n("AUR Packages Update (%1 -Sua)", m_aurHelper),
        m_aurHelper,
        {QStringLiteral("-Sua"), QStringLiteral("--noconfirm")}
    );
}

void UpdateManager::cancelOperation()
{
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            appendLog(i18n("\n>>> Operation cancelled by user. Terminating process...\n"));
            m_process->terminate();
            if (!m_process->waitForFinished(1500)) {
                m_process->kill();
                m_process->waitForFinished(500);
            }
        }
        delete m_process;
        m_process = nullptr;
    }

    if (m_isRunning) {
        m_isRunning = false;
        emit isRunningChanged();
    }
}

void UpdateManager::onReadyReadStandardOutput()
{
    if (!m_process) return;
    const QString chunk = QString::fromUtf8(m_process->readAllStandardOutput());
    if (!chunk.isEmpty()) {
        appendLog(chunk);
    }
}

void UpdateManager::onReadyReadStandardError()
{
    if (!m_process) return;
    const QString chunk = QString::fromUtf8(m_process->readAllStandardError());
    if (!chunk.isEmpty()) {
        appendLog(chunk);
    }
}

void UpdateManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    m_exitCode = exitCode;
    m_lastOperationSuccess = (exitCode == 0);
    m_isRunning = false;
    emit isRunningChanged();
    emit processFinished();

    const QString resultStr = m_lastOperationSuccess
        ? i18n("\n>>> Operation completed SUCCESSFULLY (exit code 0).\n")
        : i18n("\n>>> Operation FAILED with exit code %1.\n", exitCode);

    appendLog(resultStr);

    LogModel::instance()->append(
        QStringLiteral("UpdateManager"),
        m_lastOperationSuccess ? QStringLiteral("info") : QStringLiteral("error"),
        QStringLiteral("Operation '%1' finished with code %2").arg(m_currentOperation).arg(exitCode)
    );

    emit finished(exitCode, m_lastOperationSuccess);

    // Refresh update counts after completion
    if (m_lastOperationSuccess) {
        checkUpdates();
    }
}

QString UpdateManager::lastCheckTimeStr() const
{
    if (!s_lastCheckTime.isValid()) {
        return i18n("Never");
    }
    const qint64 secs = s_lastCheckTime.secsTo(QDateTime::currentDateTime());
    if (secs < 60) {
        return i18n("Just now");
    } else if (secs < 3600) {
        return i18n("%1 min ago", secs / 60);
    } else {
        return s_lastCheckTime.toString(QStringLiteral("hh:mm"));
    }
}

void UpdateManager::appendLog(const QString &text)
{
    m_logOutput.append(text);
    // Keep log buffer bounded to 200,000 characters
    if (m_logOutput.size() > 200000) {
        m_logOutput = m_logOutput.right(150000);
    }
    emit outputReceived(text);
    emit logOutputChanged();
}

void UpdateManager::clearLogs()
{
    m_logOutput.clear();
    emit logOutputChanged();
}

void UpdateManager::openArchNews()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://archlinux.org/news/")));
}

void UpdateManager::copyToClipboard(const QString &text)
{
    if (auto *cb = QGuiApplication::clipboard()) {
        cb->setText(text);
    }
}
