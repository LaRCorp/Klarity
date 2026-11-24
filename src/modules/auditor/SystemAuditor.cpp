#include "SystemAuditor.h"
#include "core/LogModel.h"
#include "core/PkexecRunner.h"
#include "core/ProcessRunner.h"

#include <KLocalizedString>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>

namespace {

const QStringList CONFIG_WHITELIST = {
    QStringLiteral("autostart"),
    QStringLiteral("dconf"),
    QStringLiteral("gtk-2.0"),
    QStringLiteral("gtk-3.0"),
    QStringLiteral("gtk-4.0"),
    QStringLiteral("kde.org"),
    QStringLiteral("kded5rc"),
    QStringLiteral("kded6rc"),
    QStringLiteral("kglobalshortcutsrc"),
    QStringLiteral("kwinrc"),
    QStringLiteral("plasma*"),
    QStringLiteral("systemd"),
    QStringLiteral("pulse"),
    QStringLiteral("pipewire"),
    QStringLiteral("fish"),
    QStringLiteral("bash"),
    QStringLiteral("zsh"),
    QStringLiteral("fontconfig"),
    QStringLiteral("mimeapps.list"),
    QStringLiteral("user-dirs.*"),
    QStringLiteral("environment.d"),
    QStringLiteral("klarity"),
    QStringLiteral("keyd"),
    QStringLiteral("libinput-gestures.conf"),
    QStringLiteral("xsettingsd"),
    QStringLiteral("cachyos*"),
    QStringLiteral("alacritty"),
    QStringLiteral("kitty"),
    QStringLiteral("foot"),
    QStringLiteral("konsole*"),
    QStringLiteral("dolphin*"),
    QStringLiteral("google-chrome*"),
    QStringLiteral("chromium*"),
    QStringLiteral("mozilla"),
    QStringLiteral("vesktop"),
    QStringLiteral("discord"),
    QStringLiteral("telegram*"),
    QStringLiteral("ayugram*"),
    QStringLiteral("code*"),
    QStringLiteral("vscode*"),
    QStringLiteral("vlc"),
    QStringLiteral("mpv"),
    QStringLiteral("gwenview*"),
    QStringLiteral("spectacle*"),
    QStringLiteral("session"),
    QStringLiteral("trash"),
    QStringLiteral("Trash"),
    QStringLiteral("applications"),
    QStringLiteral("icons"),
    QStringLiteral("themes"),
    QStringLiteral("fonts"),
    QStringLiteral("flatpak")
};

qint64 calculateDirSize(const QString &dirPath)
{
    qint64 size = 0;
    QDirIterator it(dirPath, QDir::Files | QDir::Hidden | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        size += it.fileInfo().size();
    }
    return size;
}

} // namespace

SystemAuditor::SystemAuditor(QObject *parent)
    : QObject(parent)
{
}

QVariantList SystemAuditor::items() const
{
    QVariantList list;
    list.reserve(m_items.size());
    for (const auto &item : m_items) {
        list.append(item.toMap());
    }
    return list;
}

int SystemAuditor::selectedCount() const
{
    int count = 0;
    for (const auto &item : m_items) {
        if (item.selected) {
            count++;
        }
    }
    return count;
}

QString SystemAuditor::formatSize(qint64 bytes)
{
    if (bytes <= 0) return i18n("0 B");
    const double b = static_cast<double>(bytes);
    if (b < 1024.0) return i18n("%1 B", bytes);
    if (b < 1024.0 * 1024.0) return i18n("%1 KiB", QString::number(b / 1024.0, 'f', 1));
    if (b < 1024.0 * 1024.0 * 1024.0) return i18n("%1 MiB", QString::number(b / (1024.0 * 1024.0), 'f', 1));
    return i18n("%1 GiB", QString::number(b / (1024.0 * 1024.0 * 1024.0), 'f', 2));
}

QString SystemAuditor::totalReclaimableFormatted() const
{
    return formatSize(m_totalReclaimableBytes);
}

