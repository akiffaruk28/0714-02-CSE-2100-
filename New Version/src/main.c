#include "../headers/backup_settings.h"
#include "../headers/config_manager.h"
#include "../headers/ui_components.h"
#include "../headers/backup_core.h"

int main(int argc, char *argv[]) {
    g_setenv("GSETTINGS_SCHEMA_DIR", "C:\\msys64\\mingw64\\share\\glib-2.0\\schemas", FALSE);
    g_setenv("PATH", "C:\\msys64\\mingw64\\bin", FALSE);

    gtk_init(&argc, &argv);

    load_settings();
    create_main_window();

    if (settings.auto_backup && timer_id == 0) {
        timer_id = g_timeout_add_seconds(settings.backup_interval, auto_backup_timer, NULL);
    }

    gtk_main();
    return 0;
}
