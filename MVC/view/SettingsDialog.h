#pragma once
#include <gtk/gtk.h>
#include <string>
#include <functional>
#include "../model/ISettingsModel.h"

#ifdef _WIN32
  #include <windows.h>
  #include <shlobj.h>
#endif

// Windows-safe path existence check — avoids struct stat conflicts inside lambdas
#ifdef _WIN32
  #include <sys/stat.h>
  static inline bool path_exists(const char* p) {
      struct ::_stat64 st; return ::_stat64(p, &st) == 0;
  }
#else
  #include <sys/stat.h>
  static inline bool path_exists(const char* p) {
      struct ::stat st; return ::stat(p, &st) == 0;
  }
#endif

// ── SettingsDialog ────────────────────────────────────────────────────────────
// Improved settings dialog with:
//   • Backup Strategy selector (Full Copy / Incremental)
//   • Retry on Failure toggle
//   • Backup on App Start toggle
//   • Destination validation (shows warning if folder doesn't exist)
//   • Interval combo with human-readable labels + estimated daily backup count
//   • Max Copies tooltip explaining what happens when limit is hit

class SettingsDialog {
public:
    using SaveCallback = std::function<void(
        const std::string& dest,
        const std::string& backupName,
        bool autoBackup,
        int interval,
        int maxCopies,
        bool subfolders,
        bool hidden,
        BackupStrategy strategy,
        bool retryOnFailure,
        bool backupOnAppStart)>;

