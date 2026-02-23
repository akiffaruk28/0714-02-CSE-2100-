/**
 * @file   utils.h
 * @brief  Shared display utility prototypes — status bar and notification dialog.
 *
 * Dependencies: standard C library only (GTK pointers accessed via window.h externs)
 */

#ifndef UTILS_H
#define UTILS_H

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Update the status label and progress bar in the main window.
 *
 * Also drains the GTK event queue so the UI repaints immediately.
 * Must be called from the GTK main thread.
 *
 * @param message   Text to display in the status label.
 * @param progress  Progress fraction in [0.0, 1.0].
 */
void update_status(const char *message, double progress);

/**
 * @brief  Show a modal info dialog with a title and message.
 *
 * Does nothing if settings.show_notifications is 0.
 *
 * @param title  Dialog window title.
 * @param msg    Message body text.
 */
void show_notification_msg(const char *title, const char *msg);

#endif /* UTILS_H */
