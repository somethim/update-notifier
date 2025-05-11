CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = updater
INSTALL_DIR = /opt/package-updater
USER_HOME := $(if $(SUDO_USER),/home/$(SUDO_USER),$(HOME))
SYSTEMD_USER_DIR := $(USER_HOME)/.config/systemd/user

all: install

install:
	@if [ "$(shell id -u)" -ne 0 ]; then \
		echo "This target must be run as root"; \
		exit 1; \
	fi
	mkdir -p $(INSTALL_DIR)
	$(CC) $(CFLAGS) -o $(INSTALL_DIR)/$(TARGET) updater.c
	chmod 755 $(INSTALL_DIR)/$(TARGET)
	@echo "Installing icon..."
	cp update.png $(INSTALL_DIR)/icon.png
	chown -R root:root $(INSTALL_DIR)
	chmod -R 755 $(INSTALL_DIR)
	@echo "Installation complete!"
	@echo "You can now run the updater with: $(INSTALL_DIR)/$(TARGET)"
	@echo "To install the desktop file (per-user): $(INSTALL_DIR)/$(TARGET) --install-desktop"
	@echo "To install the desktop file (system-wide, all users): sudo $(INSTALL_DIR)/$(TARGET) --install-desktop --system-wide"
	@echo "To setup autostart (per-user): $(INSTALL_DIR)/$(TARGET) --setup-autostart"
	@echo "To setup autostart (system-wide, all users): sudo $(INSTALL_DIR)/$(TARGET) --setup-autostart --system-wide"
	@echo "The updater will automatically configure systemd timer if available"

setup-desktop: install
	@echo "Setting up desktop entry..."
	$(INSTALL_DIR)/$(TARGET) --install-desktop

setup-autostart: install
	@echo "Setting up autostart entry..."
	$(INSTALL_DIR)/$(TARGET) --setup-autostart

clean:
	rm -f $(INSTALL_DIR)/$(TARGET)

uninstall:
	@if [ "$(shell id -u)" -ne 0 ]; then \
		echo "This target must be run as root"; \
		exit 1; \
	fi
	@echo "Removing package updater..."
	rm -rf $(INSTALL_DIR)
	@echo "Removing systemd files..."
	rm -f $(SYSTEMD_USER_DIR)/package-updater.service
	rm -f $(SYSTEMD_USER_DIR)/package-updater.timer
	@echo "Removing desktop files..."
	rm -f $(USER_HOME)/.local/share/applications/package-updater.desktop
	rm -f $(USER_HOME)/.config/autostart/package-updater.desktop
	@echo "Uninstallation complete!"

.PHONY: all clean install setup-desktop setup-autostart uninstall