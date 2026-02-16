#include "../headers/backup_core.h"
#include "../headers/notifications.h"
#include "../headers/progress_display.h"

gboolean perform_backup(gpointer data) {
    if (backup_running) return G_SOURCE_REMOVE;

    GtkTreeIter iter;
    gboolean has_items = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_list), &iter);

    if (!has_items) {
        update_status("No items to backup", 0.0);
        return G_SOURCE_REMOVE;
    }

    backup_running = TRUE;

    int total = 0;
    GtkTreeIter count_iter = iter;
    do { total++; } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &count_iter));

    int current = 0;
    int success = 0;

    char backup_dir[MAX_PATH];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    sprintf(backup_dir, "%s\\Backup_%04d%02d%02d_%02d%02d",
            settings.backup_destination,
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
            tm->tm_hour, tm->tm_min);
    mkdir(backup_dir);

    char log_file[MAX_PATH];
    sprintf(log_file, "%s\\backup_log.txt", backup_dir);
    FILE *log = fopen(log_file, "w");
    if (log) {
        fprintf(log, "Backup started: %s", ctime(&now));
    }

    do {
        char *source_path;
        gtk_tree_model_get(GTK_TREE_MODEL(items_list), &iter, 0, &source_path, -1);

        current++;
        const char *filename = strrchr(source_path, '\\');
        filename = filename ? filename + 1 : source_path;

        char status[200];
        sprintf(status, "Backing up (%d/%d): %s", current, total, filename);
        update_status(status, (double)current/total);

        char dest_path[MAX_PATH];
        sprintf(dest_path, "%s\\%s", backup_dir, filename);

        FILE *src = fopen(source_path, "rb");
        if (src) {
            FILE *dst = fopen(dest_path, "wb");
            if (dst) {
                char buffer[8192];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, bytes, dst);
                }
                fclose(dst);
                success++;

                if (log) {
                    fprintf(log, "✓ %s -> %s\n", source_path, dest_path);
                }
            }
            fclose(src);
        }

        g_free(source_path);

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &iter));

    if (log) {
        fprintf(log, "\nBackup completed: %d/%d files successful\n", success, total);
        fclose(log);
    }

    char final_msg[200];
    sprintf(final_msg, "Backup completed! %d/%d files backed up successfully", success, total);
    update_status(final_msg, 1.0);

    show_notification_msg("Backup Complete", final_msg);

    backup_running = FALSE;

    g_timeout_add_seconds(3, (GSourceFunc)gtk_progress_bar_set_fraction, GINT_TO_POINTER(0));

    return G_SOURCE_REMOVE;
}

gboolean auto_backup_timer(gpointer data) {
    if (settings.auto_backup && !backup_running) {
        g_idle_add(perform_backup, NULL);
    }
    return G_SOURCE_CONTINUE;
}

void on_backup_now(GtkWidget *widget, gpointer data) {
    if (backup_running) {
        show_notification_msg("Busy", "Backup already in progress");
        return;
    }
    g_idle_add(perform_backup, NULL);
}
