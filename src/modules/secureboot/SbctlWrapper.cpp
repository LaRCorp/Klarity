#include "SbctlWrapper.h"
#include "core/LogModel.h"
#include "core/PkexecRunner.h"
#include "core/ProcessRunner.h"

#include <KLocalizedString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

SbctlWrapper::SbctlWrapper(QObject *parent)
    : QObject(parent)
{
    if (hasSbctl()) {
        refreshStatus();
    }
}

bool SbctlWrapper::hasSbctl() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("sbctl")).isEmpty();
}

void SbctlWrapper::parseStatusJson(const QString &jsonStr)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        LogModel::instance()->append(
            QStringLiteral("SecureBoot"),
            QStringLiteral("warn"),
            QStringLiteral("Failed to parse sbctl status JSON: %1").arg(err.errorString())
        );
        return;
    }

    const QJsonObject obj = doc.object();

    // "installed": bool
    if (obj.contains(QStringLiteral("installed"))) {
        const auto val = obj.value(QStringLiteral("installed"));
        m_installed = val.isBool() ? val.toBool() : (val.toString().toLower() == QStringLiteral("true") || val.toString().toLower() == QStringLiteral("installed"));
    }

    // "setup_mode": bool or string
    if (obj.contains(QStringLiteral("setup_mode"))) {
        const auto val = obj.value(QStringLiteral("setup_mode"));
        if (val.isBool()) {
            m_setupMode = val.toBool();
        } else {
            const QString s = val.toString().toLower();
            m_setupMode = (s == QStringLiteral("enabled") || s == QStringLiteral("true") || s.contains(QStringLiteral("setup")));
        }
    }

    // "secure_boot": bool or string
    if (obj.contains(QStringLiteral("secure_boot"))) {
        const auto val = obj.value(QStringLiteral("secure_boot"));
        if (val.isBool()) {
            m_secureBoot = val.toBool();
        } else {
            const QString s = val.toString().toLower();
            m_secureBoot = (s == QStringLiteral("enabled") || s == QStringLiteral("true"));
        }
    }

    m_guid = obj.value(QStringLiteral("guid")).toString();

    m_vendors.clear();
    const QJsonArray vendorsArr = obj.value(QStringLiteral("vendors")).toArray();
    for (const auto &v : vendorsArr) {
        m_vendors.append(v.toString());
    }

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Status updated: Installed=%1, SetupMode=%2, SecureBoot=%3")
            .arg(m_installed ? "Yes" : "No")
            .arg(m_setupMode ? "Enabled" : "Disabled")
            .arg(m_secureBoot ? "Enabled" : "Disabled")
    );

    emit statusUpdated(obj);
}

void SbctlWrapper::parseVerifyOutput(const QString &output)
{
    m_unsignedFiles.clear();
    m_signedFiles.clear();

    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const auto &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        if (line.contains(QChar(0x2713)) || line.contains(QStringLiteral("✓")) || line.endsWith(QStringLiteral("is signed"))) {
            // Signed line: ✓ /path/to/file is signed
            QString filePath = line;
            filePath.remove(QChar(0x2713)).remove(QStringLiteral("✓")).remove(QStringLiteral("is signed"));
            filePath = filePath.trimmed();
            if (!filePath.isEmpty()) {
                m_signedFiles.append(filePath);
            }
        } else if (line.contains(QChar(0x2717)) || line.contains(QStringLiteral("✗")) || line.contains(QStringLiteral("not signed")) || line.contains(QStringLiteral("is not signed"))) {
            // Unsigned line: ✗ /path/to/file is not signed
            QString filePath = line;
            filePath.remove(QChar(0x2717)).remove(QStringLiteral("✗")).remove(QStringLiteral("is not signed")).remove(QStringLiteral("not signed"));
            filePath = filePath.trimmed();
            if (!filePath.isEmpty()) {
                m_unsignedFiles.append(filePath);
            }
        }
    }

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Verify completed: %1 signed, %2 unsigned").arg(m_signedFiles.size()).arg(m_unsignedFiles.size())
    );

    emit verifyUpdated(m_unsignedFiles, m_signedFiles);
}

