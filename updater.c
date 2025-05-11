#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

// Buffer sizes
#define PATH_MAX_SIZE PATH_MAX
#define CMD_MAX_SIZE 2048
#define BUFFER_SIZE 8192

// Forward declarations
void run_update_dialog();

void setup_autostart();

void create_desktop_file();

void notify_mode();

void show_help(const char *program_name);

// Function to execute a command and get its output
char *execute_command(const char *cmd) {
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        return nullptr;
    }

    char *result = malloc(BUFFER_SIZE);
    if (!result) {
        pclose(pipe);
        return nullptr;
    }

    char buffer[128];
    result[0] = '\0';

    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            if (strlen(result) + strlen(buffer) >= BUFFER_SIZE - 1) {
                break; // Prevent buffer overflow
            }
            strcat(result, buffer);
        }
    }

    pclose(pipe);
    return result;
}

// Function to count lines in a string
int count_lines(const char *str) {
    int count = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n') {
            count++;
        }
    }
    return count;
}

// Function to check if a command exists
bool command_exists(const char *cmd) {
    char command[256];
    snprintf(command, sizeof(command), "command -v %s &> /dev/null", cmd);
    return system(command) == 0;
}

// Function to check if a file exists
bool file_exists(const char *path) {
    struct stat buffer;
    return stat(path, &buffer) == 0;
}

// Function to install zenity
bool install_zenity() {
    printf("Zenity is required but not installed. Would you like to install it? (y/n): ");
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return false;
    }

    if (response[0] != 'y' && response[0] != 'Y') {
        printf("Zenity installation declined. The updater will not function properly.\n");
        return false;
    }

    // Try to install zenity using the system package manager
    if (file_exists("/usr/bin/dnf")) {
        return system("sudo dnf install -y zenity") == 0;
    }
    if (file_exists("/usr/bin/apt")) {
        return system("sudo apt-get install -y zenity") == 0;
    }
    if (file_exists("/usr/bin/pacman")) {
        return system("sudo pacman -S --noconfirm zenity") == 0;
    }
    printf("Could not determine package manager to install zenity.\n");
    return false;
}

// Function to check if systemd is available
bool systemd_available() {
    return file_exists("/run/systemd/system") || file_exists("/var/run/systemd/system");
}

// Function to set up systemd timer
bool setup_systemd_timer() {
    char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Unable to get HOME directory\n");
        return false;
    }

    char systemd_dir[512];
    snprintf(systemd_dir, sizeof(systemd_dir), "%s/.config/systemd/user", home);

    // Create a timer file
    char timer_file[600];
    if (strlen(systemd_dir) + strlen("/package-updater.timer") + 1 > sizeof(timer_file)) {
        fprintf(stderr, "Path too long!\n");
        return false;
    }
    snprintf(timer_file, sizeof(timer_file), "%s/package-updater.timer", systemd_dir);

    FILE *file = fopen(timer_file, "w");
    if (!file) {
        fprintf(stderr, "Unable to create systemd timer file\n");
        return false;
    }

    fprintf(file, "[Unit]\n");
    fprintf(file, "Description=Package Update Checker Timer\n\n");
    fprintf(file, "[Timer]\n");
    fprintf(file, "OnBootSec=5min\n");
    fprintf(file, "OnUnitActiveSec=1h\n");
    fprintf(file, "Unit=package-updater.service\n\n");
    fprintf(file, "[Install]\n");
    fprintf(file, "WantedBy=timers.target\n");

    fclose(file);

    // Enable and start the timer
    char enable_cmd[600];
    if (strlen(systemd_dir) + strlen("/systemctl --user enable package-updater.timer") + 1 > sizeof(enable_cmd)) {
        fprintf(stderr, "Path too long!\n");
        return false;
    }
    snprintf(enable_cmd, sizeof(enable_cmd), "systemctl --user enable package-updater.timer");
    system(enable_cmd);

    char start_cmd[600];
    if (strlen(systemd_dir) + strlen("/systemctl --user start package-updater.timer") + 1 > sizeof(start_cmd)) {
        fprintf(stderr, "Path too long!\n");
        return false;
    }
    snprintf(start_cmd, sizeof(start_cmd), "systemctl --user start package-updater.timer");
    system(start_cmd);

    return true;
}

