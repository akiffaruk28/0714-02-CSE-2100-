#pragma once
#include <gtk/gtk.h>

// Backup-in-progress guard (defined in core/backup.cpp)
extern gboolean backup_running;

// Auto-backup timer handle (defined in core/backup.cpp)
extern guint timer_id;

/**
 * Perform a full backup of every item in the list.
 * Designed to run via g_idle_add(); always returns G_SOURCE_REMOVE.
 *
 * Creates a timestamped sub-folder inside settings.backup_destination
 * and writes a backup_log.txt there on completion.
 *
 * @param data  Unused (required by GSourceFunc signature).
 */
gboolean perform_backup(gpointer data);

/**
 * Timer callback that triggers an auto-backup when the interval fires.
 * Returns G_SOURCE_CONTINUE so the timer keeps repeating.
 *
 * @param data  Unused.
 */
gboolean auto_backup_timer(gpointer data);
