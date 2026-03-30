#pragma once

#define CONFIG_FILE "backup_config.txt"
#define MAX_PATH_LEN 512

/**
 * BackupSettings
 * Holds all user-configurable options for the backup utility.
 */
struct BackupSettings {
    char backup_destination[MAX_PATH_LEN] = "C:\\Backups";
    bool auto_backup        = true;
    int  backup_interval    = 300;   // seconds
    int  max_copies         = 10;
    bool backup_subfolders  = true;
    bool include_hidden     = false;
    bool show_notifications = true;
};

// Global settings instance (defined in settings.cpp)
extern BackupSettings settings;

/**
 * Load settings from CONFIG_FILE.
 * Creates the file with defaults if it does not exist.
 */
void load_settings();

/**
 * Persist current settings to CONFIG_FILE.
 */
void save_settings();
