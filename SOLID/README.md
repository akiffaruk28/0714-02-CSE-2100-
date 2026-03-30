# Smart Backup Utility — SOLID Refactored

> **Assignment 2 · SOLID OOD Refactoring · Branch: `solid-refactor`**

A GTK-based desktop backup application for Linux, fully refactored from a C-style codebase to a modern **C++17 OOP** architecture following all five **SOLID design principles**.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture Evolution](#architecture-evolution)
3. [Project Structure](#project-structure)
4. [SOLID Principles Applied](#solid-principles-applied)
5. [Class Reference](#class-reference)
6. [Build & Run](#build--run)
7. [Requirements](#requirements)

---

## Project Overview

**Smart Backup Utility** is a GTK+ 3.0 desktop application that allows users to:

- Select files and folders to back up
- Configure backup destination, schedule, and options
- Run backups with real-time progress feedback
- Receive desktop notifications on completion

The codebase was refactored step-by-step from a flat C-style structure into a clean, maintainable, and extensible C++ OOP design using SOLID principles.

---

## Architecture Evolution

The refactoring was carried out in three incremental steps:

### Step 1 — C to C++ Conversion

All `.c` source files were renamed to `.cpp` and the project was reorganized into `include/` and `src/` folders. No logic was changed; the goal was to establish a C++ foundation and verify compilation with `g++`.

| Item | Before | After |
|------|--------|-------|
| File extension | `.c` | `.cpp` |
| Compiler | `gcc` | `g++` |
| Code logic | C-style | Unchanged (C is valid C++) |
| Structure | Flat | `include/` + `src/` folders |

### Step 2 — OOP Refactoring (SRP)

Each responsibility was extracted into its own class. The monolithic structure was broken into focused, single-purpose components:

- `BackupManager` — orchestrates backup flow only
- `SettingsManager` — reads/writes config only
- `FileOperations` — filesystem queries and copies only
- `MainWindow` — GTK UI management only
- `Utils` — stateless helper functions only

### Step 3 — Interface Introduction (OCP, LSP, ISP, DIP)

Abstract interfaces were introduced so that concrete implementations can be swapped or extended without modifying existing code:

- `IBackupStrategy` — allows adding `ZipBackupStrategy`, `CloudBackupStrategy`, etc.
- `IBackupObserver` — allows multiple progress listeners
- `IFileScanner` — allows custom file scanning strategies
- `ISettingsRepository` — allows different persistence backends
- `INotifier` — allows swapping GTK notifications for other systems

All dependencies are wired in `main.cpp` and injected downward — no concrete type is visible to `BackupManager` or `MainWindow`.

---

## Project Structure

```
SmartBackup/
├── include/
│   ├── backup.h        # IBackupStrategy, IBackupObserver, BackupManager
│   ├── fileops.h       # IFileScanner, FileOperations, RecursiveFileScanner
│   ├── settings.h      # ISettingsRepository, BackupConfig, SettingsManager
│   ├── utils.h         # Utils namespace (pure helper functions)
│   └── window.h        # INotifier, GtkNotifier, MainWindow (IBackupObserver)
└── src/
    ├── core/
    │   ├── backup.cpp      # BackupManager implementation
    │   └── settings.cpp    # SettingsManager implementation
    ├── main/
    │   └── main.cpp        # Entry point + dependency injection wiring
    ├── rendering/
    │   └── window.cpp      # GTK UI + GtkNotifier implementation
    └── utils/
        ├── fileops.cpp     # FileOperations + RecursiveFileScanner
        └── utils.cpp       # Utils namespace implementation
```

---

## SOLID Principles Applied

### S — Single Responsibility Principle (SRP)

Every class has exactly **one reason to change**:

| Class | Single Responsibility |
|-------|----------------------|
| `BackupManager` | Orchestrate the backup flow — nothing else |
| `SettingsManager` | Read and write `BackupConfig` to disk — nothing else |
| `FileOperations` | Provide filesystem queries and file copy — nothing else |
| `MainWindow` | Manage GTK widgets and UI events — nothing else |
| `Utils` | Stateless helper functions (time, string, path) — no side effects |

`SettingsManager` deliberately does **not** start timers, create directories, or touch GTK. `FileOperations` does **not** know about config or UI.

---

### O — Open/Closed Principle (OCP)

Classes are **open for extension, closed for modification**:

- Add a `ZipBackupStrategy` or `CloudBackupStrategy` by implementing `IBackupStrategy` — `BackupManager` is untouched.
- Add a filtered file scanner by implementing `IFileScanner` — `FileOperations` is untouched.
- Add a new observer (e.g. log writer) by implementing `IBackupObserver` — `BackupManager` is untouched.

```cpp
// Adding a new strategy requires ZERO changes to BackupManager:
class ZipBackupStrategy : public IBackupStrategy {
public:
    bool copyFile(const std::string& src, const std::string& dest) override { /* zip logic */ }
    std::string getStrategyName() const override { return "ZipBackup"; }
};
```

---

### L — Liskov Substitution Principle (LSP)

All concrete implementations are **fully substitutable** for their abstract base:

| Concrete Class | Base Interface | Substitutable? |
|----------------|----------------|----------------|
| `GtkNotifier` | `INotifier` | ✅ Yes |
| `RecursiveFileScanner` | `IFileScanner` | ✅ Yes |
| `MainWindow` | `IBackupObserver` | ✅ Yes |
| `SettingsManager` | `ISettingsRepository` | ✅ Yes |
| `SimpleFileCopyStrategy` | `IBackupStrategy` | ✅ Yes |

Any code that holds a pointer to `INotifier` will work identically whether it contains `GtkNotifier` or any future `SystemTrayNotifier`.

---

### I — Interface Segregation Principle (ISP)

Interfaces are **narrow and focused** — no class is forced to implement methods it doesn't need:

| Interface | Methods | Purpose |
|-----------|---------|---------|
| `IBackupStrategy` | `copyFile()`, `getStrategyName()` | File copy concern only |
| `IBackupObserver` | `onProgress()`, `onComplete()`, `onError()` | Progress notification only |
| `INotifier` | `notify()` | Desktop notification only |
| `ISettingsRepository` | `load()`, `save()` | Persistence only |
| `IFileScanner` | `scan()` | File discovery only |

---

### D — Dependency Inversion Principle (DIP)

**High-level modules depend on abstractions**, not concrete classes. All concrete types are created and injected in `main.cpp`:

```cpp
// main.cpp — all wiring happens here, nowhere else
SettingsManager    settingsMgr("backup_config.txt");
SimpleFileCopyStrategy copyStrategy;               // IBackupStrategy
BackupManager      backupMgr(&copyStrategy);       // receives abstraction
GtkNotifier        notifier(nullptr);              // INotifier
MainWindow         window(&backupMgr, &settingsMgr, &notifier); // receives abstractions
```

`BackupManager` only knows about `IBackupStrategy*` and `IBackupObserver*`. `MainWindow` only knows about `BackupManager*`, `SettingsManager*`, and `INotifier*`. No concrete type leaks across module boundaries.

---

## Class Reference

### `BackupManager` (`include/backup.h`)

Orchestrates the backup process. Delegates file copying to the injected `IBackupStrategy` and reports progress via `IBackupObserver`.

```cpp
explicit BackupManager(IBackupStrategy* strategy);
void setObserver(IBackupObserver* observer);
bool runBackup(const std::vector<std::string>& items, const std::string& destination);
bool isRunning() const;
```

---

### `SettingsManager` (`include/settings.h`)

Reads and writes `BackupConfig` to a plain-text config file. Contains no GTK code and does not apply settings to any running component.

```cpp
explicit SettingsManager(const std::string& configFilePath);
bool load() override;
bool save() override;
BackupConfig& getConfig();
void setConfig(const BackupConfig& config);
```

**`BackupConfig` fields:**

| Field | Default | Description |
|-------|---------|-------------|
| `backupDestination` | `C:\Backups` | Target folder |
| `autoBackup` | `true` | Enable scheduled backups |
| `backupInterval` | `300` (seconds) | Time between auto-backups |
| `maxCopies` | `10` | Max backup copies to retain |
| `backupSubfolders` | `true` | Include subdirectories |
| `includeHidden` | `false` | Include hidden files |
| `showNotifications` | `true` | Show desktop notifications |

---

### `FileOperations` (`include/fileops.h`)

Static filesystem helper. No instance state. Used by `BackupManager` and `RecursiveFileScanner`.

```cpp
static bool        fileExists(const std::string& path);
static bool        isDirectory(const std::string& path);
static long        fileSize(const std::string& path);
static std::string lastModified(const std::string& path);
static bool        copyFile(const std::string& src, const std::string& dest);
static bool        createDirectory(const std::string& path);
static std::string formatSize(long bytes);
```

---

### `RecursiveFileScanner` (`include/fileops.h`)

Implements `IFileScanner`. Walks a directory tree and returns all matching file paths.

```cpp
std::vector<std::string> scan(const std::string& path, bool recursive, bool includeHidden) override;
```

---

### `Utils` namespace (`include/utils.h`)

Pure, stateless helper functions. No GTK dependencies, no global state.

```cpp
std::string formatTimestamp(time_t t, const std::string& fmt = "%Y-%m-%d %H:%M");
std::string currentTimestampForPath();   // e.g. "20260330_2245"
std::string baseName(const std::string& path);
std::string joinPath(const std::string& dir, const std::string& file);
std::string formatSize(long bytes);
std::string intToStr(int value);
bool        startsWith(const std::string& s, const std::string& prefix);
```

---

### `MainWindow` (`include/window.h`)

Manages all GTK widgets, builds the UI, and implements `IBackupObserver` to receive live progress updates from `BackupManager`. Receives all dependencies via constructor injection.

```cpp
MainWindow(BackupManager* backupMgr, SettingsManager* settingsMgr, INotifier* notifier);
void build();
void show();

// IBackupObserver
void onProgress(int current, int total, const std::string& filename) override;
void onComplete(int success, int total) override;
void onError(const std::string& message) override;
```

---

### `GtkNotifier` (`include/window.h`)

Implements `INotifier` using `GtkMessageDialog`. Substitutable with any other `INotifier` implementation.

```cpp
explicit GtkNotifier(GtkWindow* parent);
void notify(const std::string& title, const std::string& message) override;
```

---

## Build & Run

```bash
# Build the executable
make

# Run the application
./SmartBackup

# Remove build artifacts
make clean
```

The `Makefile` compiles all six source files with C++17 and links against GTK+ 3.0:

```
src/main/main.cpp
src/core/backup.cpp
src/core/settings.cpp
src/rendering/window.cpp
src/utils/fileops.cpp
src/utils/utils.cpp
```

---

## Requirements

| Dependency | Version | Install |
|------------|---------|---------|
| g++ | C++17 or later | `sudo apt install g++` |
| GTK+ 3.0 | 3.x | `sudo apt install libgtk-3-dev` |
| pkg-config | any | `sudo apt install pkg-config` |

Verify GTK is available:
```bash
pkg-config --modversion gtk+-3.0
```

---

## Extending the Application

Because SOLID principles are applied throughout, adding new features requires minimal changes:

**Add a new backup strategy** (e.g. compress to ZIP):
1. Create `ZipBackupStrategy : public IBackupStrategy` in a new `.h/.cpp` pair
2. Instantiate it in `main.cpp` and pass it to `BackupManager`
3. Zero changes to `BackupManager`, `MainWindow`, or any other class

**Add a new notification backend** (e.g. system tray):
1. Create `TrayNotifier : public INotifier`
2. Pass it to `MainWindow` in `main.cpp`
3. Zero changes to `MainWindow` or `BackupManager`

**Add a custom file scanner** (e.g. filter by extension):
1. Create `ExtensionFileScanner : public IFileScanner`
2. Use it wherever `IFileScanner` is accepted
3. Zero changes to `BackupManager` or `FileOperations`

---

*Refactored and verified — March 30, 2026*
