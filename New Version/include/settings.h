/**
 * @file   settings.h
 * @brief  BackupSettings struct definition, constants, and config prototypes.
 *
 * Dependencies: standard C library only (no GTK, no project headers)
 * Platform:     Windows (MSYS2/MinGW-w64) with GTK+ 3.0
 */

#ifndef SETTINGS_H
#define SETTINGS_H

/* ── Platform compatibility ───────────────────────────────────────── */
#ifdef _WIN32
    #include <direct.h>
    #undef  stat
    #define stat     _stat
    #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
    #define mkdir    _mkdir
#endif

/* ── Constants ────────────────────────────────────────────────────── */
#define BACKUP_CONFIG_FILENAME   "backup_config.txt"
#define BACKUP_LOG_FILENAME      "backup_log.txt"
#define BACKUP_MAX_PATH_LENGTH   512
#define BACKUP_MAX_QUEUE_ITEMS   1000
#define BACKUP_COPY_BUFFER_SIZE  8192
#define BACKUP_DIR_PREFIX        "Backup_"
#define BACKUP_TIMESTAMP_FORMAT  "%04d%02d%02d_%02d%02d"

/* ── Data types ───────────────────────────────────────────────────── */

/**
 * @brief  All persistent configuration for the backup utility.
 *
 * Stored in backup_config.txt as key=value pairs.
 * Loaded at startup by Settings_Load(), saved by Settings_Save().
 */
typedef struct {
    char backup_destination[BACKUP_MAX_PATH_LENGTH];
    int  auto_backup;
    int  backup_interval;
    int  max_copies;
    int  backup_subfolders;
    int  include_hidden;
    int  show_notifications;
} BackupSettings;

/* ── Global instance ──────────────────────────────────────────────── */
extern BackupSettings settings;

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Load settings from backup_config.txt.
 *         Creates the file with defaults if it does not exist.
 */
void load_settings(void);

/**
 * @brief  Write current settings to backup_config.txt.
 */
void save_settings(void);

#endif /* SETTINGS_H */
