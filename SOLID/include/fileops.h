#ifndef FILEOPS_H
#define FILEOPS_H

// ============================================================
//  fileops.h
//  SRP  – Responsible only for scanning the filesystem and
//          copying individual files.
//  OCP  – FileList is an opaque context; new scan strategies
//          can be added without changing callers.
//  LSP  – fileops_copy_file is a pure function; any caller
//          that uses it can substitute a mock with the same sig.
//  ISP  – Backup core and UI each import only what they need
//          from this header.
//  DIP  – fileops_scan_folder accepts a callback so the caller
//          (backup core) decides what to do with each path.
// ============================================================

#include "settings.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Opaque list used by the UI layer ─────────────────────── */
#define FILEOPS_MAX_ITEMS 1000

typedef struct {
    char paths[FILEOPS_MAX_ITEMS][MAX_PATH_LEN];
    int  count;
} FileList;

/* Initialise (zero out) a FileList */
void filelist_init(FileList *list);

/* Add a single file path if it is not already present.
   Respects settings->include_hidden.
   Returns 1 if added, 0 if skipped/duplicate. */
int filelist_add_file(FileList *list, const char *path,
                      const BackupSettings *s);

/* Recursively scan a directory and add every file found. */
void filelist_add_folder(FileList *list, const char *dir_path,
                         int recursive, const BackupSettings *s);

/* ── Low-level copy (used by backup core) ─────────────────── */

/* Copy src to dst using binary streams.
   Returns 1 on success, 0 on failure. */
int fileops_copy_file(const char *src, const char *dst);

#ifdef __cplusplus
}
#endif

#endif /* FILEOPS_H */
