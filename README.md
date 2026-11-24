<div align="center">
  <img src="resources/icons/klarity.svg" width="96" height="96" alt="Klarity Logo" />
  <h1>⚙️🛡️ Klarity v1.1 — Linux Administration & Optimization Control Center</h1>
  <p><strong>A unified native center for system administration, security hardening, and deep customization for Arch Linux and KDE Plasma 6</strong></p>

  <p>
    <strong>English</strong> | <a href="README_RU.md">Русский</a>
  </p>

  <p>
    <a href="https://github.com/LaRCorp/Klarity/releases"><img src="https://img.shields.io/badge/version-1.1-6366f1?style=flat-square" alt="Version 1.1" /></a>
    <img src="https://img.shields.io/badge/Platform-Arch%20Linux%20%7C%20CachyOS-0ea5e9?style=flat-square" alt="Arch Linux | CachyOS" />
    <img src="https://img.shields.io/badge/Qt%206.12-KF6%20%7C%20C%2B%2B20-8b5cf6?style=flat-square" alt="Qt 6.12 + KF6 + C++20" />
    <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-10b981?style=flat-square" alt="GPLv3 License" /></a>
    <img src="https://img.shields.io/badge/Desktop-KDE%20Plasma%206-f59e0b?style=flat-square" alt="KDE Plasma 6" />
    <img src="https://img.shields.io/badge/Security-Polkit%20%7C%20Secure%20Boot-ec4899?style=flat-square" alt="Polkit + Secure Boot" />
    <img src="https://img.shields.io/badge/i18n-English%20%7C%20Русский-10b981?style=flat-square" alt="Localization" />
  </p>
</div>

**Klarity** is a lightweight, native, and high-performance administration, security, and optimization control center for Linux.
Autostart services, shell aliases & functions, global shortcuts, touchpad gestures, kernel-level keyboard remapping (`keyd`), system security auditing and cleaning, UEFI Secure Boot key enrollment, package management (Pacman, Flatpak, Portable Apps), DNS configuration, lightning-fast file search, system updates, CPU power profiles, and S.M.A.R.T. disk telemetry are unified into a single coherent application adhering strictly to KDE Plasma 6 standards.

> 🛡️ **Secure Architecture:** Klarity runs strictly as an unprivileged user. Privileged system operations (package installation, DNS configuration, sbctl keys, power profiles) are executed in isolation via standard **Polkit (`pkexec`)** with an interactive real-time audit log.

Built with **C++20**, **Qt 6.12**, and **KDE Frameworks 6 (Kirigami 6)**. Fully compatible with both Wayland and X11, ships with bilingual localization (English & Russian), contains zero third-party telemetry, and is engineered for instantaneous UI responsiveness.

---

## Features

### 🚀 Autostart & Background Services
- Manage Desktop Autostart entries (`~/.config/autostart/`) and user-level Systemd units (`~/.config/systemd/user/`).
- Instantly enable, disable, create, and safely remove autostart entries.
- Filter by type (Desktop / Systemd) and scope (User / System), with search by name and executable path.
- Adaptive card layout: automatic title truncation (`elide`) and strict vertical alignment of toggle switches.

### 🐚 Shell Aliases & Functions
- Read, parse, and edit aliases and custom shell functions for Bash (`~/.bashrc`), Zsh (`~/.zshrc`), and Fish (`~/.config/fish/`).
- Create and update aliases with syntax validation.
- Automatic creation of safety backups (`.bak`) before modifying configuration files.

### ⌨️ Shortcuts & Keybindings
- Deep integration with `kglobalaccel` (KDE Plasma 6) and the `kded6` daemon.
- Detect and resolve conflicts between system-wide shortcuts and third-party applications.
- Asynchronous scanning on a background `QThread` without UI stutter or latency.

### 👆 Touchpad Gestures
- Gesture management powered by `libinput-gestures` for 3-finger and 4-finger swipes and pinch gestures.
- Built-in presets: virtual desktop switching, window overview (Overview / Present Windows), browser Back/Forward navigation.
- Preserves custom comments and directives within configuration files.

### 🖮 Kernel Keyboard Dispatcher (keyd)
- Integration with the low-level `keyd` remapping daemon (kernel evdev/uinput level).
- Popular presets: `CapsLock` overloaded as `Escape` on tap and `Control` on hold, Dual-role modifiers, navigation layers (Nav Layer).
- Pre-flight syntax validation (`keyd check`) and privileged daemon reload via Polkit.

