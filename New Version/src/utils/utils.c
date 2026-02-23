/**
 * @file   utils.c
 * @brief  Shared display utilities — status bar update and notification dialog.
 *
 * Single responsibility: provide UI feedback functions used by
 * multiple modules (backup.c, fileops/window callbacks).
 *
 * Dependencies: utils.h, window.h, settings.h
 * Platform:     Windows (MSYS2/MinGW-w64) with GTK+ 3.0
 */

#include "utils.h"
#include "window.h"
#include "settings.h"

#include <gtk/gtk.h>

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Update the status label and progress bar in the main window.
 *
 * Also drains the GTK event queue so the UI repaints immediately
 * during long-running backup operations.
 * Must be called from the GTK main thread.
 *
 * @param message   Text to display in the status label.
 * @param progress  Progress fraction in [0.0, 1.0].
 */
void update_status(const char *message, double progress) {
    gtk_label_set_text(GTK_LABEL(status_label), message);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);

    /* Force UI repaint during long backup operations */
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
}

/**
 * @brief  Show a modal info dialog with a title and message.
 *
 * Does nothing if settings.show_notifications is 0.
 *
 * @param title  Dialog window title.
 * @param msg    Message body text.
 */
void show_notification_msg(const char *title, const char *msg) {
    if (!settings.show_notifications) { return; }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", msg);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
