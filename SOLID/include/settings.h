#ifndef SETTINGS_H
#define SETTINGS_H

// ============================================================
//  settings.h
//  SRP  – Owns only the BackupSettings data structure and
//          the load/save contract.
//  OCP  – Add new fields here without touching backup logic.
//  DIP  – Other modules depend on this abstraction, not on
//          a concrete file-format parser.
// ============================================================

#define CONFIG_FILE   "backup_config.txt"
#define MAX_PATH_LEN  512

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char backup_destination[MAX_PATH_LEN];
    int  auto_backup;        /* bool: 1 = enabled */
    int  backup_interval;    /* seconds           */
    int  max_copies;
    int  backup_subfolders;  /* bool              */
    int  include_hidden;     /* bool              */
    int  show_notifications; /* bool              */
} BackupSettings;

/* DIP: callers depend on these declarations, not on the .c impl */
void settings_load(BackupSettings *s);
void settings_save(const BackupSettings *s);
void settings_init_defaults(BackupSettings *s);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H */