// Function to check and setup systemd service
bool setup_systemd_service() {
    if (!systemd_available()) {
        printf("Systemd is not available on this system. Using autostart instead.\n");
        setup_autostart();
        return false;
    }

    char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Unable to get HOME directory\n");
        return false;
    }

    char systemd_dir[PATH_MAX_SIZE];
    char service_file[PATH_MAX_SIZE];
    char mkdir_cmd[CMD_MAX_SIZE];

    if (strlen(home) + strlen("/.config/systemd/user") + 1 > sizeof(systemd_dir)) {
        fprintf(stderr, "Path too long!\n");
        return false;
    }
    snprintf(systemd_dir, sizeof(systemd_dir), "%s/.config/systemd/user", home);

    if (strlen(systemd_dir) + strlen("/package-updater.service") + 1 > sizeof(service_file)) {
        fprintf(stderr, "Path too long!\n");
        return false;
    }
    snprintf(service_file, sizeof(service_file), "%s/package-updater.service", systemd_dir);

    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", systemd_dir);
    system(mkdir_cmd);

    FILE *file = fopen(service_file, "w");
    if (!file) {
        fprintf(stderr, "Unable to create systemd service file\n");
        return false;
    }

    fprintf(file, "[Unit]\n");
    fprintf(file, "Description=Package Update Checker\n");
    fprintf(file, "After=network.target\n\n");
    fprintf(file, "[Service]\n");
    fprintf(file, "Type=oneshot\n");
    fprintf(file, "ExecStart=/opt/package-updater/updater --notify-only\n");
    fprintf(file, "Environment=DISPLAY=:0\n");
    fprintf(file, "Environment=XAUTHORITY=/home/%s/.Xauthority\n", getenv("USER"));
    fprintf(file, "User=%s\n", getenv("USER"));
    fprintf(file, "Group=%s\n", getenv("USER"));

    fclose(file);

    // Set up the timer
    if (!setup_systemd_timer()) {
        printf("Warning: Failed to setup systemd timer. Updates will not be checked periodically.\n");
    }

    return true;
}

