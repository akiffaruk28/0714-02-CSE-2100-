#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "backup_settings.h"

void add_file_to_list(const char *path);
void add_folder_to_list(const char *path, int recursive);
void on_select_files_folders(GtkWidget *widget, gpointer data);
void on_select_folder_only(GtkWidget *widget, gpointer data);
void on_select_backup_folder(GtkWidget *widget, gpointer data);
void on_remove_items(GtkWidget *widget, gpointer data);
void on_clear_all(GtkWidget *widget, gpointer data);

#endif
