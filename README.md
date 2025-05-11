# Package Updater

A compiled C application that checks for package updates across multiple package managers (Homebrew, DNF, Flatpak, and
Snap).

## Features

- Checks for updates across multiple package managers
- Shows notifications when updates are available
- Provides an interactive dialog for managing updates
- Can be set to start automatically at login
- Creates desktop menu entry for easy access

## Installation

### Prerequisites

Make sure you have the following dependencies installed:

- GCC (GNU Compiler Collection)
- Zenity (for GUI dialogs)
- Ptyxis or another terminal emulator (for executing updates)
- The package managers you want to check (Homebrew, DNF, Flatpak, Snap)

### Installation Steps

1. Clone or download this repository
2. Open a terminal in the project directory
3. Run the installation script as root:

```bash
sudo ./install.sh
```

Alternatively, you can use the provided Makefile:

```bash
sudo make install
```

### Setting Up Desktop Entry and Autostart

After installation, you can set up the desktop entry and autostart with:

```bash
# Create desktop entry
/opt/package-updater/updater --install-desktop

# Set up autostart
/opt/package-updater/updater --setup-autostart
```

Or use the Makefile targets:

```bash
sudo make setup-desktop
sudo make setup-autostart
```

## Usage

### Running the Updater

You can run the updater in several ways:

- From the application menu (if desktop entry is installed)
- From the terminal:

```bash
/opt/package-updater/updater
```

### Command Line Options

```
Usage: /opt/package-updater/updater [OPTION]
Package Update Notification Program

Options:
  --install-desktop  Create desktop file in applications menu
  --setup-autostart  Configure program to run at startup
  --notify-only      Show notification if updates available
  --help             Display this help

With no options, runs the interactive update dialog.
```

## Uninstallation

To uninstall the package updater:

```bash
sudo rm -rf /opt/package-updater
rm -f ~/.local/share/applications/package-updater.desktop
rm -f ~/.config/autostart/package-updater.desktop
```