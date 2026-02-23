/**
 * @file   settings.c
 * @brief  Configuration persistence — reads and writes backup_config.txt.
 *
 * Single responsibility: load and save BackupSettings from/to disk.
 * No GTK dependency — can be tested without a display.
 *
 * Dependencies: settings.h, stdio.h, string.h, stdlib.h, sys/stat.h
 * Platform:     Windows (MSYS2/MinGW-w64)
 */

#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ── Global settings instance ─────────────────────────────────────── */
BackupSettings settings = {
    .backup_destination = "C:\\Backups",
    .auto_backup        = 1,
    .backup_interval    = 300,
    .max_copies         = 10,
    .backup_subfolders  = 1,
    .include_hidden     = 0,
    .show_notifications = 1
};

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Load settings from backup_config.txt.
 *         Creates the file with defaults if it does not exist.
 */
void load_settings(void) {
    FILE *file = fopen(BACKUP_CONFIG_FILENAME, "r");
    if (!file) {
        save_settings();   /* write defaults on first run */
        return;
    }

    char line[BACKUP_MAX_PATH_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        char *key   = strtok(line, "=");
        char *value = strtok(NULL, "=");
        if (!key || !value) { continue; }

        if (strcmp(key, "destination") == 0) {
            strncpy(settings.backup_destination, value,
                    BACKUP_MAX_PATH_LENGTH - 1);
            settings.backup_destination[BACKUP_MAX_PATH_LENGTH - 1] = '\0';

        } else if (strcmp(key, "auto_backup") == 0) {
            settings.auto_backup = atoi(value);

        } else if (strcmp(key, "interval") == 0) {
            settings.backup_interval = atoi(value);

        } else if (strcmp(key, "max_copies") == 0) {
            settings.max_copies = atoi(value);

        } else if (strcmp(key, "subfolders") == 0) {
            settings.backup_subfolders = atoi(value);

        } else if (strcmp(key, "hidden") == 0) {
            settings.include_hidden = atoi(value);
        }
    }
    fclose(file);

    mkdir(settings.backup_destination);
}

/**
 * @brief  Write current settings to backup_config.txt.
 */
void save_settings(void) {
    FILE *file = fopen(BACKUP_CONFIG_FILENAME, "w");
    if (!file) { return; }

    fprintf(file, "destination=%s\n", settings.backup_destination);
    fprintf(file, "auto_backup=%d\n", settings.auto_backup);
    fprintf(file, "interval=%d\n",    settings.backup_interval);
    fprintf(file, "max_copies=%d\n",  settings.max_copies);
    fprintf(file, "subfolders=%d\n",  settings.backup_subfolders);
    fprintf(file, "hidden=%d\n",      settings.include_hidden);

    fclose(file);
}