// Function to check updates for each package manager
// Now returns a string with package names separated by newlines, and a summary for the dialog text
char *get_update_packages(char *summary, const size_t summary_size) {
    char *packages = malloc(BUFFER_SIZE);
    if (!packages) {
        summary[0] = '\0';
        return nullptr;
    }
    packages[0] = '\0';
    summary[0] = '\0';

    // Homebrew
    if (command_exists("brew")) {
        char *brew_updates = execute_command("brew outdated 2>/dev/null");
        if (brew_updates && strlen(brew_updates) > 0) {
            const int brew_count = count_lines(brew_updates);
            snprintf(summary + strlen(summary), summary_size - strlen(summary), "Homebrew: %d packages\n", brew_count);
            // Add each package as a row
            const char *line = strtok(brew_updates, "\n");
            while (line) {
                strncat(packages, line, BUFFER_SIZE - strlen(packages) - 2);
                strncat(packages, "\n", BUFFER_SIZE - strlen(packages) - 2);
                line = strtok(nullptr, "\n");
            }
        }
        free(brew_updates);
    }
    // DNF
    if (command_exists("dnf")) {
        char *dnf_updates = execute_command("sudo dnf check-update 2>/dev/null | awk 'NR>2 {print $1}'");
        if (dnf_updates && strlen(dnf_updates) > 0) {
            const int dnf_count = count_lines(dnf_updates);
            snprintf(summary + strlen(summary), summary_size - strlen(summary), "DNF: %d system packages\n", dnf_count);
            const char *line = strtok(dnf_updates, "\n");
            while (line) {
                strncat(packages, line, BUFFER_SIZE - strlen(packages) - 2);
                strncat(packages, "\n", BUFFER_SIZE - strlen(packages) - 2);
                line = strtok(nullptr, "\n");
            }
        }
        free(dnf_updates);
    }
    // Flatpak
    if (command_exists("flatpak")) {
        char *flatpak_updates = execute_command("flatpak update --dry-run 2>&1 | grep '^  ' | awk '{print $2}'");
        if (flatpak_updates && strlen(flatpak_updates) > 0) {
            const int flatpak_count = count_lines(flatpak_updates);
            snprintf(summary + strlen(summary), summary_size - strlen(summary), "Flatpak: %d packages\n",
                     flatpak_count);
            const char *line = strtok(flatpak_updates, "\n");
            while (line) {
                strncat(packages, line, BUFFER_SIZE - strlen(packages) - 2);
                strncat(packages, "\n", BUFFER_SIZE - strlen(packages) - 2);
                line = strtok(nullptr, "\n");
            }
        }
        free(flatpak_updates);
    }
    // Snap
    if (command_exists("snap")) {
        char *snap_updates = execute_command("snap refresh --list 2>/dev/null | awk 'NR>1 {print $1}'");
        if (snap_updates && strlen(snap_updates) > 0) {
            const int snap_count = count_lines(snap_updates);
            snprintf(summary + strlen(summary), summary_size - strlen(summary), "Snap: %d packages\n", snap_count);
            const char *line = strtok(snap_updates, "\n");
            while (line) {
                strncat(packages, line, BUFFER_SIZE - strlen(packages) - 2);
                strncat(packages, "\n", BUFFER_SIZE - strlen(packages) - 2);
                line = strtok(nullptr, "\n");
            }
        }
        free(snap_updates);
    }
    if (strlen(packages) == 0) {
        free(packages);
        summary[0] = '\0';
        return nullptr;
    }
    return packages;
}

// Function to create a desktop file
void create_desktop_file() {
    char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Unable to get HOME directory\n");
        return;
    }

    char desktop_dir[PATH_MAX_SIZE];
    char desktop_file[PATH_MAX_SIZE];
    char chmod_cmd[PATH_MAX_SIZE + 32];

    if (strlen(home) + strlen("/.local/share/applications") + 1 > sizeof(desktop_dir)) {
        fprintf(stderr, "Path too long!\n");
        return;
    }
    snprintf(desktop_dir, sizeof(desktop_dir), "%s/.local/share/applications", home);

    if (strlen(desktop_dir) + strlen("/package-updater.desktop") + 1 > sizeof(desktop_file)) {
        fprintf(stderr, "Path too long!\n");
        return;
    }
    snprintf(desktop_file, sizeof(desktop_file), "%s/package-updater.desktop", desktop_dir);

    FILE *file = fopen(desktop_file, "w");
    if (!file) {
        fprintf(stderr, "Unable to create desktop file\n");
        return;
    }

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Version=1.0\n");
    fprintf(file, "Name=Package Updater\n");
    fprintf(file, "Comment=Check for package updates\n");
    fprintf(file, "Exec=/opt/package-updater/updater\n");
    fprintf(file, "Icon=/opt/package-updater/icon.png\n");
    fprintf(file, "Terminal=false\n");
    fprintf(file, "Type=Application\n");
    fprintf(file, "Categories=System;Utility;\n");
    fprintf(file, "Keywords=update;package;system;\n");

    fclose(file);

    snprintf(chmod_cmd, sizeof(chmod_cmd), "chmod +x %s", desktop_file);
    system(chmod_cmd);

    printf("Desktop file created at: %s\n", desktop_file);
}

