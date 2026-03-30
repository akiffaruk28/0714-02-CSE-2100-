#ifndef WINDOW_H
#define WINDOW_H

// ============================================================
//  window.h
//  SRP  – Owns the GTK main window and all UI state.
//  ISP  – Exposes only the two entry-points other modules need:
//          window_create() and window_run().
//  DIP  – The window depends on BackupSettings and the backup/
//          fileops abstractions, not on concrete implementations.
// ============================================================

#include "settings.h"
#include "fileops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AppContext bundles all shared UI + domain state.
   Keeps global variables off the table (SRP). */
typedef struct {
    BackupSettings settings;
    FileList       file_list;

    /* GTK widget handles needed across callbacks */
    void *window;          /* GtkWidget* */
    void *progress_bar;    /* GtkWidget* */
    void *status_label;    /* GtkWidget* */
    void *items_list;      /* GtkListStore* */
    void *treeview;        /* GtkWidget* */
    void *dest_entry;      /* GtkWidget* – settings dialog */

    int  backup_running;   /* bool */
    unsigned int timer_id; /* g_timeout source id */
} AppContext;

/* Build and show the main window.
   ctx must be zero-initialised and have ctx->settings populated. */
void window_create(AppContext *ctx);

/* Enter the GTK main loop (blocks until the window is closed). */
void window_run(void);

#ifdef __cplusplus
}
#endif

#endif /* WINDOW_H */
