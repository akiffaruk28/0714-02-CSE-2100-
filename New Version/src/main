

#include <gtk/gtk.h>

#include "settings.h"
#include "window.h"
#include "backup.h"

int main(int argc, char *argv[]) {
    /* Configure MSYS2/MinGW-w64 GTK environment */
    g_setenv("GSETTINGS_SCHEMA_DIR",
             "C:\\msys64\\mingw64\\share\\glib-2.0\\schemas", FALSE);
    g_setenv("PATH", "C:\\msys64\\mingw64\\bin", FALSE);

    gtk_init(&argc, &argv);

    load_settings();
    create_main_window();

    /* Start auto-backup timer if enabled */
    if (settings.auto_backup && timer_id == 0) {
        timer_id = g_timeout_add_seconds(settings.backup_interval,
                                         auto_backup_timer, NULL);
    }

    gtk_main();
    return 0;
}