### 🛡️ System Auditor & Cleaner
- Comprehensive hygiene audit to reclaim disk space:
  - Broken symlinks in the user home directory.
  - Orphaned packages (unneeded dependencies via `pacman -Qdtq`).
  - Obsolete package versions in `/var/cache/pacman/pkg` (`paccache`).
  - Leftover configuration folders in `~/.config` from uninstalled packages.
  - Failed systemd user services hanging in a failed state.
- Safe cleaning workflow with reclaimable space calculation and safety warnings for review items.

### 🔒 Secure Boot Hardening (sbctl)
- Complete graphical interface for the `sbctl` key management utility.
- Monitor UEFI modes (Setup Mode, Secure Boot Enrolled, TPM chain status).
- Scan and sign bootloaders and kernels (`systemd-boot`, GRUB, UKI, `vmlinuz-*`).
- One-click custom key generation and UEFI NVRAM enrollment (with or without Microsoft OEM certificates).

### 📦 Applications & Package Manager
- Unified hub for installed packages: **Pacman** (official repositories), **Flatpak**, and **Portable Apps** (AppImages and standalone binaries in `~/.local/bin`, `~/Applications`, `/opt`).
- Automatic categorization following Freedesktop standards: Development, Multimedia, Utilities, Network, System, Office, Games.
- Flexible filters: GUI applications, dependencies (Explicit / Dependency), portable applications.
- Direct launch and one-click uninstall with complete metadata cleanup.

### 🌐 DNS Center
- Monitor active resolvers and gateways via `resolvectl` (systemd-resolved) and `NetworkManager`.
- Detect active DNS-over-TLS (DoT) and DNS-over-HTTPS (DoH) encryption.
- Rapid switching between trusted providers: **Cloudflare** (1.1.1.1), **Google** (8.8.8.8), **AdGuard** (ad & tracker blocking), **Quad9** (malware protection), and custom servers.
- Built-in DNS latency benchmark and DNS leak detection.

### 🔍 Global File Search
- High-speed search engine powered by `fd` (path search) and `ripgrep` (content search) with graceful fallback to `find/grep`.
- Advanced syntax: filter by file size (`>10M`, `<500K`), extensions (`*.service`), modification age (`<2d`), permissions.
- Interactive suggestion chips and debounced search history to prevent disk saturation.

### 🔄 System Updates
- Simultaneous update audit: Pacman (official repositories), Flatpak, and AUR (`yay` / `paru`).
- Smart 30-minute caching to reduce network bandwidth and CPU usage.
- Integrated **Arch Linux News** feed to alert users of required manual intervention prior to upgrades.
- Real-time streaming terminal log for update execution.

### ⚡ Power & Performance
- Manage the `auto-cpufreq` daemon (automatic CPU speed & power optimizer for laptops and desktops).
- Real-time telemetry: per-core frequencies, CPU Package temperatures, active scaling driver, and Turbo Boost status.
- Configurable profiles for AC power and battery modes with battery charge threshold controls.
- Expert direct sysfs frequency scaling mode with safety confirmation.

### 💾 Disk Health & S.M.A.R.T.
- In-depth drive diagnostics for NVMe and SATA drives via `smartctl`.
- Critical reliability indicators: SSD Wear Level, reallocated sectors, temperature, unsafe shutdowns.
- Trigger short and extended S.M.A.R.T. self-tests with abort capability.
- Full interactive S.M.A.R.T. attribute table with visual highlighting of warning and critical parameters.

### 🛠️ System Overview & Diagnostics
- Environmental summary: Linux distribution, kernel release, Wayland / X11 session, CPU/GPU hardware.
- Real-time dependency validator with one-click installation commands for missing tools.

### 🖥️ Native UI & Ergonomics
- **KDE Plasma 6 Native:** Built on Kirigami 6 to blend seamlessly into your desktop environment, automatically adopting system color palettes and fonts.
- **Full Localization:** Ships with built-in English and Russian translations using KDE Frameworks KI18n / gettext.
- **Dock Navigation Drawer:** Persistent sidebar featuring all 14 modules with instantaneous page switching and preserved tab states.
- **Responsive Layout:** Dynamically adjusts to any display resolution, from compact laptop screens to ultrawide 4K monitors.
- **Slide-out Log Drawer:** Centralized execution log with colored log levels (info, warning, error) and clipboard export.

---

## Tech Stack

