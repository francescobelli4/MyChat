#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <json-c/json.h>

#include "google/utils.h"

#include "login.h"
#include "glib.h"
#include "ui.h"

GtkBuilder *builder;




/*
 *	This function is an async function which is called from the http_server thread when the user's email is ready.
 *	This function runs on the main thread (the GTK one). It continues the logging in.
 *
 *	This function should now make the user set a password if server never saved a password for this email. Otherwise, just log in.
 */
gboolean on_google_authentication_complete(gpointer data) {
	
	char *email = (char *) data;
	printf("Email: %s\n", email);
    
	GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "login_mode_stack"));

	// if emailnotfound
    	//	gtk_stack_set_visible_child_name(stack, "google_login_success");
	// else
	//	login_complete

    	return FALSE;  
}




/*
 *	This function setups the necessary callbacks in the login_page stack page.
 */
void setup_login_mode() {

	GtkButton *google_login = GTK_BUTTON(gtk_builder_get_object(builder, "login_mode_google"));
	GoogleLoginButtonArg *arg = malloc(sizeof(GoogleLoginButtonArg));
	arg->callback = on_google_authentication_complete;
	arg->builder = builder;
	g_signal_connect(google_login, "clicked", G_CALLBACK(google_button_clicked), arg);
}

void setup_register_mode() {

}

/**
 *	This function is a callback for the "clicked" event of the two buttons at the top of the form.
 *	arg is "login" or "register"
 */
void login_mode_btn_clicked(GtkButton *button, gpointer arg) {
	char *data = (char *) arg;

	// Setting the stack's displaying child
	GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "login_mode_stack"));

	if (strcmp(data, "login") == 0) {
		gtk_stack_set_visible_child_name(stack, "login_page");
		setup_login_mode();
	} else {
		gtk_stack_set_visible_child_name(stack, "register_page");
		setup_register_mode();
	}
}

/*
 *	This function shows the login page if the login file is not found.
 *	(GTK stuff in there... mainly buttons callbacks and UI stuff)
 */
void display_login() {
	
	// Loading icons :D
	GError *error = NULL;
	GResource *resource = g_resource_load("ui/Login/icons.gresource", &error);
	if (!resource) {
    		g_printerr("Failed to load gresource: %s\n", error->message);
    		g_error_free(error);
    		return;
	}

	g_resources_register(resource);


	// Using the builder, an XML file can be parsed to get widgets defined in that XML file
	builder = gtk_builder_new_from_file("ui/Login/main.ui");

	// Getting the main widget of the page ("background")
	GtkWidget *content = GTK_WIDGET(gtk_builder_get_object(builder, "background"));

	// Setting the main widget as the displaying page
	gtk_window_set_child(get_app_window(), content);

	// The dafault mode is the login one
	setup_login_mode();

	// Setting up click callbacks
	GtkButton *login_mode_btn = GTK_BUTTON(gtk_builder_get_object(builder, "login_mode_btn"));
	GtkButton *register_mode_btn = GTK_BUTTON(gtk_builder_get_object(builder, "register_mode_btn"));
	g_signal_connect(login_mode_btn, "clicked", G_CALLBACK(login_mode_btn_clicked), (gpointer) "login");
	g_signal_connect(register_mode_btn, "clicked", G_CALLBACK(login_mode_btn_clicked), (gpointer) "register");

	// Setting CSS
	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(provider, "ui/Login/main.css");

	gtk_style_context_add_provider_for_display(
    		gdk_display_get_default(),
    		GTK_STYLE_PROVIDER(provider),
    		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(provider);
}

/*
 *	This function should check if the login file exists. The defined macros will be the return value of this function.
 */
int get_login_file() {
	
	int login_fd = open("user_login.json", O_EXCL|O_CREAT|O_RDWR, 0666);

	if (login_fd == -1) {
		
		if (errno == EEXIST) {
			return LOGIN_FILE_DOES_NOT_EXIST;
		}

		return LOGIN_ERROR;
	}

	return login_fd;
}


/*
 *	This function manages the login phase. It checks if a login file exists: if so, it will automatically login the user, without displaying the login page.
 *	Otherwise, it will display the login page to let the user choose the authentication method.
 */
int login_handler() {
	
	int login_fd = get_login_file();

	if (login_fd == LOGIN_ERROR)
		return LOGIN_ERROR;

	if (login_fd == LOGIN_FILE_DOES_NOT_EXIST) {

		// Registra
		display_login();
	} else {

		// Login automatico
	}
}