// Function to set up autostart
void setup_autostart() {
    char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "Unable to get HOME directory\n");
        return;
    }

    char autostart_dir[PATH_MAX_SIZE];
    char autostart_file[PATH_MAX_SIZE];

    if (strlen(home) + strlen("/.config/autostart") + 1 > sizeof(autostart_dir)) {
        fprintf(stderr, "Path too long!\n");
        return;
    }
    snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config/autostart", home);

    if (strlen(autostart_dir) + strlen("/package-updater.desktop") + 1 > sizeof(autostart_file)) {
        fprintf(stderr, "Path too long!\n");
        return;
    }
    snprintf(autostart_file, sizeof(autostart_file), "%s/package-updater.desktop", autostart_dir);

    FILE *file = fopen(autostart_file, "w");
    if (!file) {
        fprintf(stderr, "Unable to create autostart file\n");
        return;
    }

    fprintf(file, "[Desktop Entry]\n");
    fprintf(file, "Version=1.0\n");
    fprintf(file, "Name=Package Updater\n");
    fprintf(file, "Comment=Check for package updates\n");
    fprintf(file, "Exec=/opt/package-updater/updater --notify-only\n");
    fprintf(file, "Icon=/opt/package-updater/icon.png\n");
    fprintf(file, "Terminal=false\n");
    fprintf(file, "Type=Application\n");
    fprintf(file, "Categories=System;Utility;\n");
    fprintf(file, "Keywords=update;package;system;\n");
    fprintf(file, "X-GNOME-Autostart-enabled=true\n");

    fclose(file);

    printf("Autostart entry created at: %s\n", autostart_file);
}

// Function to display a notification using zenity
void notify_mode() {
    char *updates = get_update_packages(nullptr, 0);
    if (updates) {
        system(
            "zenity --notification --text=\"Package updates available. Click to manage updates.\" --window-icon=system-software-update"
        );

        // Check if user wants to open the update dialog
        const int result = system("zenity --question --text=\"Open update manager?\"");
        if (result == 0) {
            // User clicked Yes
            run_update_dialog();
        }

        free(updates);
    }
}

// Main update dialog function
void run_update_dialog() {
    while (1) {
        char summary[1024];
        char *packages = get_update_packages(summary, sizeof(summary));
        if (!packages) {
            system(
                "zenity --info --title=\"Package Updates\" --text=\"No updates available.\" --width=400 --height=200");
            return;
        }
        char zenity_cmd[BUFFER_SIZE * 2];
        strcpy(zenity_cmd, "zenity --list --title=\"Package Updates Available\" ");
        strcat(zenity_cmd, "--text=\"The following packages have updates available:\\n");
        strcat(zenity_cmd, summary);
        strcat(zenity_cmd, "\" --column=\"Action or Package\" ");
        strcat(zenity_cmd, "\"Brew, Snap & Flatpak\" ");
        strcat(zenity_cmd, "\"Open Software Manager\" ");
        strcat(zenity_cmd, "\"Check for updates again\" ");
        // Add each package as a row
        const char *pkg = strtok(packages, "\n");
        while (pkg) {
            strcat(zenity_cmd, "\"");
            strcat(zenity_cmd, pkg);
            strcat(zenity_cmd, "\" ");
            pkg = strtok(nullptr, "\n");
        }
        strcat(zenity_cmd, "--ok-label=\"Select\" --cancel-label=\"Exit\" --width=400 --height=300");
        free(packages);

        FILE *pipe = popen(zenity_cmd, "r");
        if (!pipe) {
            fprintf(stderr, "Failed to run zenity command\n");
            return;
        }
        char buffer[256];
        char result[256] = {0};
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            strcpy(result, buffer);
            result[strcspn(result, "\n")] = 0;
        }
        const int status = pclose(pipe);
        if (status != 0) {
            return;
        }
        if (strcmp(result, "Brew, Snap & Flatpak") == 0) {
            system("ptyxis --new-window bash -c '"
                "echo \"Updating Homebrew...\";"
                "brew update && brew upgrade;"
                "echo \"Updating Snap packages...\";"
                "sudo snap refresh;"
                "echo \"Updating Flatpak packages...\";"
                "flatpak update;"
                "echo \"Updates complete. Press Enter to close.\";"
                "read;"
                "'");
        } else if (strcmp(result, "Open Software Manager") == 0) {
            if (command_exists("gnome-software")) {
                system("gnome-software --mode=updates");
            } else if (command_exists("plasma-discover")) {
                system("plasma-discover --mode=updates");
            } else {
                system("zenity --error --text=\"No software update manager found.\"");
            }
        } else if (strcmp(result, "Check for updates again") == 0) {
        }
    }
}

