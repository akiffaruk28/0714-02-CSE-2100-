#include "../headers/file_operations.h"
#include "../headers/notifications.h"
#include "../headers/progress_display.h"

void add_file_to_list(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return;

    if (!settings.include_hidden) {
        const char *name = strrchr(path, '\\');
        name = name ? name + 1 : path;
        if (name[0] == '.') return;
    }

    GtkTreeIter iter;
    gboolean exists = FALSE;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_list), &iter)) {
        do {
            char *existing;
            gtk_tree_model_get(GTK_TREE_MODEL(items_list), &iter, 0, &existing, -1);
            if (strcmp(existing, path) == 0) {
                exists = TRUE;
                g_free(existing);
                break;
            }
            g_free(existing);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &iter));
    }

    if (!exists) {
        char size_str[32];
        if (st.st_size < 1024)
            sprintf(size_str, "%ld B", st.st_size);
        else if (st.st_size < 1024*1024)
            sprintf(size_str, "%.1f KB", st.st_size/1024.0);
        else if (st.st_size < 1024*1024*1024)
            sprintf(size_str, "%.1f MB", st.st_size/(1024.0*1024.0));
        else
            sprintf(size_str, "%.1f GB", st.st_size/(1024.0*1024.0*1024.0));

        char time_str[64];
        struct tm *tm = localtime(&st.st_mtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);

        gtk_list_store_append(items_list, &iter);
        gtk_list_store_set(items_list, &iter,
                          0, path,
                          1, size_str,
                          2, time_str,
                          -1);
    }
}

void add_folder_to_list(const char *path, int recursive) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char full_path[MAX_PATH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s\\%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            if (recursive)
                add_folder_to_list(full_path, recursive);
        } else {
            add_file_to_list(full_path);
        }
    }
    closedir(dir);
}

void on_select_files_folders(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Files and Folders",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Add", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkWidget *recursive_check = gtk_check_button_new_with_label("Include subfolders recursively");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), recursive_check);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recursive_check));
        GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

        for (GSList *iter = files; iter; iter = iter->next) {
            char *path = (char*)iter->data;
            struct stat st;

            if (stat(path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    add_folder_to_list(path, recursive && settings.backup_subfolders);
                } else {
                    add_file_to_list(path);
                }
            }
            g_free(path);
        }
        g_slist_free(files);

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(items_list), NULL);
        char msg[100];
        sprintf(msg, "%d items ready for backup", count);
        update_status(msg, 0.0);
    }

    gtk_widget_destroy(dialog);
}

void on_select_folder_only(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Folder to Backup",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkWidget *recursive_check = gtk_check_button_new_with_label("Include subfolders");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), recursive_check);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recursive_check));
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (folder) {
            add_folder_to_list(folder, recursive && settings.backup_subfolders);
            g_free(folder);
        }

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(items_list), NULL);
        char msg[100];
        sprintf(msg, "%d items ready for backup", count);
        update_status(msg, 0.0);
    }

    gtk_widget_destroy(dialog);
}

void on_select_backup_folder(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Backup Destination Folder",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        NULL);

    if (strlen(settings.backup_destination) > 0) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), settings.backup_destination);
    }

    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (folder) {
            strcpy(settings.backup_destination, folder);

            if (dest_entry_global) {
                gtk_entry_set_text(GTK_ENTRY(dest_entry_global), folder);
            }

            mkdir(folder);

            char msg[200];
            sprintf(msg, "Backup folder set to: %s", folder);
            update_status(msg, 0.0);

            g_free(folder);
        }
    }

    gtk_widget_destroy(dialog);
}

void on_remove_items(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(items_list), NULL);
        char msg[100];
        sprintf(msg, "%d items remaining", count);
        update_status(msg, 0.0);
    } else {
        show_notification_msg("Notice", "Please select an item to remove");
    }
}

void on_clear_all(GtkWidget *widget, gpointer data) {
    gtk_list_store_clear(items_list);
    update_status("List cleared", 0.0);
}
