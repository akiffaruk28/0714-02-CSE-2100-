#ifndef BACKUP_SETTINGS_H
#define BACKUP_SETTINGS_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <direct.h>
#include <dirent.h>
#include <unistd.h>

#ifdef _WIN32
    #define stat _stat
    #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
    #define mkdir _mkdir
#endif

#define CONFIG_FILE "backup_config.txt"
#define LOG_FILE "backup_log.txt"
#define MAX_PATH 512
#define MAX_ITEMS 1000

typedef struct {
    char backup_destination[MAX_PATH];
    int auto_backup;
    int backup_interval;
    int max_copies;
    int backup_subfolders;
    int include_hidden;
    int show_notifications;
} BackupSettings;

extern BackupSettings settings;
extern GtkWidget *window;
extern GtkWidget *progress_bar;
extern GtkWidget *status_label;
extern GtkListStore *items_list;
extern GtkWidget *treeview;
extern gboolean backup_running;
extern guint timer_id;
extern GtkWidget *dest_entry_global;

#endif
