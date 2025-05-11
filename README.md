# Package Update Notifier

A simple C applet to check for and notify about system and application updates on Linux. It supports Homebrew, DNF, Flatpak, and Snap, and provides a graphical interface using Zenity.

## Features
- Checks for updates from multiple package managers: Homebrew, DNF, Flatpak, Snap
- Shows a Zenity dialog listing the actual packages that need updating
- Dialog is thinner and longer for better readability (`400x300`)
- Robust handling of long file paths (uses `PATH_MAX` and checks for truncation)
- Desktop integration and autostart/systemd timer support
- Notifies if no updates are available
- User-friendly error messages if a path is too long
- Supports both per-user and system-wide desktop/autostart integration

## How it works
- On launch, the app checks for updates from all supported package managers.
- If updates are found, a Zenity dialog appears with:
  - A summary of updates per package manager
  - A list of all packages that need updating (as selectable rows)
  - Action options at the top: "Brew, Snap & Flatpak", "Open Software Manager", "Check for updates again"
- If no updates are found, a message is shown and the app exits.
- The dialog is sized at 400x300 for a compact, readable appearance.

## Installation
1. Build and install:
   ```sh
   sudo make install
   ```
2. (Optional) Add to desktop or autostart (per-user, no root needed):
   ```sh
   /opt/package-updater/updater --install-desktop
   /opt/package-updater/updater --setup-autostart
   ```
   Or for system-wide (all users, requires root):
   ```sh
   sudo /opt/package-updater/updater --install-desktop --system-wide
   sudo /opt/package-updater/updater --setup-autostart --system-wide
   ```

## Usage
- Run `/opt/package-updater/updater` to check for updates interactively.
- Use the desktop entry or autostart for background notifications.
- The app will automatically configure a systemd timer if available.
- Use `--system-wide` with `--install-desktop` or `--setup-autostart` to install for all users (requires root).

## Supported Package Managers
- Homebrew
- DNF
- Flatpak
- Snap

## Robust Path Handling
- All file path buffers use `PATH_MAX` for safety.
- If a path is too long for the buffer, a warning is shown and the operation is aborted.

## Requirements
- Zenity (will prompt to install if missing)
- At least one supported package manager

## Uninstallation
```sh
sudo make uninstall
```

## License
MIT