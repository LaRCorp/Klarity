# Klarity

<div align="center">

![Klarity Logo](resources/icons/klarity.svg)

**Modern, modular, GUI-first system administration & optimization hub for Linux (KDE Plasma 6 / Wayland)**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Qt](https://img.shields.io/badge/Qt-6.12-green.svg)](https://www.qt.io/)
[![KDE Frameworks](https://img.shields.io/badge/KDE%20Frameworks-6-1d99f3.svg)](https://develop.kde.org/frameworks/)
[![Kirigami](https://img.shields.io/badge/UI-Kirigami%206-orange.svg)](https://develop.kde.org/frameworks/kirigami/)
[![Platform](https://img.shields.io/badge/Platform-Arch%20%7C%20CachyOS%20%7C%20Linux-lightgrey.svg)](https://archlinux.org)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

</div>

---

## 🌟 Overview

**Klarity** is an all-in-one system administration, performance tuning, and diagnostics center built specifically for KDE Plasma 6 on Wayland and X11. It brings together low-level Linux administration tools into an elegant, responsive Kirigami user interface.

With Klarity, you no longer need to remember dozens of complex terminal commands or edit configuration files by hand.

---

## 🚀 Key Modules & Capabilities

Klarity includes 13 purpose-built administrative modules:

### 1. ⚡ Power & Performance
* **Automatic Mode:** Full integration with the `auto-cpufreq` daemon (start, stop, profile configuration, live monitoring).
* **Expert Sysfs Mode:** Direct frequency scaling, governor selection (`performance`, `powersave`, `schedutil`), and CPU turbo control.
* **Live Telemetry:** Real-time 1Hz monitoring of CPU temperatures, frequencies, and battery status.

### 2. 💽 Disk Health & S.M.A.R.T.
* **Drive Health Diagnostics:** Automatic detection of NVMe and SATA drives via `smartctl` with comprehensive JSON telemetry parsing.
* **Failure Indicators:** Real-time tracking of critical parameters (Reallocated Sectors, Unsafe Shutdowns, SSD Wear Percentage, Media Errors).
* **Self-Tests & Daemon:** Trigger Short/Long SMART tests and manage background monitoring via `smartd`.

### 3. 🔒 Secure Boot Manager
* **Guided Workflow:** Step-by-step assistance for `sbctl` (1. Verify unsigned files → 2. Sign kernels/bootloaders → 3. Enroll keys).
* **Real-time Status:** Color-coded badges for Secure Boot, Setup Mode, and Custom Keys.
* **File Management:** List and sign detected EFI binaries.

### 4. 📦 Applications & Packages
* **Multi-Source Catalog:** Unified view of Native Packages (`pacman`), AUR, Flatpak, and Portable Apps (AppImage / ELF).
* **Smart Filtering:** Cleanly separates user-installed GUI apps from system libraries and dependencies. Filter by category (Internet, Development, Graphics, Multimedia, Office, Games, System, Utilities).
* **Portable App Integration:** Discover and integrate standalone AppImages into the KDE Plasma application menu with 1 click.

### 5. 🌐 DNS Center
* **Resolver Management:** View active DNS resolvers configured in `systemd-resolved` and NetworkManager.
* **Latency Benchmark:** Benchmark and compare DNS query latencies across Cloudflare, Google, AdGuard, and Quad9.
* **DNS Leak Protection:** Detect active outbound resolvers to protect privacy from ISP interception.

### 6. 🔍 Global File Search
* **High-Performance Finder:** Leverages `fd` and `ripgrep` for blazing-fast file and content searches across home directories and root filesystem.
* **History & Suggestions:** Intelligent debounced search history and quick extension filters.

### 7. 🔄 System Updates
* **Multi-Backend Caching:** Queries `pacman`, `yay`/`paru`, and `flatpak` with a 30-minute memory cache to prevent UI freezing.
* **Arch News Feed:** Integrated news browser to check manual intervention announcements before running full system upgrades (`pacman -Syu`).

### 8. ⌨️ Shortcuts & Keybindings
* **Zero-Lag Conflict Resolver:** Asynchronously parses KWin, Plasma, and global shortcut configurations in a background worker thread.
* **Collision Detection:** Visual conflict notifications with one-click reassign and unbind actions.

### 9. 🧹 System Auditor
* **Disk Cleaner:** Scans for reclaimable space (package caches, orphan packages, broken symlinks, user cache, leftover configs).
* **Safe vs Review:** Clear risk stratification with confirmation dialogs before removing files.

### 10. 🖱️ Touchpad Gestures
* Visual configuration and service controller for `libinput-gestures` (Swipe, Pinch, Multi-finger actions).

### 11. 🎹 Keyboard Dispatcher (keyd)
* Configure low-level key remappings, layered bindings, and macros via the `keyd` daemon.

### 12. 🐚 Shell Aliases & Functions
* Visual editor for Fish, Bash, and Zsh aliases and functions with real-time syntax checking.

### 13. 🚀 Autostart Management
* Manage desktop autostart entries (`~/.config/autostart`) and user systemd services (`systemctl --user`).

---

## 🛡️ Security Architecture

* **Polkit (pkexec):** Privileged actions (package removal, service manipulation, Secure Boot signing, sysfs governors) use policy-based privilege escalation through standard system Polkit dialogs.
* **No Password Storage:** Klarity never requests, handles, or stores user passwords.
* **Isolated Background Workers:** Heavy I/O, network benchmarks, and disk queries run on isolated worker threads (`QThread`), ensuring the UI remains 100% fluid and responsive.

---

## 🛠️ Building & Installation

### Requirements (Arch Linux / CachyOS / Fedora)

```bash
# Core Dependencies
sudo pacman -S cmake gcc qt6-base qt6-declarative qt6-tools \
               kcoreaddons kconfig ki18n kirigami \
               polkit smartmontools sbctl fd ripgrep
```

### Build from Source

```bash
git clone https://github.com/larcorp/klarity.git
cd klarity

# Configure CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build -j$(nproc)

# Run
./build/klarity
```

### Running Tests

Klarity includes a comprehensive test suite (37+ tests) validating C++ backends and offscreen QML page instantiation:

```bash
./build/klarity_tests
```

---

## 📜 License

Klarity is open-source software licensed under the **GNU General Public License v3.0** (GPL-3.0).
See [LICENSE](LICENSE) for more information.

Copyright © 2026 LaRCorp. All rights reserved.
