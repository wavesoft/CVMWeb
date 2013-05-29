/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

#include <string>
#include <gtk/gtk.h>
#include "../Dialogs.h"

typedef struct {
    int result;
    const char * text;
} DialogData;

static gboolean display_dialog( gpointer user_data ) {
    DialogData *dialog_data = (DialogData*) user_data;
    GtkWidget *dialog;
    
    /* Fetch window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    /* Create dialog */
    dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), 
        GTK_DIALOG_DESTROY_WITH_PARENT, 
        GTK_MESSAGE_QUESTION, 
        GTK_BUTTONS_YES_NO, 
        dialog_data->text);

    /* Set caption */
    gtk_window_set_keep_above(GTK_WINDOW(dialog), true);
    gtk_window_set_title(GTK_WINDOW(dialog), "CernVM Web API");
    dialog_data->result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_main_quit();  // Quits the main loop run in MessageBox()
    gtk_widget_destroy(GTK_WIDGET(dialog));

    return FALSE;
}

bool CVMConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message) {
    
    // Initialize GTK and threads
    if (!XInitThreads()) {
        std::cerr << "[Dialogs] XInitThreads Failed" << std::endl;
        return false;
    }
    if (!gtk_init_check(NULL, NULL)) {
        std::cerr << "[Dialogs] gtk_init_check failed" << std::endl;
        return false;
    }
    
    DialogData dialog_data;
    dialog_data.text = (const char * ) message.c_str();
    g_idle_add(display_dialog, &dialog_data);
    gtk_main();
    return (dialog_data.result == GTK_RESPONSE_YES);
}
