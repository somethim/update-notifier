CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = updater
INSTALL_DIR = /opt/package-updater
SYSTEMD_USER_DIR = $(HOME)/.config/systemd/user

all: $(TARGET)

$(TARGET): updater.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "This target must be run as root"; \
		exit 1; \
	fi
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/
	chmod 755 $(INSTALL_DIR)/$(TARGET)
	@echo "Installing icon..."
	cp update.png $(INSTALL_DIR)/icon.png
	chown -R root:root $(INSTALL_DIR)
	chmod -R 755 $(INSTALL_DIR)
	@echo "Installation complete!"
	@echo "You can now run the updater with: $(INSTALL_DIR)/$(TARGET)"
	@echo "To install the desktop file: $(INSTALL_DIR)/$(TARGET) --install-desktop"
	@echo "To setup autostart: $(INSTALL_DIR)/$(TARGET) --setup-autostart"
	@echo "The updater will automatically configure systemd timer if available"

setup-desktop: install
	@echo "Setting up desktop entry..."
	$(INSTALL_DIR)/$(TARGET) --install-desktop

setup-autostart: install
	@echo "Setting up autostart entry..."
	$(INSTALL_DIR)/$(TARGET) --setup-autostart

uninstall:
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "This target must be run as root"; \
		exit 1; \
	fi
	@echo "Removing package updater..."
	rm -rf $(INSTALL_DIR)
	@echo "Removing systemd files..."
	rm -f $(SYSTEMD_USER_DIR)/package-updater.service
	rm -f $(SYSTEMD_USER_DIR)/package-updater.timer
	@echo "Removing desktop files..."
	rm -f $(HOME)/.local/share/applications/package-updater.desktop
	rm -f $(HOME)/.config/autostart/package-updater.desktop
	@echo "Uninstallation complete!"

.PHONY: all clean install setup-desktop setup-autostart uninstall