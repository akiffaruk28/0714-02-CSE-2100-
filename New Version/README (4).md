# 🗂️ Smart Backup Utility — Code Refactoring & Software Engineering Standards Guide

**Course:** Advanced Programming Lab
**Project:** Smart Backup Utility (C Language)
**Purpose:** Improve code maintainability, readability, modularity, and scalability
**Date:** February 2026

---

## 📑 Table of Contents

1. [Executive Summary](#executive-summary)
2. [What Changed: v1.0 → v2.0](#what-changed-v10--v20)
3. [Naming Conventions](#naming-conventions)
4. [Coding Style Guidelines](#coding-style-guidelines)
5. [Folder Structure](#folder-structure)
6. [Modular Design Principles](#modular-design-principles)
7. [Bug Fixes](#bug-fixes)
8. [Error Handling & Memory Safety](#error-handling--memory-safety)
9. [Future Improvements](#future-improvements)

---

## Executive Summary

### Project Overview

Smart Backup Utility is a GTK3-based desktop application for Windows that allows users to:
- Select files and folders for backup
- Queue them and copy to a chosen destination
- Schedule automatic backups on a timer
- View per-session logs with success/failure tracking

**Strengths:**
- Functional file copying and queue system
- GTK3 GUI with progress bar and status updates
- Auto-backup timer with configurable interval
- Per-session backup log with timestamps
- Settings persistence via config file

**Areas for Improvement (v1.0):**
- All ~700 lines of code in a single `main.c`
- Inconsistent use of `sprintf` (unsafe) vs `snprintf` (safe)
- Runtime crash bug in progress bar reset
- Dangling pointer in settings dialog
- No separation between UI, logic, and utilities

### Refactoring Philosophy

> "Refactor incrementally without breaking existing functionality."

**Goals:**
- Cleaner, modular code structure
- Easier debugging and navigation
- Professional project layout
- Safer string and memory handling

---

## What Changed: v1.0 → v2.0

### Version Overview

| | v1.0 (Before) | v2.0 (After) |
|---|---|---|
| Files | 1 file (`main.c`) | 6 source + 5 header files |
| Lines per file | ~700 lines | 20–350 lines each |
| Bug count | 3 known bugs | 0 known bugs |
| String safety | `sprintf`, `strcpy` | `snprintf`, `strncpy` |
| Failure logging | Silent skip | Logged as `FAIL` |
| `.gitignore` | ❌ | ✅ |

---

## Naming Conventions

### Variable Naming

#### Before
```c
int tm;
struct tm *tm;
char log[512];
```

**Issues:**
- `tm` shadows the standard library `struct tm`
- Ambiguous names — is `log` a file path or content?

#### After
```c
struct tm *tm_info;
char log_path[MAX_PATH];
char log_content[256];
```

**Benefits:**
- No stdlib name collisions
- Intent is immediately clear

---

### Constants Naming

#### Before
```c
#define MAX_PATH 512
#define LOG_FILE "backup_log.txt"
#define CONFIG_FILE "backup_config.txt"
```

#### After
```c
#define BACKUP_MAX_PATH_LENGTH   512
#define BACKUP_LOG_FILENAME      "backup_log.txt"
#define BACKUP_CONFIG_FILENAME   "backup_config.txt"
#define BACKUP_COPY_BUFFER_SIZE  8192
#define BACKUP_DIR_PREFIX        "Backup_"
```

**Improvement:**
- `BACKUP_` prefix groups related constants
- Eliminates ambiguity with system macros

---

### Function Naming

#### Before
```c
void load_settings();
void add_file_to_list();
gboolean perform_backup();
```

#### After — same names but now each lives in its own file:
```c
// settings.c
void load_settings(void);
void save_settings(void);

// fileops.c
void add_file_to_list(const char *path);
void add_folder_to_list(const char *path, int recursive);

// backup.c
gboolean perform_backup(gpointer data);
gboolean auto_backup_timer(gpointer data);
```

**Why This Matters:**
- Functions are grouped by responsibility
- Easy to find where a function lives

---

## Coding Style Guidelines

### Indentation
- **4 spaces** used throughout
- No mixed tabs and spaces

### Comment Quality

#### Before
```c
mkdir(backup_dir); // make dir
```

#### After
```c
/* Create timestamped backup directory, e.g. C:\Backups\Backup_20250224_1530 */
mkdir(backup_dir);
```

**Principle:**
Comments should explain **why** or **what it produces**, not just restate the code.

### Line Safety

#### Before
```c
sprintf(backup_dir, "%s\\Backup_%04d...", settings.backup_destination, ...);
strcpy(settings.backup_destination, value);
```

#### After
```c
snprintf(backup_dir, sizeof(backup_dir), "%s\\Backup_%04d...", settings.backup_destination, ...);
strncpy(settings.backup_destination, value, BACKUP_MAX_PATH_LENGTH - 1);
```

| Function | Risk | Replacement |
|---|---|---|
| `sprintf()` | Buffer overflow | `snprintf()` |
| `strcpy()` | Buffer overflow | `strncpy()` |
| `%ld` with `off_t` | Type mismatch warning | Cast to `(long)` first |

---

## Folder Structure

### Before Refactoring
```
smart-backup-utility/
└── main.c          ← Everything in one file (~700 lines)
```

**Problems:**
- Impossible to navigate quickly
- UI code and backup logic mixed together
- Two people cannot work on different features simultaneously

### After Refactoring
```
smart-backup-utility/
│
├── src/
│   ├── main/
│   │   └── main.c              ← Entry point only (~20 lines)
│   ├── core/
│   │   ├── backup.c            ← File copying & logging
│   │   ├── fileops.c           ← Queue management
│   │   └── settings.c          ← Config load/save
│   ├── rendering/
│   │   └── window.c            ← GTK window & all callbacks
│   └── utils/
│       └── utils.c             ← Status bar & notifications
│
├── include/
│   ├── backup.h
│   ├── fileops.h
│   ├── settings.h
│   ├── utils.h
│   └── window.h
│
├── build/                      ← Compiled output (.exe)
├── Makefile
├── README.md
└── .gitignore
```

**Benefits:**
- Each file has **one clear responsibility**
- Easy to find and fix bugs
- Professional project layout

---

## Modular Design Principles

### Before — Everything Mixed Together

```c
// main.c — all in one place
void load_settings() { ... }      // config logic
void add_file_to_list() { ... }   // queue logic
void perform_backup() { ... }     // backup logic
void create_main_window() { ... } // UI logic
void update_status() { ... }      // utility
int main() { ... }                // entry point
```

**Problems:**
- Changing one thing risks breaking another
- Hard to debug — error could be anywhere
- Hard to reuse individual parts

### After — Separated by Responsibility

```
main.c          → starts the app, nothing else
settings.c      → only reads/writes config file
fileops.c       → only manages the file queue
backup.c        → only copies files and writes logs
window.c        → only builds the UI and handles buttons
utils.c         → only updates status bar and popups
```

**The rule followed:**
> Each module should have **one reason to change**.

If the backup logic needs to change — only `backup.c` is touched.
If the UI needs to change — only `window.c` is touched.

---

## Bug Fixes

### Bug 1 — App Crash After Backup *(Critical)*
**File:** `backup.c`

**Root Cause:**
`gtk_progress_bar_set_fraction()` requires a `double` as its second argument. It was being cast and passed as a `GSourceFunc` callback, causing a type mismatch crash at runtime.

```c
// v1.0 — WRONG (crashes at runtime)
g_timeout_add_seconds(3,
    (GSourceFunc)gtk_progress_bar_set_fraction,
    GINT_TO_POINTER(0));
```

**Fix:**
```c
// v2.0 — CORRECT (proper wrapper function)
static gboolean reset_progress_bar(gpointer data) {
    (void)data;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
    return G_SOURCE_REMOVE;
}

g_timeout_add_seconds(3, reset_progress_bar, NULL);
```

---

### Bug 2 — Dangling Pointer in Settings Dialog
**File:** `window.c`

**Root Cause:**
After the Settings dialog was destroyed, `dest_entry_global` still pointed to the freed widget memory. Clicking Browse again would access invalid memory.

```c
// v1.0 — pointer left dangling after destroy
gtk_widget_destroy(dialog);
```

**Fix:**
```c
// v2.0 — reset before destroying
dest_entry_global = NULL;
gtk_widget_destroy(dialog);
```

---

### Bug 3 — Compiler Warning on File Size Format
**File:** `fileops.c`

```c
// v1.0 — type mismatch between off_t and %ld
sprintf(size_str, "%ld B", st.st_size);

// v2.0 — explicit cast resolves warning
snprintf(size_str, sizeof(size_str), "%ld B", (long)st.st_size);
```

---

## Error Handling & Memory Safety

### Failure Logging
In v1.0, failed file copies were silently skipped. In v2.0, every result is recorded:

```
Backup started: Tue Feb 24 15:30:00 2025
OK   C:\Users\user\report.pdf  ->  C:\Backups\Backup_20250224_1530\report.pdf
OK   C:\Users\user\notes.txt   ->  C:\Backups\Backup_20250224_1530\notes.txt
FAIL (cannot open source): C:\locked_file.db
FAIL (cannot create dest): C:\Backups\Backup_20250224_1530\locked.txt

Backup completed: 2/4 files successful
```

### Duplicate Detection
```c
// Check if file already exists in the queue before adding
if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_list), &iter)) {
    do {
        char *existing;
        gtk_tree_model_get(..., 0, &existing, -1);
        if (strcmp(existing, path) == 0) { exists = TRUE; }
        g_free(existing);
    } while (gtk_tree_model_iter_next(...));
}
```

---

## How to Build

**Requirements:**
- Windows with [MSYS2](https://www.msys2.org/) installed
- MinGW64 GTK3 package

**Step 1 — Install GTK3:**
```bash
pacman -S mingw-w64-x86_64-gtk3
```

**Step 2 — Build:**
```bash
make
```

**Step 3 — Run:**
```bash
./build/smart_backup.exe
```

**Clean build files:**
```bash
make clean
```

---

## Future Improvements

### Gameplay Features
- Max backup copies enforcement (currently stored but not enforced)
- Backup compression (`.zip` output)
- Email notification on backup completion

### Technical Enhancements
- Cross-platform support (Linux/macOS path separators)
- Progress bar per-file instead of per-total
- Multi-threaded backup (non-blocking UI)

### Software Engineering Improvements
- Unit tests for `settings.c` and `fileops.c`
- Continuous integration pipeline
- Automatic `.gitignore` generation on first run

---

## Final Note

This refactoring effort aims to:

✅ Improve code quality and readability
✅ Eliminate runtime bugs
✅ Establish a professional project structure
✅ Apply safe string and memory handling practices
✅ Support future feature expansion

A well-structured Backup Utility project demonstrates not only functional file management but also strong software engineering discipline, maintainable architecture, and scalability for future enhancements.
