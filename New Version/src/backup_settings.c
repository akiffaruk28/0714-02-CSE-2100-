#include "../headers/backup_settings.h"

BackupSettings settings = {
    .backup_destination = "C:\\Backups",
    .auto_backup = 1,
    .backup_interval = 300,
    .max_copies = 10,
    .backup_subfolders = 1,
    .include_hidden = 0,
    .show_notifications = 1
};

GtkWidget *window;
GtkWidget *progress_bar;
GtkWidget *status_label;
GtkListStore *items_list;
GtkWidget *treeview;
gboolean backup_running = FALSE;
guint timer_id = 0;
GtkWidget *dest_entry_global;