void SbctlWrapper::refreshStatus()
{
    if (!hasSbctl()) {
        emit operationFinished(false, QStringLiteral("sbctl executable not found on system."));
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    auto runner = new ProcessRunner(this);
    connect(runner, &ProcessRunner::finished, this, [this](int exitCode, const QString &output) {
        m_isLoading = false;
        emit isLoadingChanged();

        if (exitCode == 0 && !output.trimmed().isEmpty()) {
            m_lastOutput = output;
            emit commandFinished(exitCode, output);
            parseStatusJson(output);
        } else {
            // Try with pkexec if non-root access was denied
            auto rootRunner = new PkexecRunner(this);
            connect(rootRunner, &PkexecRunner::finished, this, [this](int code, const QString &out) {
                m_lastOutput = out;
                emit commandFinished(code, out);
                if (code == 0) {
                    parseStatusJson(out);
                }
            });
            rootRunner->runRoot(QStringLiteral("sbctl"), {QStringLiteral("status"), QStringLiteral("--json")});
        }
    });

    runner->runAsync(QStringLiteral("sbctl"), {QStringLiteral("status"), QStringLiteral("--json")});
}

void SbctlWrapper::verifyFiles()
{
    if (!hasSbctl()) {
        emit operationFinished(false, QStringLiteral("sbctl executable not found."));
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Running sbctl verify with elevated privileges...")
    );

    auto runner = new PkexecRunner(this);
    connect(runner, &PkexecRunner::finished, this, [this](int exitCode, const QString &output) {
        m_isLoading = false;
        emit isLoadingChanged();
        m_lastOutput = output;
        emit commandFinished(exitCode, output);

        if (exitCode == 0 || exitCode == 1) { // sbctl returns 1 if some files are unsigned
            parseVerifyOutput(output);
            emit operationFinished(true, i18n("Files verification completed."));
        } else if (exitCode == 126 || exitCode == 127) {
            emit operationFinished(false, i18n("Verification cancelled by user."));
        } else {
            emit operationFinished(false, i18n("Verification failed: %1", output));
        }
    });

    runner->runRoot(QStringLiteral("sbctl"), {QStringLiteral("verify")});
}

void SbctlWrapper::signAll()
{
    if (!hasSbctl()) {
        emit operationFinished(false, i18n("sbctl executable not found."));
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Executing sbctl sign-all...")
    );

    auto runner = new PkexecRunner(this);
    connect(runner, &PkexecRunner::finished, this, [this](int exitCode, const QString &output) {
        m_isLoading = false;
        emit isLoadingChanged();
        m_lastOutput = output;
        emit commandFinished(exitCode, output);

        if (exitCode == 0) {
            LogModel::instance()->append(QStringLiteral("SecureBoot"), QStringLiteral("info"), QStringLiteral("All EFI files signed successfully"));
            emit operationFinished(true, i18n("All files signed successfully."));
            verifyFiles();
        } else if (exitCode == 126 || exitCode == 127) {
            emit operationFinished(false, i18n("Signing cancelled by user."));
        } else {
            emit operationFinished(false, i18n("Failed to sign files: %1", output));
        }
    });

    runner->runRoot(QStringLiteral("sbctl"), {QStringLiteral("sign-all")});
}

void SbctlWrapper::signFile(const QString &filePath)
{
    if (!hasSbctl() || filePath.isEmpty()) {
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Signing EFI binary: %1").arg(filePath)
    );

    auto runner = new PkexecRunner(this);
    connect(runner, &PkexecRunner::finished, this, [this, filePath](int exitCode, const QString &output) {
        m_isLoading = false;
        emit isLoadingChanged();
        m_lastOutput = output;
        emit commandFinished(exitCode, output);

        if (exitCode == 0) {
            LogModel::instance()->append(QStringLiteral("SecureBoot"), QStringLiteral("info"), QStringLiteral("Signed: %1").arg(filePath));
            emit operationFinished(true, i18n("Signed %1 successfully.", QFileInfo(filePath).fileName()));
            verifyFiles();
        } else if (exitCode == 126 || exitCode == 127) {
            emit operationFinished(false, i18n("Signing cancelled."));
        } else {
            emit operationFinished(false, i18n("Signing failed: %1", output));
        }
    });

    runner->runRoot(QStringLiteral("sbctl"), {QStringLiteral("sign"), QStringLiteral("-s"), filePath});
}

void SbctlWrapper::createKeys()
{
    if (!hasSbctl()) {
        emit operationFinished(false, i18n("sbctl executable not found."));
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Creating new custom Secure Boot keys (sbctl create-keys)...")
    );

    auto runner = new PkexecRunner(this);
    connect(runner, &PkexecRunner::finished, this, [this](int exitCode, const QString &output) {
        m_isLoading = false;
        emit isLoadingChanged();
        m_lastOutput = output;
        emit commandFinished(exitCode, output);

        if (exitCode == 0) {
            LogModel::instance()->append(QStringLiteral("SecureBoot"), QStringLiteral("info"), QStringLiteral("Custom Secure Boot keys created successfully"));
            emit operationFinished(true, i18n("Custom Secure Boot keys created."));
            refreshStatus();
        } else if (exitCode == 126 || exitCode == 127) {
            emit operationFinished(false, i18n("Key creation cancelled."));
        } else {
            emit operationFinished(false, i18n("Key creation failed: %1", output));
        }
    });

    runner->runRoot(QStringLiteral("sbctl"), {QStringLiteral("create-keys")});
}

void SbctlWrapper::enrollKeys(bool withMicrosoft)
{
    if (!hasSbctl()) {
        emit operationFinished(false, i18n("sbctl executable not found."));
        return;
    }

    if (!m_setupMode) {
        emit operationFinished(false, i18n("Cannot enroll keys: UEFI firmware is not in Setup Mode. Enable Setup Mode in your BIOS/UEFI settings first."));
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    QStringList args = {QStringLiteral("enroll-keys")};
    if (withMicrosoft) {
        args.append(QStringLiteral("--microsoft"));
    }

    LogModel::instance()->append(
        QStringLiteral("SecureBoot"),
        QStringLiteral("info"),
        QStringLiteral("Enrolling keys into UEFI firmware (%1)...").arg(args.join(QLatin1Char(' ')))
    );

    auto runner = new PkexecRunner(this);
    connect(runner, &PkexecRunner::finished, this, [this](int exitCode, const QString &output) {
        m_isLoading = false;
        emit isLoadingChanged();
        m_lastOutput = output;
        emit commandFinished(exitCode, output);

        if (exitCode == 0) {
            LogModel::instance()->append(QStringLiteral("SecureBoot"), QStringLiteral("info"), QStringLiteral("Keys enrolled into UEFI successfully"));
            emit operationFinished(true, i18n("Keys enrolled into UEFI firmware successfully."));
            refreshStatus();
        } else if (exitCode == 126 || exitCode == 127) {
            emit operationFinished(false, i18n("Enrollment cancelled."));
        } else {
            emit operationFinished(false, i18n("Key enrollment failed: %1", output));
        }
    });

    runner->runRoot(QStringLiteral("sbctl"), args);
}
