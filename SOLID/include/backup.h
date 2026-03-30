#ifndef BACKUP_H
#define BACKUP_H

// ============================================================
//  backup.h
//  SRP  – Orchestrates the backup operation only; does NOT
//          know about GTK widgets or file-scanning details.
//  OCP  – Progress / completion callbacks let callers extend
//          behaviour (logging, UI updates) without modifying
//          the backup engine.
//  DIP  – Depends on abstractions: BackupSettings (settings.h),
//          FileList (fileops.h), and function-pointer callbacks.
// ============================================================

#include "settings.h"
#include "fileops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Callback types (DIP / OCP) ─────────────────────────── */

/* Called after each file is processed.
   current  = 1-based index of the file just processed.
   total    = total number of files in this run.
   filename = basename of the file just processed.
   user_data= opaque pointer supplied by the caller. */
typedef void (*BackupProgressCb)(int current, int total,
                                 const char *filename,
                                 void *user_data);

/* Called once when the backup finishes.
   success = number of files copied successfully.
   total   = total attempted. */
typedef void (*BackupCompleteCb)(int success, int total,
                                 void *user_data);

/* ── Result record ───────────────────────────────────────── */
typedef struct {
    int files_total;
    int files_success;
    char backup_dir[MAX_PATH_LEN]; /* directory created for this run */
} BackupResult;

/* ── Public API ──────────────────────────────────────────── */

/* Run a synchronous backup.
   progress_cb and complete_cb may be NULL.
   Returns a populated BackupResult. */
BackupResult backup_run(const FileList        *list,
                        const BackupSettings  *settings,
                        BackupProgressCb       progress_cb,
                        BackupCompleteCb       complete_cb,
                        void                  *user_data);

/* Write (or append) a log entry to the standard log file. */
void backup_write_log(const BackupResult *result,
                      const BackupSettings *settings);

#ifdef __cplusplus
}
#endif

#endif /* BACKUP_H */
