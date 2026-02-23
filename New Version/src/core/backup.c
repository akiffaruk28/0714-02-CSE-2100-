/**
 * @file   backup.c
 * @brief  Backup execution engine — file copying, session logging, auto-timer.
 *
 * Single responsibility: copy queued files to a timestamped directory
 * and write a per-session backup_log.txt.
 *
 * Dependencies: backup.h, settings.h, window.h, utils.h
 * Platform:     Windows (MSYS2/MinGW-w64) with GTK+ 3.0
 */

#include "backup.h"
#include "settings.h"
#include "window.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <gtk/gtk.h>

/* ── Internal helpers ─────────────────────────────────────────────── */

/**
 * @brief  GLib timer callback to reset the progress bar to 0 after backup.
 *
 * FIX: The original code cast gtk_progress_bar_set_fraction directly as a
 * GSourceFunc, which is undefined behaviour — the function signatures are
 * incompatible (double vs gpointer). This wrapper is the correct approach.
 *
 * @param data  Unused. Pass NULL.
 * @return      G_SOURCE_REMOVE (one-shot).
 */
static gboolean reset_progress_bar(gpointer data) {
    (void)data;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
    return G_SOURCE_REMOVE;
}

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Execute a full backup of all queued files.
 *
 * Creates a timestamped directory under settings.backup_destination,
 * copies each file from the GTK list store, and writes backup_log.txt.
 * Updates the GTK progress bar throughout.
 * Sets backup_running flag to prevent concurrent runs.
 *
 * @param data  Unused (GLib idle callback parameter). Pass NULL.
 * @return      G_SOURCE_REMOVE always (one-shot idle callback).
 */
gboolean perform_backup(gpointer data) {
    if (backup_running) { return G_SOURCE_REMOVE; }

    GtkTreeIter iter;
    gboolean has_items = gtk_tree_model_get_iter_first(
                             GTK_TREE_MODEL(items_list), &iter);
    if (!has_items) {
        update_status("No items to backup", 0.0);
        return G_SOURCE_REMOVE;
    }

    backup_running = TRUE;

    /* Count total items for progress calculation */
    int total = 0;
    GtkTreeIter count_iter = iter;
    do { total++; }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &count_iter));

    int current = 0;
    int success = 0;

    /* Build timestamped backup directory path */
    char backup_dir[BACKUP_MAX_PATH_LENGTH];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    snprintf(backup_dir, sizeof(backup_dir),
             "%s\\" BACKUP_DIR_PREFIX BACKUP_TIMESTAMP_FORMAT,
             settings.backup_destination,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min);
    mkdir(backup_dir);

    /* Open session log */
    char log_path[BACKUP_MAX_PATH_LENGTH];
    snprintf(log_path, sizeof(log_path), "%s\\backup_log.txt", backup_dir);
    FILE *log = fopen(log_path, "w");
    if (log) {
        fprintf(log, "Backup started: %s", ctime(&now));
    }

    /* Copy each file in the queue */
    do {
        char *source_path;
        gtk_tree_model_get(GTK_TREE_MODEL(items_list), &iter,
                           0, &source_path, -1);

        current++;
        const char *filename = strrchr(source_path, '\\');
        filename = filename ? filename + 1 : source_path;

        /* Update progress bar */
        char status_msg[200];
        snprintf(status_msg, sizeof(status_msg),
                 "Backing up (%d/%d): %s", current, total, filename);
        update_status(status_msg, (double)current / total);

        /* Copy the file */
        char dest_path[BACKUP_MAX_PATH_LENGTH];
        snprintf(dest_path, sizeof(dest_path), "%s\\%s", backup_dir, filename);

        FILE *src = fopen(source_path, "rb");
        if (src) {
            FILE *dst = fopen(dest_path, "wb");
            if (dst) {
                char buffer[BACKUP_COPY_BUFFER_SIZE];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, bytes, dst);
                }
                fclose(dst);
                success++;
                if (log) {
                    fprintf(log, "OK  %s -> %s\n", source_path, dest_path);
                }
            } else {
                if (log) {
                    fprintf(log, "FAIL (cannot create dest): %s\n", dest_path);
                }
            }
            fclose(src);
        } else {
            if (log) {
                fprintf(log, "FAIL (cannot open source): %s\n", source_path);
            }
        }

        g_free(source_path);   /* free GTK-allocated string */

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &iter));

    if (log) {
        fprintf(log, "\nBackup completed: %d/%d files successful\n",
                success, total);
        fclose(log);
    }

    char final_msg[200];
    snprintf(final_msg, sizeof(final_msg),
             "Backup completed! %d/%d files backed up successfully",
             success, total);
    update_status(final_msg, 1.0);
    show_notification_msg("Backup Complete", final_msg);

    backup_running = FALSE;
    /* Reset progress bar after a short delay using a proper callback wrapper */
    g_timeout_add_seconds(3, reset_progress_bar, NULL);

    return G_SOURCE_REMOVE;
}

/**
 * @brief  GLib timer callback — triggers perform_backup if auto-backup is on.
 *
 * @param data  Unused. Pass NULL.
 * @return      G_SOURCE_CONTINUE always (recurring timer).
 */
gboolean auto_backup_timer(gpointer data) {
    if (settings.auto_backup && !backup_running) {
        g_idle_add(perform_backup, NULL);
    }
    return G_SOURCE_CONTINUE;
}
