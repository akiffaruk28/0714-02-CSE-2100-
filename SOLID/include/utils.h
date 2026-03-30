#ifndef UTILS_H
#define UTILS_H

// ============================================================
//  utils.h
//  SRP  – General-purpose helpers that have no dependency on
//          GTK or on backup-domain logic.
//  ISP  – Only exposes the small surface that callers actually
//          need (format helpers, time strings, directory ops).
// ============================================================

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Format a byte count into a human-readable string (e.g. "1.4 MB").
   buf must be at least 32 bytes. */
void utils_format_size(long bytes, char *buf, size_t buf_len);

/* Write a formatted timestamp into buf (format: "YYYY-MM-DD HH:MM").
   buf must be at least 20 bytes. */
void utils_format_time(time_t t, char *buf, size_t buf_len);

/* Create a directory; silently succeeds if it already exists.
   Returns 0 on success, -1 on error. */
int utils_mkdir(const char *path);

/* Returns 1 if the last component of path starts with '.' */
int utils_is_hidden(const char *path);

/* Safely concatenate dir + separator + name into dest.
   dest_len is the size of dest buffer. */
void utils_path_join(char *dest, size_t dest_len,
                     const char *dir, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
