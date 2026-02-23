# Smart Backup Utility

A GTK+ 3.0 desktop backup utility for Windows (MSYS2/MinGW-w64).

## Project Structure

```
smart-backup-utility/
├── src/
│   ├── main/
│   │   └── main.c              # Entry point ONLY (~30 lines)
│   ├── core/
│   │   ├── settings.c          # Config read/write — backup_config.txt
│   │   ├── backup.c            # Backup engine, timer, button callback
│   │   └── fileops.c           # File queue: add/remove/clear
│   ├── rendering/
│   │   └── window.c            # GTK window, all dialogs, callbacks, widget globals
│   └── utils/
│       └── utils.c             # update_status, show_notification_msg
├── include/
│   ├── settings.h              # BackupSettings struct, constants, prototypes
│   ├── window.h                # GTK global widget externs + create_main_window
│   ├── backup.h                # perform_backup, auto_backup_timer prototypes
│   ├── fileops.h               # add_file_to_list, add_folder_to_list prototypes
│   └── utils.h                 # update_status, show_notification_msg prototypes
├── docs/
├── assets/
├── build/
├── Makefile
├── .gitignore
└── README.md
```

## Module Responsibilities

| File | Responsibility |
|------|---------------|
| `main.c` | GTK init, startup sequence, event loop |
| `settings.c` | Read/write `backup_config.txt` |
| `fileops.c` | Populate the GTK backup queue list |
| `backup.c` | Copy files, write session log, auto-timer |
| `window.c` | All GTK widgets, dialogs, button callbacks |
| `utils.c` | Status bar update, notification dialog |

## Include Dependency Graph

```
main.c        → settings.h, window.h, backup.h
window.c      → window.h, settings.h, fileops.h, backup.h, utils.h
backup.c      → backup.h, settings.h, window.h, utils.h
fileops.c     → fileops.h, settings.h, window.h
settings.c    → settings.h  (no other project headers)
utils.c       → utils.h, window.h, settings.h
```

No circular dependencies. `settings.h` is the stable foundation layer.

## Build

Requires MSYS2 with MinGW-w64 and GTK+ 3.0:

```bash
pacman -S mingw-w64-x86_64-gtk3
make
```

Binary output: `build/smart_backup.exe`

## Features

- Add individual files or entire folders to a backup queue
- Recursive subfolder support with per-dialog toggle
- Timestamped backup directories (`Backup_YYYYMMDD_HHMM`)
- Per-session `backup_log.txt` with success/failure per file
- Auto-backup on configurable timer interval
- Settings dialog: destination, interval, max copies, hidden files
- Backup history viewer (shows latest log)
