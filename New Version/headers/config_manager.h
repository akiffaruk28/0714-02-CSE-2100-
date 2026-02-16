#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "backup_settings.h"

void load_settings(void);
void save_settings(void);
void on_settings(GtkWidget *widget, gpointer data);

#endif
