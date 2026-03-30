#pragma once
#include <gtk/gtk.h>

// Forward declaration – defined in rendering/window.cpp
extern GtkWidget *window;
extern GtkWidget *progress_bar;
extern GtkWidget *status_label;

/**
 * Update the status label and progress bar in the main window.
 * Also flushes the GTK event queue so the UI stays responsive.
 *
 * @param message  Text to display in the status label.
 * @param progress Value in [0.0, 1.0] for the progress bar.
 */
void update_status(const char *message, double progress);

/**
 * Show a modal information dialog (respects settings.show_notifications).
 *
 * @param title  Dialog window title.
 * @param msg    Message body.
 */
void show_notification_msg(const char *title, const char *msg);
