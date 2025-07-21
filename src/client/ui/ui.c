#include "ui.h"
#include "gio/gio.h"
#include "glib-object.h"
#include <gtk/gtk.h>

#include "login/ui_login.h"

GtkApplication *app;
GtkWindow *window;

GtkWindow *get_app_window() {
	return window;
}

int setup_application_window() {

	GtkWidget *window_widget = gtk_application_window_new(app);

	window = GTK_WINDOW(window_widget);

    	gtk_window_set_title(window, "Chat");
    	gtk_window_set_default_size(window, 400, 300);

	return 0;
}



static void on_activate() {

    	
	setup_application_window();

	display_login();
	
	gtk_window_set_application(window, app);
	
	gtk_window_present(window);
    	
}

int setup_ui() {

    	app = gtk_application_new("com.francesco.chat", G_APPLICATION_DEFAULT_FLAGS);
    	int status;

    	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    	status = g_application_run(G_APPLICATION(app), 0, NULL);

    	g_object_unref(app);

    	printf("AOOOOOOO\n");

    	return status;
}