bool SystemAuditor::isWhitelisted(const QString &name) const
{
    const QString lower = name.toLower();
    for (const auto &pattern : CONFIG_WHITELIST) {
        if (pattern.endsWith(QLatin1Char('*'))) {
            const QString prefix = pattern.left(pattern.length() - 1).toLower();
            if (lower.startsWith(prefix)) return true;
        } else if (pattern.contains(QLatin1Char('*'))) {
            const QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(pattern), QRegularExpression::CaseInsensitiveOption);
            if (rx.match(lower).hasMatch()) return true;
        } else {
            if (lower == pattern.toLower()) return true;
        }
    }
    return false;
}

void SystemAuditor::recalculateTotals()
{
    m_totalReclaimableBytes = 0;
    m_safeCount = 0;
    m_reviewCount = 0;

    for (const auto &item : m_items) {
        m_totalReclaimableBytes += item.sizeBytes;
        if (item.riskLevel == QStringLiteral("safe")) {
            m_safeCount++;
        } else {
            m_reviewCount++;
        }
    }

    emit itemsChanged();
}

void SystemAuditor::setItemSelected(const QString &id, bool selected)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            m_items[i].selected = selected;
            emit itemsChanged();
            return;
        }
    }
}

void SystemAuditor::selectAllSafe()
{
    for (auto &item : m_items) {
        item.selected = (item.riskLevel == QStringLiteral("safe"));
    }
    emit itemsChanged();
}

void SystemAuditor::deselectAll()
{
    for (auto &item : m_items) {
        item.selected = false;
    }
    emit itemsChanged();
}

void SystemAuditor::startScan()
{
    m_isScanning = true;
    emit isScanningChanged();
    m_items.clear();
    m_scanProgress = 0;
    emit scanProgressChanged();

    LogModel::instance()->append(
        QStringLiteral("Auditor"),
        QStringLiteral("info"),
        QStringLiteral("Starting comprehensive system audit...")
    );

    // Phase 1: Broken Symlinks
    m_scanStatusText = QStringLiteral("Scanning for broken symbolic links...");
    emit scanStatusTextChanged();
    scanBrokenSymlinks();
    m_scanProgress = 25;
    emit scanProgressChanged();

    // Phase 2: Pacman Orphans
    m_scanStatusText = QStringLiteral("Checking orphaned packages (pacman -Qdtq)...");
    emit scanStatusTextChanged();
    scanOrphanedPackages();
    m_scanProgress = 50;
    emit scanProgressChanged();

    // Phase 3: Pacman Cache
    m_scanStatusText = QStringLiteral("Inspecting package cache (/var/cache/pacman/pkg)...");
    emit scanStatusTextChanged();
    scanPackageCache();
    m_scanProgress = 75;
    emit scanProgressChanged();

    // Phase 4: Orphaned Configs
    m_scanStatusText = QStringLiteral("Checking orphaned configuration folders...");
    emit scanStatusTextChanged();
    scanOrphanedConfigs();
    m_scanProgress = 90;
    emit scanProgressChanged();

    // Phase 5: Forgotten / Failed Services
    m_scanStatusText = QStringLiteral("Checking failed user services...");
    emit scanStatusTextChanged();
    scanForgottenServices();

    m_scanProgress = 100;
    emit scanProgressChanged();
    m_scanStatusText = QStringLiteral("Scan complete.");
    emit scanStatusTextChanged();

    recalculateTotals();

    m_isScanning = false;
    emit isScanningChanged();

    LogModel::instance()->append(
        QStringLiteral("Auditor"),
        QStringLiteral("info"),
        QStringLiteral("Audit completed: found %1 item(s) totalling %2 reclaimable space")
            .arg(m_items.size())
            .arg(totalReclaimableFormatted())
    );
}

void SystemAuditor::scanBrokenSymlinks()
{
    const QStringList scanDirs = {
        QDir::homePath() + QStringLiteral("/.local/bin"),
        QDir::homePath() + QStringLiteral("/.local/share/applications"),
        QDir::homePath() + QStringLiteral("/Desktop"),
        QStringLiteral("/usr/local/bin")
    };

    int index = 0;
    for (const auto &dirPath : scanDirs) {
        if (!QFile::exists(dirPath)) {
            continue;
        }

        QDirIterator it(dirPath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            it.next();
            const QFileInfo fi = it.fileInfo();

            if (fi.isSymLink() && !fi.exists()) {
                AuditItem item;
                item.id = QStringLiteral("symlink_%1").arg(index++);
                item.category = QStringLiteral("symlinks");
                item.title = fi.fileName();
                item.subtitle = i18n("Points to missing: %1", fi.symLinkTarget());
                item.path = fi.absoluteFilePath();
                item.sizeBytes = 0;
                item.formattedSize = i18n("0 B");
                item.riskLevel = QStringLiteral("safe");
                item.selected = true;

                m_items.append(item);
            }
        }
    }
}

