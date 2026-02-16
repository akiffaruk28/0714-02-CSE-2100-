#include "../headers/config_manager.h"
#include "../headers/progress_display.h"
#include "../headers/backup_core.h"

void load_settings() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        save_settings();
        return;
    }

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if (!key || !value) continue;

        if (strcmp(key, "destination") == 0)
            strcpy(settings.backup_destination, value);
        else if (strcmp(key, "auto_backup") == 0)
            settings.auto_backup = atoi(value);
        else if (strcmp(key, "interval") == 0)
            settings.backup_interval = atoi(value);
        else if (strcmp(key, "max_copies") == 0)
            settings.max_copies = atoi(value);
        else if (strcmp(key, "subfolders") == 0)
            settings.backup_subfolders = atoi(value);
        else if (strcmp(key, "hidden") == 0)
            settings.include_hidden = atoi(value);
    }
    fclose(file);

    mkdir(settings.backup_destination);
}

void save_settings() {
    FILE *file = fopen(CONFIG_FILE, "w");
    if (!file) return;

    fprintf(file, "destination=%s\n", settings.backup_destination);
    fprintf(file, "auto_backup=%d\n", settings.auto_backup);
    fprintf(file, "interval=%d\n", settings.backup_interval);
    fprintf(file, "max_copies=%d\n", settings.max_copies);
    fprintf(file, "subfolders=%d\n", settings.backup_subfolders);
    fprintf(file, "hidden=%d\n", settings.include_hidden);

    fclose(file);
}

void on_settings(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Backup Settings",
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        "_Save", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
    gtk_container_add(GTK_CONTAINER(content), grid);

    int row = 0;

    GtkWidget *dest_label = gtk_label_new("Backup Destination:");
    gtk_widget_set_halign(dest_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), dest_label, 0, row, 1, 1);

    GtkWidget *dest_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach(GTK_GRID(grid), dest_box, 1, row, 2, 1);

    dest_entry_global = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dest_entry_global), settings.backup_destination);
    gtk_widget_set_hexpand(dest_entry_global, TRUE);
    gtk_box_pack_start(GTK_BOX(dest_box), dest_entry_global, TRUE, TRUE, 0);

    GtkWidget *browse_btn = gtk_button_new_with_label("Browse...");
    g_signal_connect(browse_btn, "clicked", G_CALLBACK(on_select_backup_folder), NULL);
    gtk_box_pack_start(GTK_BOX(dest_box), browse_btn, FALSE, FALSE, 0);
    row++;

    GtkWidget *auto_label = gtk_label_new("Auto Backup:");
    gtk_grid_attach(GTK_GRID(grid), auto_label, 0, row, 1, 1);

    GtkWidget *auto_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(auto_switch), settings.auto_backup);
    gtk_grid_attach(GTK_GRID(grid), auto_switch, 1, row, 1, 1);
    row++;

    GtkWidget *interval_label = gtk_label_new("Interval (seconds):");
    gtk_grid_attach(GTK_GRID(grid), interval_label, 0, row, 1, 1);

    GtkWidget *interval_spin = gtk_spin_button_new_with_range(60, 86400, 60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(interval_spin), settings.backup_interval);
    gtk_grid_attach(GTK_GRID(grid), interval_spin, 1, row, 1, 1);
    row++;

    GtkWidget *copies_label = gtk_label_new("Max Copies:");
    gtk_grid_attach(GTK_GRID(grid), copies_label, 0, row, 1, 1);

    GtkWidget *copies_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(copies_spin), settings.max_copies);
    gtk_grid_attach(GTK_GRID(grid), copies_spin, 1, row, 1, 1);
    row++;

    GtkWidget *sub_label = gtk_label_new("Include Subfolders:");
    gtk_grid_attach(GTK_GRID(grid), sub_label, 0, row, 1, 1);

    GtkWidget *sub_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(sub_switch), settings.backup_subfolders);
    gtk_grid_attach(GTK_GRID(grid), sub_switch, 1, row, 1, 1);
    row++;

    GtkWidget *hidden_label = gtk_label_new("Include Hidden:");
    gtk_grid_attach(GTK_GRID(grid), hidden_label, 0, row, 1, 1);

    GtkWidget *hidden_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(hidden_switch), settings.include_hidden);
    gtk_grid_attach(GTK_GRID(grid), hidden_switch, 1, row, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        strcpy(settings.backup_destination, gtk_entry_get_text(GTK_ENTRY(dest_entry_global)));
        settings.auto_backup = gtk_switch_get_active(GTK_SWITCH(auto_switch));
        settings.backup_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(interval_spin));
        settings.max_copies = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(copies_spin));
        settings.backup_subfolders = gtk_switch_get_active(GTK_SWITCH(sub_switch));
        settings.include_hidden = gtk_switch_get_active(GTK_SWITCH(hidden_switch));

        save_settings();
        mkdir(settings.backup_destination);

        if (timer_id) {
            g_source_remove(timer_id);
            timer_id = 0;
        }
        if (settings.auto_backup) {
            timer_id = g_timeout_add_seconds(settings.backup_interval, auto_backup_timer, NULL);
        }

        update_status("Settings saved", 0.0);
    }

    gtk_widget_destroy(dialog);
}
