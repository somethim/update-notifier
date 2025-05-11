#!/bin/bash

# Check if the script is run as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root"
   exit 1
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if systemd is available
systemd_available() {
    [ -d "/run/systemd/system" ] || [ -d "/var/run/systemd/system" ]
}

# Create directory if it doesn't exist
mkdir -p /opt/package-updater

# Set the working directory
cd "$(dirname "$0")"

# Check for required build tools
if ! command_exists gcc; then
    echo "Error: gcc is required but not installed."
    echo "Please install gcc and try again."
    exit 1
fi

# Compile the C source code
echo "Compiling updater.c..."
gcc -Wall -Wextra -O2 -o updater updater.c

# Check if compilation was successful
if [[ $? -ne 0 ]]; then
    echo "Compilation failed!"
    exit 1
fi

# Copy the compiled binary to destination
echo "Installing binary to /opt/package-updater/"
cp updater /opt/package-updater/
chmod 755 /opt/package-updater/updater

# Create an SVG icon for the application
echo "Installing icon..."
cp update.png /opt/package-updater/icon.png

# Set proper ownership and permissions
chown -R root:root /opt/package-updater
chmod -R 755 /opt/package-updater

# Check for zenity
if ! command_exists zenity; then
    echo "Warning: zenity is not installed. The GUI interface will not work."
    echo "Please install zenity for full functionality."
fi

echo "Installation complete!"
echo "You can now run the updater with: /opt/package-updater/updater"
echo "To install the desktop file: /opt/package-updater/updater --install-desktop"
echo "To setup autostart: /opt/package-updater/updater --setup-autostart"

# Check if systemd is available
if systemd_available; then
    echo "Systemd detected. The updater will be configured to run automatically."
else
    echo "Systemd not detected. Using autostart for automatic updates."
fi

# Run the updater to setup automatic updates
echo "Setting up automatic updates..."
/opt/package-updater/updater --setup-autostart