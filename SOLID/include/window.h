#pragma once
#include <gtk/gtk.h>

// ── Shared widget handles (defined in rendering/window.cpp) ──────────────────
extern GtkWidget    *window;
extern GtkWidget    *progress_bar;
extern GtkWidget    *status_label;
extern GtkListStore *items_list;
extern GtkWidget    *treeview;
extern GtkWidget    *dest_entry_global;

// ── GTK signal callbacks ─────────────────────────────────────────────────────

/** Open a multi-select file/folder chooser and add items to the list. */
void on_select_files_folders(GtkWidget *widget, gpointer data);

/** Open a folder-chooser to set the backup destination directory. */
void on_select_backup_folder(GtkWidget *widget, gpointer data);

/** Open a folder-chooser to add an entire directory to the backup list. */
void on_select_folder_only(GtkWidget *widget, gpointer data);

/** Remove the currently selected row from the backup list. */
void on_remove_items(GtkWidget *widget, gpointer data);

/** Clear every row from the backup list. */
void on_clear_all(GtkWidget *widget, gpointer data);

/** Immediately trigger a backup (unless one is already running). */
void on_backup_now(GtkWidget *widget, gpointer data);

/** Open the Settings dialog. */
void on_settings(GtkWidget *widget, gpointer data);

/** Open the Backup History / log viewer dialog. */
void on_view_log(GtkWidget *widget, gpointer data);

// ── Window construction ───────────────────────────────────────────────────────

/**
 * Build and display the main application window.
 * Must be called after gtk_init().
 */
void create_main_window();