void SystemAuditor::scanOrphanedPackages()
{
    QProcess proc;
    proc.start(QStringLiteral("pacman"), {QStringLiteral("-Qdtq")});
    proc.waitForFinished(10000);

    if (proc.exitCode() != 0) {
        return;
    }

    const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (out.isEmpty()) {
        return;
    }

    const QStringList pkgs = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    int index = 0;

    for (const auto &pkg : pkgs) {
        const QString name = pkg.trimmed();
        if (name.isEmpty()) continue;

        // Query size and description
        QProcess qi;
        qi.start(QStringLiteral("pacman"), {QStringLiteral("-Qi"), name});
        qi.waitForFinished(3000);

        QString desc;
        qint64 sizeBytes = 0;

        if (qi.exitCode() == 0) {
            const QString qiOut = QString::fromUtf8(qi.readAllStandardOutput());
            const QStringList lines = qiOut.split(QLatin1Char('\n'));

            for (const auto &line : lines) {
                if (line.startsWith(QStringLiteral("Description")) && desc.isEmpty()) {
                    const int col = line.indexOf(QLatin1Char(':'));
                    if (col != -1) desc = line.mid(col + 1).trimmed();
                } else if (line.startsWith(QStringLiteral("Installed Size"))) {
                    const int col = line.indexOf(QLatin1Char(':'));
                    if (col != -1) {
                        const QString sizeStr = line.mid(col + 1).trimmed();
                        // e.g. "124.50 MiB" or "500.00 KiB"
                        const QStringList parts = sizeStr.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                        if (parts.size() >= 2) {
                            const double val = parts[0].toDouble();
                            const QString unit = parts[1].toLower();
                            if (unit.contains(QStringLiteral("gib"))) sizeBytes = static_cast<qint64>(val * 1024.0 * 1024.0 * 1024.0);
                            else if (unit.contains(QStringLiteral("mib"))) sizeBytes = static_cast<qint64>(val * 1024.0 * 1024.0);
                            else if (unit.contains(QStringLiteral("kib"))) sizeBytes = static_cast<qint64>(val * 1024.0);
                            else sizeBytes = static_cast<qint64>(val);
                        }
                    }
                }
            }
        }

        AuditItem item;
        item.id = QStringLiteral("orphan_%1").arg(index++);
        item.category = QStringLiteral("orphans");
        item.title = name;
        item.subtitle = desc.isEmpty() ? i18n("Orphaned dependency package") : desc;
        item.path = name;
        item.sizeBytes = sizeBytes;
        item.formattedSize = formatSize(sizeBytes);
        item.riskLevel = QStringLiteral("safe");
        item.selected = true;

        m_items.append(item);
    }
}

