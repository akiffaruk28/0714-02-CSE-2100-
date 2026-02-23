/**
 * @file   backup.h
 * @brief  Backup engine prototypes — perform_backup, auto_backup_timer.
 *
 * Dependencies: glib.h (for gboolean, gpointer, guint)
 */

#ifndef BACKUP_H
#define BACKUP_H

#include <glib.h>

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Execute a full backup of all queued files.
 *
 * Creates a timestamped directory under settings.backup_destination,
 * copies each file from the GTK list store, writes backup_log.txt.
 * Sets backup_running flag to prevent concurrent runs.
 *
 * @param data  Unused (GLib idle callback parameter). Pass NULL.
 * @return      G_SOURCE_REMOVE always (one-shot idle callback).
 */
gboolean perform_backup(gpointer data);

/**
 * @brief  GLib timer callback — triggers perform_backup if auto-backup is on.
 *
 * @param data  Unused. Pass NULL.
 * @return      G_SOURCE_CONTINUE always (recurring timer).
 */
gboolean auto_backup_timer(gpointer data);

#endif /* BACKUP_H */
