/**
 * @file   window.h
 * @brief  GTK global widget extern declarations and UI_CreateMainWindow prototype.
 *
 * All GTK widget globals are DEFINED in src/rendering/window.c
 * and declared here as extern so other modules can reference them.
 *
 * Dependencies: gtk/gtk.h
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>

/* ── Global GTK widget pointers (defined in window.c) ────────────── */
extern GtkWidget    *window;
extern GtkWidget    *progress_bar;
extern GtkWidget    *status_label;
extern GtkListStore *items_list;
extern GtkWidget    *treeview;
extern GtkWidget    *dest_entry_global;
extern gboolean      backup_running;
extern guint         timer_id;

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Build and display the main application window.
 *         Initializes all global GTK widget pointers.
 *         Must be called after gtk_init() and load_settings().
 */
void create_main_window(void);

/* ── GTK Signal Callbacks ─────────────────────────────────────────── */
void on_select_files_folders(GtkWidget *widget, gpointer data);
void on_select_backup_folder(GtkWidget *widget, gpointer data);
void on_select_folder_only(GtkWidget *widget, gpointer data);
void on_remove_items(GtkWidget *widget, gpointer data);
void on_clear_all(GtkWidget *widget, gpointer data);
void on_backup_now(GtkWidget *widget, gpointer data);
void on_settings(GtkWidget *widget, gpointer data);
void on_view_log(GtkWidget *widget, gpointer data);

#endif /* WINDOW_H */
