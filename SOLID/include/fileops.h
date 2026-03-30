#pragma once
#include <gtk/gtk.h>

// Shared list store – defined in rendering/window.cpp
extern GtkListStore *items_list;

/**
 * Add a single file to the backup list.
 * Silently ignores duplicates and hidden files (if settings forbid them).
 *
 * @param path  Absolute path to the file.
 */
void add_file_to_list(const char *path);

/**
 * Recursively (or shallowly) enumerate a directory and add all files.
 *
 * @param path       Absolute path to the directory.
 * @param recursive  Non-zero to descend into sub-directories.
 */
void add_folder_to_list(const char *path, int recursive);
