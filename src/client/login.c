#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "login.h"
#include "../shared/shared.h"
#include "glib.h"
#include "ui.h"

GtkBuilder *builder;
pthread_t http_server;


/*
 *	This function decodes the percentage encoding.
 */
char *decode_code (char *encoded) {

	int encoded_len = strlen(encoded);
	int decoded_len = encoded_len;
	char *temp = encoded;
	char *decoded = malloc(decoded_len);
	memset(decoded, 0, decoded_len);
	int offset = 0;
	int decoded_offset = 0;

	while ((temp = strchr(temp, '%')) != NULL) {

		int copying_bytes = temp - (encoded + offset);

		memcpy(decoded + decoded_offset, encoded + offset, copying_bytes);

		decoded_offset += copying_bytes;
		offset += (copying_bytes + 3);

		char *encoded_char = malloc(3);
		memset(encoded_char, 0, 3);

		memcpy(encoded_char, temp + 1, 2);
		char decoded_char = (char) strtol(encoded_char, NULL, 16);

		memcpy(decoded + decoded_offset, &decoded_char, 1);
		decoded_offset++;

		temp+= 3;
	}

	memcpy(decoded + decoded_offset, encoded + offset, encoded_len - offset);

	free(temp);

	return decoded;
}

/*
 *	This function parses the encoded "code" from the server's response. This code will be decoded.
 */
char *get_decoded_code_from_response (char *response) {

	char *code;
	
	char *start = strstr(response, "code=");
	start += strlen("code=");

	char *end = strchr(start, '&');

	int code_len = end - start;

	code = malloc(code_len + 1);
	memset(code, 0, code_len + 1);
	memcpy(code, start, code_len);

	char *decoded = decode_code(code);

	free(code);

	return decoded;
}

/*
 *	This function is an async function which is called from the http_server thread when the authorization code is ready.
 *	This function runs on the main thread (the GTK one). It continues the logging in.
 *
 *	This function should now use the authorization_code to send an HTTPS post request to google oauth2, to receive an access 
 *	token, that will be used to send another post request and receive the user's data.
 */
gboolean on_code_received(gpointer data) {
    
    printf("Codice ricevuto: %s\n", (char *) data);

    //gtk_stack_set_visible_child_name(stack, "login");

    return FALSE;  // Esegui solo una volta
}

/*
 *	This function creates a server socket which will catch google's response when the user accepts to login using Google.
 *	The server's port has to match with the one specified in url_format.
 *
 *	Note that this thread will call g_idle_add to continue the login using the on_code_received async function, which 
 *	receives the authorization_code as param.
 *	In this way, the main thread continues the user's login without crashing.
 *
 *	ALSO CHECK google_button_clicked function to understand why a separate thread is necessary!
 */
void *get_authorization_code(void *arg) {

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(8081);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_socket == -1)
		return NULL;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)))
		return NULL;

	if (listen(server_socket, 1))
		return NULL;

	char *buffer = malloc(4096);

	struct sockaddr client_address;
	int client_addrlen;

	int client = accept(server_socket, (struct sockaddr *) &client_address, (socklen_t *) &client_addrlen);
	if (client == -1)
		return NULL;

	int offset = 0;
	int temp = 0;

	while (offset < 4096) {
		temp = read(client, buffer + offset, 4096 - offset);

		if (temp == 0)
			break;

		if (temp == -1)
			return NULL;

		offset += temp;
	}

	// The received code has to be parsed from the server's response (which is located in buffer). This code is encoded using percentage encoding, so 
	// it also has to be decoded.
	char *authorization_code = get_decoded_code_from_response(buffer);

	close(server_socket);
	close(client);

	g_idle_add(on_code_received, (gpointer *) authorization_code);

	return NULL;
}

/*
 * 	https://developers.google.com/identity/protocols/oauth2/web-server?hl=IPPROTO_TCP
 *
 *	When the "Sign in with Google" button is clicked, the client has to connect to google oauth2 with
 *	the url_format format: the uset's web browser is opened with that url using xdg-utils.
 *
 *	Note that this function can't "wait" for the authorization code, otherwise the GTK app will crash.
 *	The login will continue with on_code_received async function.
 */
void google_button_clicked() {

	const char *url_format = "https://accounts.google.com/o/oauth2/v2/auth?"
		"access_type=offline&"
		"scope=openid%%20email%%20profile&"
		"include_granted_scopes=true&"
		"response_type=code&"
		"redirect_uri=http://localhost:8081/&"
		"client_id=%s";

	// Building the command (xdg-open https://acc.......)
	char *url = malloc(strlen(url_format) + strlen(GOOGLE_CLIENT_ID) + 1);
	snprintf(url, strlen(url_format) + strlen(GOOGLE_CLIENT_ID), url_format, GOOGLE_CLIENT_ID);
	char *command = malloc(strlen(url) + strlen("xdg-open '%s'"));
	snprintf(command, strlen(url) + strlen("xdg-open '%s'") + 1, "xdg-open '%s'", url);

	system(command);

	GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "login_mode_stack"));
	// Telling the user to check his browser
	gtk_stack_set_visible_child_name(stack, "waiting_for_google_page");

	// This thread will start an http local server. This server will catch google's response after the user accepts
	// to login using his google account.
	// It has to run on a separate thread because the main thread (which runs GTK) can't be blocked, otherwise the 
	// GTK app will crash.
	pthread_create(&http_server, NULL, get_authorization_code, NULL);
}

/*
 *	This function setups the necessary callbacks in the login_page stack page.
 */
void setup_login_mode() {

	GtkButton *google_login = GTK_BUTTON(gtk_builder_get_object(builder, "login_mode_google"));
	g_signal_connect(google_login, "clicked", G_CALLBACK(google_button_clicked), NULL);
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