void SystemAuditor::scanPackageCache()
{
    // Check paccache dry-run: paccache -d -k 2
    QProcess proc;
    proc.start(QStringLiteral("paccache"), {QStringLiteral("-d"), QStringLiteral("-k"), QStringLiteral("2")});
    proc.waitForFinished(10000);

    if (proc.exitCode() == 0) {
        const QString out = QString::fromUtf8(proc.readAllStandardOutput());
        // Look for: "disk space saved: 8.17 GiB" or "candidates (disk space saved: X)"
        const QRegularExpression re(QStringLiteral(R"((\d+)\s+candidates\s*\(disk space saved:\s*([\d\.]+\s*[a-zA-Z]+)\))"));
        const auto match = re.match(out);

        if (match.hasMatch()) {
            const int candidates = match.captured(1).toInt();
            const QString savedStr = match.captured(2).trimmed();

            if (candidates > 0) {
                // Parse bytes from string
                const QStringList parts = savedStr.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                qint64 sizeBytes = 0;
                if (parts.size() >= 2) {
                    const double val = parts[0].toDouble();
                    const QString unit = parts[1].toLower();
                    if (unit.contains(QStringLiteral("gib"))) sizeBytes = static_cast<qint64>(val * 1024.0 * 1024.0 * 1024.0);
                    else if (unit.contains(QStringLiteral("mib"))) sizeBytes = static_cast<qint64>(val * 1024.0 * 1024.0);
                    else if (unit.contains(QStringLiteral("kib"))) sizeBytes = static_cast<qint64>(val * 1024.0);
                }

                AuditItem item;
                item.id = QStringLiteral("cache_paccache");
                item.category = QStringLiteral("cache");
                item.title = i18n("Pacman Package Cache (old package versions)");
                item.subtitle = i18n("%1 candidate archives in /var/cache/pacman/pkg (keeping last 2 versions)", candidates);
                item.path = QStringLiteral("/var/cache/pacman/pkg");
                item.sizeBytes = sizeBytes;
                item.formattedSize = savedStr;
                item.riskLevel = QStringLiteral("safe");
                item.selected = true;

                m_items.append(item);
            }
        }
    }
}

void SystemAuditor::scanOrphanedConfigs()
{
    // Retrieve list of installed package names to match against config directories
    QProcess proc;
    proc.start(QStringLiteral("pacman"), {QStringLiteral("-Qq")});
    proc.waitForFinished(8000);

    QSet<QString> installedPkgs;
    if (proc.exitCode() == 0) {
        const QString out = QString::fromUtf8(proc.readAllStandardOutput());
        const QStringList list = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const auto &p : list) {
            installedPkgs.insert(p.trimmed().toLower());
        }
    }

    const QString configDir = QDir::homePath() + QStringLiteral("/.config");
    QDir dir(configDir);
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);

    int index = 0;
    for (const auto &fi : entries) {
        const QString dirName = fi.fileName();
        if (isWhitelisted(dirName)) {
            continue;
        }

        const QString lower = dirName.toLower();
        if (installedPkgs.contains(lower)) {
            continue;
        }

        // Calculate size of this candidate directory
        const qint64 size = calculateDirSize(fi.absoluteFilePath());

        AuditItem item;
        item.id = QStringLiteral("config_%1").arg(index++);
        item.category = QStringLiteral("configs");
        item.title = dirName;
        item.subtitle = i18n("Directory in ~/.config with no active matching package");
        item.path = fi.absoluteFilePath();
        item.sizeBytes = size;
        item.formattedSize = formatSize(size);
        item.riskLevel = QStringLiteral("review");
        item.selected = false; // Never select review items by default

        m_items.append(item);
    }
}

void SystemAuditor::scanForgottenServices()
{
    QProcess proc;
    proc.start(QStringLiteral("systemctl"), {QStringLiteral("--user"), QStringLiteral("list-units"), QStringLiteral("--type=service"), QStringLiteral("--state=failed")});
    proc.waitForFinished(5000);

    if (proc.exitCode() != 0) {
        return;
    }

    const QString out = QString::fromUtf8(proc.readAllStandardOutput());
    const QStringList lines = out.split(QLatin1Char('\n'));
    int index = 0;

    for (const auto &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("UNIT")) || trimmed.startsWith(QStringLiteral("LOAD")) || trimmed.isEmpty()) {
            continue;
        }

        if (trimmed.contains(QStringLiteral(".service")) && trimmed.contains(QStringLiteral("failed"))) {
            const QStringList tokens = trimmed.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (!tokens.isEmpty()) {
                const QString srvName = tokens[0];

                AuditItem item;
                item.id = QStringLiteral("service_%1").arg(index++);
                item.category = QStringLiteral("services");
                item.title = srvName;
                item.subtitle = i18n("Failed user service hanging in systemd state");
                item.path = srvName;
                item.sizeBytes = 0;
                item.formattedSize = i18n("0 B");
                item.riskLevel = QStringLiteral("review");
                item.selected = false;

                m_items.append(item);
            }
        }
    }
}

