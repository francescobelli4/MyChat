#include "ui.h"
#include "gio/gio.h"
#include "glib-object.h"
#include <gtk/gtk.h>

#include "login.h"

GtkApplication *app;
GtkWindow *window;

GtkWindow *get_app_window() {
	return window;
}

/*
 *	This function setups the main application window, which is a global GtkWindow pointer: it can be accessed from the other files.
 */
int setup_application_window() {

	GtkWidget *window_widget = gtk_application_window_new(app);

	window = GTK_WINDOW(window_widget);

    	gtk_window_set_title(window, "Chat");
    	gtk_window_set_default_size(window, 400, 300);

	return 0;
}


/*
 *	This function is a callback associated to the app's "activate" event. It creates the main window
 *	when the app is actually active
 */
static void on_activate() {

    	
	setup_application_window();
	
	// Now the window is ready to be displayed
	gtk_window_set_application(window, app);
	gtk_window_present(window);

	// The first step is logging in the user. This function automatically opens the app's home page if the user is already logged in (the login file exists),
	// otherwise it will display the login page.
	login_handler();
}

/*
 *	This function creates a new instance for the gtk app and runs the app
 */
int setup_ui() {
	
	int status;
	
	// Creating a new application instance. It also calls gtk_init()...
    	app = gtk_application_new("com.francesco.chat", G_APPLICATION_DEFAULT_FLAGS);

	// Adding a callback (on_activate) to the "activate" signal
    	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

	// Actually running app
    	status = g_application_run(G_APPLICATION(app), 0, NULL);

	// Unreferencing the app instance after it ended...
    	g_object_unref(app);

    	printf("App closed with status: %d\n", status);

    	return status;
}

