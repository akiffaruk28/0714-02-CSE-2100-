/**
 * @file   fileops.c
 * @brief  Backup queue management — add files and folders to the GTK list store.
 *
 * Single responsibility: populate the GtkListStore (items_list) with
 * files selected by the user. Does not perform any file copying.
 *
 * Dependencies: fileops.h, settings.h, window.h, utils.h
 * Platform:     Windows (MSYS2/MinGW-w64) with GTK+ 3.0
 */

#include "fileops.h"
#include "settings.h"
#include "window.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <gtk/gtk.h>

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Add a single file to the GTK backup queue list.
 *
 * Skips if: stat() fails, file is hidden and include_hidden is off,
 * or the path is already present in the queue (duplicate check).
 *
 * @param path  Absolute path to the file.
 */
void add_file_to_list(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) { return; }

    /* Apply hidden-file filter */
    if (!settings.include_hidden) {
        const char *name = strrchr(path, '\\');
        name = name ? name + 1 : path;
        if (name[0] == '.') { return; }
    }

    /* Duplicate check */
    GtkTreeIter iter;
    gboolean exists = FALSE;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_list), &iter)) {
        do {
            char *existing;
            gtk_tree_model_get(GTK_TREE_MODEL(items_list), &iter,
                               0, &existing, -1);
            if (strcmp(existing, path) == 0) {
                exists = TRUE;
                g_free(existing);
                break;
            }
            g_free(existing);   /* always free GTK-allocated strings */
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &iter));
    }

    if (!exists) {
        /* Format file size for display */
        char size_str[32];
        if (st.st_size < 1024)
            snprintf(size_str, sizeof(size_str), "%ld B",   (long)st.st_size);
        else if (st.st_size < 1024 * 1024)
            snprintf(size_str, sizeof(size_str), "%.1f KB", st.st_size / 1024.0);
        else if (st.st_size < 1024 * 1024 * 1024)
            snprintf(size_str, sizeof(size_str), "%.1f MB", st.st_size / (1024.0 * 1024.0));
        else
            snprintf(size_str, sizeof(size_str), "%.1f GB", st.st_size / (1024.0 * 1024.0 * 1024.0));

        /* Format modification time for display */
        char time_str[64];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);

        gtk_list_store_append(items_list, &iter);
        gtk_list_store_set(items_list, &iter,
                           0, path,
                           1, size_str,
                           2, time_str,
                           -1);
    }
}

/**
 * @brief  Recursively (or not) add all files in a folder to the queue.
 *
 * @param path       Absolute path to the folder.
 * @param recursive  Non-zero to recurse into subdirectories.
 */
void add_folder_to_list(const char *path, int recursive) {
    DIR *dir = opendir(path);
    if (!dir) { return; }

    struct dirent *entry;
    char full_path[BACKUP_MAX_PATH_LENGTH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s\\%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) { continue; }

        if (S_ISDIR(st.st_mode)) {
            if (recursive) {
                add_folder_to_list(full_path, recursive);
            }
        } else {
            add_file_to_list(full_path);
        }
    }
    closedir(dir);
}