| Component | Technology |
| --- | --- |
| Core Language | C++20 |
| GUI Framework | Qt 6.12, Qt Quick, QML |
| System Libraries | KDE Frameworks 6 (Kirigami 6, KConfig, KI18n, KCoreAddons) |
| Localization (i18n) | GNU Gettext / KI18n (ru, en) |
| Build System | CMake 3.22+, Ninja, Automoc |
| Privilege Architecture | Polkit (`pkexec`) |
| Search Backends | `fd`, `ripgrep` (fallback: `find`, `grep`) |
| Network & Resolvers | `systemd-resolved`, `NetworkManager`, `dig`, `nslookup` |
| Power Management | `auto-cpufreq`, Linux sysfs cpufreq/intel_pstate/amd-pstate |
| Drive Diagnostics | `smartmontools` (`smartctl`) |
| Test Suite | `QtTest` (37 unit & offscreen QML rendering tests) |
| Linter | `qmllint` (strict syntax and type verification) |

---

## 📥 Quick Start (Prebuilt Binaries)

No source compilation required — prebuilt binaries and archives are available directly on the **[Releases](https://github.com/LaRCorp/Klarity/releases/tag/v1.1)** page:

### Option 1: Standalone Binary
Download the `klarity` executable, mark it executable, and launch:
```bash
chmod +x klarity
./klarity
```

### Option 2: Full System Installation (Desktop Entry & Icons)
Download `klarity-v1.1-linux-x86_64.tar.gz` and run the installer:
```bash
tar -xzf klarity-v1.1-linux-x86_64.tar.gz
cd klarity-v1.1-linux-x86_64
./install.sh
```
*Installs the binary to `~/.local/bin/klarity`, desktop entry into KDE Plasma / GNOME application menus, and vector icon. No root privileges required.*

---

## 🛠️ Building from Source (Development)

### Prerequisites
- **Arch Linux**, **CachyOS**, **Manjaro**, or any Arch-based distribution.
- **KDE Plasma 6** (Wayland or X11).
- C++20 compliant compiler (GCC 13+ or Clang 16+).

### Install Build Dependencies
```bash
sudo pacman -S --needed \
    base-devel cmake ninja git gettext \
    qt6-base qt6-declarative \
    kirigami kconfig ki18n kcoreaddons \
    polkit
```

### Recommended Runtime Tools
```bash
sudo pacman -S --needed \
    fd ripgrep bind smartmontools \
    libinput-gestures sbctl auto-cpufreq
```

### Clone & Build
```bash
git clone https://github.com/LaRCorp/Klarity.git
cd Klarity

# Configure and compile
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Launch
./build/klarity
```

---

## Testing & Quality Assurance

Klarity includes an automated test suite verifying configuration parsers, ring-buffer logging, environment detection, and offscreen QML rendering across all pages:

```bash
# Run full unit test suite (37 tests)
./build/klarity_tests

# QML syntax and type validation
qmllint qml/Main.qml qml/modules/*.qml qml/components/*.qml
```

---

## 🤝 Credits & Acknowledgements

Klarity acts as a native graphical control center and orchestrator over battle-tested Linux tools and components. We express our gratitude to the authors and maintainers of:

- **[sbctl](https://github.com/Foxboron/sbctl)** (MIT) — by Morten Linderud (*Foxboron*). Secure Boot key manager.
- **[keyd](https://github.com/rvaiya/keyd)** (MIT) — by Rahul Rajan (*rvaiya*). Kernel-level keyboard remapping daemon.
- **[auto-cpufreq](https://github.com/AdnanHodzic/auto-cpufreq)** (LGPL-3.0) — by Adnan Hodzic. Automatic CPU speed & power optimizer.
- **[libinput-gestures](https://github.com/bulletmark/libinput-gestures)** (GPL-3.0) — by Mark Blakeney (*bulletmark*). Touchpad gestures translation utility.
- **[smartmontools](https://www.smartmontools.org/)** (GPL-2.0+) — Smartmontools Team. S.M.A.R.T. monitoring utilities (`smartctl`).
- **[ripgrep](https://github.com/BurntSushi/ripgrep)** (MIT / Unlicense) — by Andrew Gallant (*BurntSushi*). High-performance content search.
- **[fd](https://github.com/sharkdp/fd)** (MIT / Apache-2.0) — by David Peter (*sharkdp*). Fast and user-friendly path search.
- **[KDE Community](https://kde.org/) & [Kirigami](https://develop.kde.org/frameworks/kirigami/)** (LGPL-2.0+) — For the desktop components and Kirigami 6 framework.
- **[Qt Project](https://www.qt.io/)** (LGPL-3.0 / GPL-3.0) — For Qt Quick and QML.

---

## License

Released under the **GNU General Public License v3.0** (GPLv3). See [LICENSE](LICENSE) for details.

Copyright (c) 2026 LaRevole (LaRCorp).

Crafted with respect for Linux, customizability, and the KDE ecosystem.
