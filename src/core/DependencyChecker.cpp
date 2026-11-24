#include "DependencyChecker.h"

#include <QStandardPaths>
#include <QMap>

DependencyChecker::DependencyChecker(QObject *parent)
    : QObject(parent)
{
}

bool DependencyChecker::hasCommand(const QString &name) const
{
    if (name.isEmpty()) {
        return false;
    }
    return !QStandardPaths::findExecutable(name).isEmpty();
}

QStringList DependencyChecker::missing(const QStringList &names) const
{
    QStringList result;
    for (const auto &name : names) {
        if (!hasCommand(name)) {
            result.append(name);
        }
    }
    return result;
}

QString DependencyChecker::installHint(const QString &name) const
{
    static const QMap<QString, QString> s_hints = {
        {QStringLiteral("smartctl"), QStringLiteral("pacman -S smartmontools")},
        {QStringLiteral("fd"), QStringLiteral("pacman -S fd")},
        {QStringLiteral("rg"), QStringLiteral("pacman -S ripgrep")},
        {QStringLiteral("ripgrep"), QStringLiteral("pacman -S ripgrep")},
        {QStringLiteral("sbctl"), QStringLiteral("pacman -S sbctl")},
        {QStringLiteral("nmcli"), QStringLiteral("pacman -S networkmanager")},
        {QStringLiteral("resolvectl"), QStringLiteral("pacman -S systemd")},
        {QStringLiteral("dig"), QStringLiteral("pacman -S bind")},
        {QStringLiteral("keyd"), QStringLiteral("pacman -S keyd")},
        {QStringLiteral("auto-cpufreq"), QStringLiteral("AUR: auto-cpufreq (yay -S auto-cpufreq)")},
        {QStringLiteral("libinput-gestures"), QStringLiteral("AUR: libinput-gestures (yay -S libinput-gestures)")},
        {QStringLiteral("paccache"), QStringLiteral("pacman -S pacman-contrib")},
        {QStringLiteral("flatpak"), QStringLiteral("pacman -S flatpak")},
        {QStringLiteral("snap"), QStringLiteral("pacman -S snapd")}
    };

    if (s_hints.contains(name)) {
        return s_hints.value(name);
    }
    return QStringLiteral("pacman -S %1").arg(name);
}