void SystemAuditor::cleanSelected()
{
    m_isCleaning = true;
    emit isCleaningChanged();

    QStringList orphanPkgsToRemove;
    bool cleanCache = false;
    QStringList symlinksToRemove;
    QStringList configsToRemove;
    QStringList servicesToReset;

    for (const auto &item : m_items) {
        if (!item.selected) {
            continue;
        }

        if (item.category == QStringLiteral("symlinks")) {
            symlinksToRemove.append(item.path);
        } else if (item.category == QStringLiteral("orphans")) {
            orphanPkgsToRemove.append(item.path);
        } else if (item.category == QStringLiteral("cache")) {
            cleanCache = true;
        } else if (item.category == QStringLiteral("configs")) {
            if (!isWhitelisted(QFileInfo(item.path).fileName())) {
                configsToRemove.append(item.path);
            }
        } else if (item.category == QStringLiteral("services")) {
            servicesToReset.append(item.path);
        }
    }

    int removedCount = 0;

    // 1. Remove broken symlinks
    for (const auto &linkPath : symlinksToRemove) {
        if (QFile::remove(linkPath)) {
            removedCount++;
            LogModel::instance()->append(QStringLiteral("Auditor"), QStringLiteral("info"), QStringLiteral("Removed dead symlink: %1").arg(linkPath));
        }
    }

    // 2. Remove orphaned configs
    for (const auto &dirPath : configsToRemove) {
        if (!isWhitelisted(QFileInfo(dirPath).fileName())) {
            QDir dir(dirPath);
            if (dir.removeRecursively()) {
                removedCount++;
                LogModel::instance()->append(QStringLiteral("Auditor"), QStringLiteral("info"), QStringLiteral("Removed orphaned config directory: %1").arg(dirPath));
            }
        }
    }

    // 3. Reset failed services
    for (const auto &srv : servicesToReset) {
        QProcess proc;
        proc.start(QStringLiteral("systemctl"), {QStringLiteral("--user"), QStringLiteral("reset-failed"), srv});
        proc.waitForFinished(3000);
        removedCount++;
        LogModel::instance()->append(QStringLiteral("Auditor"), QStringLiteral("info"), QStringLiteral("Reset failed service: %1").arg(srv));
    }

    // 4. Clean pacman cache if selected
    if (cleanCache) {
        auto runner = new PkexecRunner(this);
        connect(runner, &PkexecRunner::finished, this, [this](int exitCode, const QString &output) {
            if (exitCode == 0) {
                LogModel::instance()->append(QStringLiteral("Auditor"), QStringLiteral("info"), QStringLiteral("Cleaned pacman package cache"));
            } else {
                LogModel::instance()->append(QStringLiteral("Auditor"), QStringLiteral("warn"), QStringLiteral("Pacman cache cleanup reported: %1").arg(output));
            }
        });
        runner->runRoot(QStringLiteral("paccache"), {QStringLiteral("-r"), QStringLiteral("-k"), QStringLiteral("2")});
    }

    // 5. Remove orphan packages via pacman -Rns
    if (!orphanPkgsToRemove.isEmpty()) {
        auto runner = new PkexecRunner(this);
        connect(runner, &PkexecRunner::finished, this, [this](int exitCode, const QString &output) {
            m_isCleaning = false;
            emit isCleaningChanged();

            if (exitCode == 0) {
                LogModel::instance()->append(QStringLiteral("Auditor"), QStringLiteral("info"), QStringLiteral("Removed orphaned packages successfully"));
                emit operationFinished(true, i18n("Cleaned selected items successfully."));
            } else if (exitCode == 126 || exitCode == 127) {
                emit operationFinished(false, i18n("Package removal cancelled by user."));
            } else {
                emit operationFinished(false, i18n("Failed to remove packages: %1", output));
            }
            startScan();
        });

        QStringList args = {QStringLiteral("-Rns"), QStringLiteral("--noconfirm")};
        args.append(orphanPkgsToRemove);
        runner->runRoot(QStringLiteral("pacman"), args);
        return;
    }

    m_isCleaning = false;
    emit isCleaningChanged();
    emit operationFinished(true, i18n("Cleanup completed successfully."));
    startScan();
}