    static void show(GtkWindow* parent,
                     const BackupSettings& current,
                     SaveCallback onSave)
    {
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "Backup Settings", parent, GTK_DIALOG_MODAL,
            "_Save",   GTK_RESPONSE_ACCEPT,
            "_Cancel", GTK_RESPONSE_REJECT,
            nullptr);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 560, -1);

        GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(content), 0);

        // ── Notebook tabs ─────────────────────────────────────────────────────
        GtkWidget* notebook = gtk_notebook_new();
        gtk_container_set_border_width(GTK_CONTAINER(notebook), 8);
        gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);

        // ── Helper lambdas ────────────────────────────────────────────────────
        auto makeGrid = [](int border) -> GtkWidget* {
            GtkWidget* g = gtk_grid_new();
            gtk_grid_set_row_spacing(GTK_GRID(g), 12);
            gtk_grid_set_column_spacing(GTK_GRID(g), 12);
            gtk_container_set_border_width(GTK_CONTAINER(g), border);
            return g;
        };

        auto addLabel = [](GtkWidget* grid, const char* text, int r, bool bold = false) {
            GtkWidget* lbl = gtk_label_new(nullptr);
            if (bold) {
                std::string markup = std::string("<b>") + text + "</b>";
                gtk_label_set_markup(GTK_LABEL(lbl), markup.c_str());
            } else {
                gtk_label_set_text(GTK_LABEL(lbl), text);
            }
            gtk_widget_set_halign(lbl, GTK_ALIGN_START);
            gtk_grid_attach(GTK_GRID(grid), lbl, 0, r, 1, 1);
        };

        // ════════════════════════════════════════════════════════════════════
        // TAB 1: General
        // ════════════════════════════════════════════════════════════════════
        GtkWidget* tab1Grid = makeGrid(16);
        int row = 0;

        // — Destination —
        addLabel(tab1Grid, "Backup Destination:", row);
        GtkWidget* destEntry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(destEntry), current.destination.c_str());
        gtk_widget_set_hexpand(destEntry, TRUE);
        gtk_grid_attach(GTK_GRID(tab1Grid), destEntry, 1, row, 1, 1);

        // Pack browse button data: entry + parent dialog pointer
        struct BrowseData { GtkWidget* entry; GtkWidget* parentDialog; };
        BrowseData* bd = new BrowseData{destEntry, dialog};

        GtkWidget* browseBtn = gtk_button_new_with_label("Browse…");
        gtk_grid_attach(GTK_GRID(tab1Grid), browseBtn, 2, row, 1, 1);
        g_signal_connect(browseBtn, "clicked",
            G_CALLBACK(+[](GtkWidget*, gpointer data) {
                BrowseData* bd = static_cast<BrowseData*>(data);

#ifdef _WIN32
                // Use Windows native folder picker via SHBrowseForFolder
                BROWSEINFOW bi = {};
                bi.hwndOwner = nullptr;
                bi.lpszTitle = L"Select Backup Destination";
                bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
                LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                if (pidl) {
                    wchar_t path[MAX_PATH] = {};
                    if (SHGetPathFromIDListW(pidl, path)) {
                        // Convert wchar_t to UTF-8 for GTK
                        gchar* utf8 = g_utf16_to_utf8(
                            reinterpret_cast<const gunichar2*>(path), -1,
                            nullptr, nullptr, nullptr);
                        if (utf8) {
                            gtk_entry_set_text(GTK_ENTRY(bd->entry), utf8);
                            g_free(utf8);
                        }
                    }
                    CoTaskMemFree(pidl);
                }
#else
                GtkWidget* chooser = gtk_file_chooser_dialog_new(
                    "Select Backup Destination",
                    GTK_WINDOW(bd->parentDialog),
                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                    "_Cancel", GTK_RESPONSE_CANCEL,
                    "_Select", GTK_RESPONSE_ACCEPT, nullptr);
                if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
                    char* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
                    if (folder) { gtk_entry_set_text(GTK_ENTRY(bd->entry), folder); g_free(folder); }
                }
                gtk_widget_destroy(chooser);
#endif
            }), bd);

        // Free BrowseData when dialog is destroyed
        g_signal_connect(dialog, "destroy",
            G_CALLBACK(+[](GtkWidget*, gpointer data) { delete static_cast<BrowseData*>(data); }), bd);
        row++;

        // — Destination warning label —
        GtkWidget* destWarn = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(destWarn),
            "<span foreground='orange'>⚠ Folder does not exist — it will be created on first backup.</span>");
        gtk_widget_set_halign(destWarn, GTK_ALIGN_START);
        gtk_widget_set_no_show_all(destWarn, TRUE);
        gtk_grid_attach(GTK_GRID(tab1Grid), destWarn, 1, row++, 2, 1);

        // Show warning if dest doesn't exist right now
        if (!current.destination.empty() && !path_exists(current.destination.c_str()))
            gtk_widget_show(destWarn);
        // Live validation as user types
        g_signal_connect(destEntry, "changed",
            G_CALLBACK(+[](GtkWidget* entry, gpointer warn) {
                const char* txt = gtk_entry_get_text(GTK_ENTRY(entry));
                if (txt && *txt && !path_exists(txt))
                    gtk_widget_show(GTK_WIDGET(warn));
                else
                    gtk_widget_hide(GTK_WIDGET(warn));
            }), destWarn);

        // — Backup Name —
        addLabel(tab1Grid, "Backup Folder Name:", row);
        GtkWidget* nameEntry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(nameEntry), "Leave empty for auto timestamp");
        gtk_entry_set_text(GTK_ENTRY(nameEntry), current.backupName.c_str());
        gtk_widget_set_hexpand(nameEntry, TRUE);
        gtk_grid_attach(GTK_GRID(tab1Grid), nameEntry, 1, row++, 2, 1);

        // — Include Subfolders —
        addLabel(tab1Grid, "Include Subfolders:", row);
        GtkWidget* subSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(subSw), current.includeSubfolders);
        gtk_widget_set_halign(subSw, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(tab1Grid), subSw, 1, row++, 1, 1);

        // — Include Hidden Files —
        addLabel(tab1Grid, "Include Hidden Files:", row);
        GtkWidget* hiddenSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(hiddenSw), current.includeHidden);
        gtk_widget_set_halign(hiddenSw, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(tab1Grid), hiddenSw, 1, row++, 1, 1);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab1Grid,
                                 gtk_label_new("General"));

        // ════════════════════════════════════════════════════════════════════
        // TAB 2: Auto Backup
        // ════════════════════════════════════════════════════════════════════
        GtkWidget* tab2Grid = makeGrid(16);
        row = 0;

        // — Auto Backup toggle —
        addLabel(tab2Grid, "Enable Auto Backup:", row);
        GtkWidget* autoSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(autoSw), current.autoBackup);
        gtk_widget_set_halign(autoSw, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(tab2Grid), autoSw, 1, row++, 1, 1);

        // — Interval —
        addLabel(tab2Grid, "Backup Interval:", row);
        const int  presets[] = { 300, 600, 900, 1800, 3600, 7200, 21600, 43200, 86400 };
        const char* labels[] = {
            "5 minutes  (~288/day)",  "10 minutes (~144/day)",
            "15 minutes (~96/day)",   "30 minutes (~48/day)",
            "1 hour     (~24/day)",   "2 hours    (~12/day)",
            "6 hours    (~4/day)",    "12 hours   (~2/day)",
            "24 hours   (1/day)"
        };
        GtkWidget* intervalCombo = gtk_combo_box_text_new();
        int activeIdx = 0;
        for (int i = 0; i < 9; i++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(intervalCombo), labels[i]);
            if (presets[i] <= current.interval) activeIdx = i;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(intervalCombo), activeIdx);
        gtk_widget_set_hexpand(intervalCombo, TRUE);
        gtk_grid_attach(GTK_GRID(tab2Grid), intervalCombo, 1, row++, 2, 1);

        // — Max Copies —
        addLabel(tab2Grid, "Max Backup Copies:", row);
        GtkWidget* copiesSpin = gtk_spin_button_new_with_range(1, 100, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(copiesSpin), current.maxCopies);
        gtk_widget_set_tooltip_text(copiesSpin,
            "When this limit is reached, the oldest backup folder is automatically deleted.");
        gtk_grid_attach(GTK_GRID(tab2Grid), copiesSpin, 1, row++, 1, 1);

        // — Backup on App Start —
        addLabel(tab2Grid, "Backup on App Start:", row);
        GtkWidget* startSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(startSw), current.backupOnAppStart);
        gtk_widget_set_halign(startSw, GTK_ALIGN_START);
        gtk_widget_set_tooltip_text(startSw,
            "Immediately run a backup when the application launches, before the first timer fires.");
        gtk_grid_attach(GTK_GRID(tab2Grid), startSw, 1, row++, 1, 1);

        // — Retry on Failure —
        addLabel(tab2Grid, "Retry on Failure:", row);
        GtkWidget* retrySw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(retrySw), current.retryOnFailure);
        gtk_widget_set_halign(retrySw, GTK_ALIGN_START);
        gtk_widget_set_tooltip_text(retrySw,
            "If a backup fails, automatically retry once after 60 seconds.");
        gtk_grid_attach(GTK_GRID(tab2Grid), retrySw, 1, row++, 1, 1);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab2Grid,
                                 gtk_label_new("Auto Backup"));

        // ════════════════════════════════════════════════════════════════════
        // TAB 3: Strategy
        // ════════════════════════════════════════════════════════════════════
        GtkWidget* tab3Grid = makeGrid(16);
        row = 0;

        addLabel(tab3Grid, "Backup Strategy:", row, true);
        row++;

        // Full Copy radio
        GtkWidget* radioFull = gtk_radio_button_new_with_label(nullptr, "Full Copy");
        gtk_widget_set_tooltip_text(radioFull,
            "Every auto-backup copies ALL selected files — safe, simple, uses more disk space.");
        gtk_grid_attach(GTK_GRID(tab3Grid), radioFull, 0, row++, 3, 1);

        // Incremental radio
        GtkWidget* radioIncr = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(radioFull), "Incremental  (only changed files)");
        gtk_widget_set_tooltip_text(radioIncr,
            "Skips files whose size and modification time haven't changed since the last backup.\n"
            "Much faster for large file sets — ideal for frequent auto-backups.");
        gtk_grid_attach(GTK_GRID(tab3Grid), radioIncr, 0, row++, 3, 1);

        // Set active based on current setting
        if (current.strategy == BackupStrategy::Incremental)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioIncr), TRUE);

        // Info box
        GtkWidget* infoLbl = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(infoLbl),
            "\n<i>Tip: Use <b>Incremental</b> with short intervals (≤15 min) to keep\n"
            "backups fast without filling up your disk quickly.</i>");
        gtk_label_set_line_wrap(GTK_LABEL(infoLbl), TRUE);
        gtk_widget_set_halign(infoLbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(tab3Grid), infoLbl, 0, row++, 3, 1);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab3Grid,
                                 gtk_label_new("Strategy"));

        // ─────────────────────────────────────────────────────────────────────
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT && onSave) {
            // Validate destination
            std::string dest = gtk_entry_get_text(GTK_ENTRY(destEntry));
            if (dest.empty()) dest = current.destination;

            int selIdx = gtk_combo_box_get_active(GTK_COMBO_BOX(intervalCombo));
            const int presetVals[] = { 300, 600, 900, 1800, 3600, 7200, 21600, 43200, 86400 };
            int chosenInterval = (selIdx >= 0 && selIdx < 9) ? presetVals[selIdx] : 300;

            BackupStrategy strat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radioIncr))
                                   ? BackupStrategy::Incremental
                                   : BackupStrategy::FullCopy;

            onSave(
                dest,
                gtk_entry_get_text(GTK_ENTRY(nameEntry)),
                gtk_switch_get_active(GTK_SWITCH(autoSw)),
                chosenInterval,
                gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(copiesSpin)),
                gtk_switch_get_active(GTK_SWITCH(subSw)),
                gtk_switch_get_active(GTK_SWITCH(hiddenSw)),
                strat,
                gtk_switch_get_active(GTK_SWITCH(retrySw)),
                gtk_switch_get_active(GTK_SWITCH(startSw))
            );
        }
        gtk_widget_destroy(dialog);
    }

};
