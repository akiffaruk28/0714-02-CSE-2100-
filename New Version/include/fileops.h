/**
 * @file   fileops.h
 * @brief  File queue management prototypes — add files and folders to backup list.
 *
 * Dependencies: standard C library only
 */

#ifndef FILEOPS_H
#define FILEOPS_H

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief  Add a single file to the GTK backup queue list.
 *
 * Skips if: stat() fails, file is hidden and include_hidden is off,
 * or the path is already present in the queue.
 *
 * @param path  Absolute path to the file.
 */
void add_file_to_list(const char *path);

/**
 * @brief  Recursively (or not) add all files in a folder to the queue.
 *
 * @param path       Absolute path to the folder.
 * @param recursive  Non-zero to recurse into subdirectories.
 */
void add_folder_to_list(const char *path, int recursive);

#endif /* FILEOPS_H */