// Display help message
void show_help(const char *program_name) {
    printf("Usage: %s [OPTION]\n", program_name);
    printf("Package Update Notification Program\n");
    printf("\n");
    printf("Options:\n");
    printf("  --install-desktop  Create desktop file in applications menu\n");
    printf("  --setup-autostart  Configure program to run at startup\n");
    printf("  --notify-only      Show notification if updates available\n");
    printf("  --help             Display this help\n");
    printf("\n");
    printf("With no options, runs the interactive update dialog.\n");
}

// Function to initialize and check requirements
bool initialize_updater() {
    printf("Initializing Package Update Checker...\n");

    // Check for zenity
    if (!command_exists("zenity")) {
        printf("Zenity is required for the GUI interface.\n");
        if (!install_zenity()) {
            printf("Please install zenity manually and try again.\n");
            return false;
        }
    }

    // Check for package managers and store their availability
    const bool has_brew = command_exists("brew");
    const bool has_dnf = command_exists("dnf");
    const bool has_flatpak = command_exists("flatpak");
    const bool has_snap = command_exists("snap");

    printf("\nDetected package managers:\n");
    if (has_brew) printf("✓ Homebrew\n");
    if (has_dnf) printf("✓ DNF\n");
    if (has_flatpak) printf("✓ Flatpak\n");
    if (has_snap) printf("✓ Snap\n");

    if (!has_brew && !has_dnf && !has_flatpak && !has_snap) {
        printf("\n❌ No supported package managers found.\n");
        printf("The updater will not be able to check for updates.\n");
        return false;
    }

    printf("\nSetting up automatic update checks...\n");
    // Setup systemd service or fallback to autostart
    if (!setup_systemd_service()) {
        printf("Warning: Failed to setup automatic update checks.\n");
        printf("You can still run the updater manually.\n");
    } else {
        printf("✓ Automatic update checks configured successfully.\n");
    }

    printf("\nInitialization complete!\n");
    return true;
}

int main(const int argc, char *argv[]) {
    // Initialize the updater
    if (!initialize_updater()) {
        fprintf(stderr, "Initialization failed. Please check the requirements and try again.\n");
        return 1;
    }

    // Set environment variables
    const uid_t user_id = getuid();
    char xdg_runtime_dir[128];
    snprintf(xdg_runtime_dir, sizeof(xdg_runtime_dir), "/run/user/%d", user_id);
    setenv("XDG_RUNTIME_DIR", xdg_runtime_dir, 1);
    setenv("DISPLAY", ":0", 1);

    // Handle command line options
    if (argc > 1) {
        if (strcmp(argv[1], "--install-desktop") == 0) {
            create_desktop_file();
            return 0;
        }
        if (strcmp(argv[1], "--setup-autostart") == 0) {
            setup_autostart();
            return 0;
        }
        if (strcmp(argv[1], "--notify-only") == 0) {
            notify_mode();
            return 0;
        }
        if (strcmp(argv[1], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        }
        fprintf(stderr, "Unknown option: %s\n", argv[1]);
        show_help(argv[0]);
        return 1;
    }
    // Default behavior - run the update dialog
    run_update_dialog();

    return 0;
}
