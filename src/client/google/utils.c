#include "utils.h"
#include <pthread.h>
#include <threads.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <json-c/json.h>
#include <unistd.h>

#include "../../shared/shared.h"

pthread_t http_server;

// This mutex is used to sync the main thread and this one. When the login is finished, the main thread has to duplicate the email string, so it does not get lost.
// This mutex gives time to do that.
pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 *	This function performs an HTTPS GET REQUEST to google to retreive the user's email
 */
char *get_user_email(char *access_token) {

	struct hostent *host = gethostbyname("www.googleapis.com");

	// Connecting to google oauth
	struct sockaddr_in server_address;
	server_address.sin_port = htons(HTTPS_PORT);
	server_address.sin_family = AF_INET;
	memcpy(&server_address.sin_addr, host->h_addr, 4);

		
	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == -1) {
		perror("failed initializing access token server socket");
		return NULL;
	}

	if (connect(server, (struct sockaddr *)&server_address, sizeof(server_address))) {
		perror("failed connecting to access token server");
		return NULL;
	}

	// Initializing SSL for HTTPS communication
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

	SSL *ssl = SSL_new(ctx);
	SSL_set_fd(ssl, server);
	SSL_connect(ssl);

	
	// Specifing the format of the request
	char *request_format = 
		"GET /oauth2/v2/userinfo HTTP/1.1\r\n"
		"Host: www.googleapis.com\r\n"
		"Authorization: Bearer %s\r\n"
		"Connection: close\r\n"
		"\r\n";
	
	size_t req_len = strlen(request_format) + strlen(access_token) - 1;

	char *request = malloc(req_len);
	memset(request, 0, req_len);
	sprintf(request, request_format, access_token);

	// Sending the request
	int ret = SSL_write(ssl, request, strlen(request));
	if (ret < strlen(request)) {
		perror("failed sending POST request to access tokens server");
		return NULL;
	}

	// Reading the answer
	char *buffer = malloc(4097);
	memset(buffer, 0, 4097);

	int offset = 0;
	while (offset < 4096) {
		
		int ret = SSL_read(ssl, buffer + offset, 4096 - offset);
		offset += ret;

		if (ret == -1) {
			perror("failed receiving access token server response");
			return NULL;
		}

		if (ret == 0) 
			break;

		if (strstr(buffer, "\r\n0\r\n\r\n") != NULL)
			break;
	}

	// Parsing the access token
	char *json_start = strstr(buffer, "{");

	struct json_object *parsed;
	struct json_object *parsed_email;
	char *email;

	parsed = json_tokener_parse(json_start);

	json_object_object_get_ex(parsed, "email", &parsed_email);

	email = strdup(json_object_get_string(parsed_email));

	// Freeing memory
	json_object_put(parsed);

	free(request);
	free(buffer);

	SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN);
   	SSL_shutdown(ssl);
    	SSL_free(ssl);
    	SSL_CTX_free(ctx);

	close(server);

	return email;
}



/*
 *	This function performs an HTTPS POST REQUEST to google to retreive the access token needed to retreive the user's email
 *	The HTTPS protocol is implemented using OPENSSL.
 *
 *	Google's oauth2 answer is sent in "chunked" mode, so I will need more "reads" to retrieive the full message. The esiest
 *	method to reach the end of the whole message, is reading until I find "\r\n0\r\n\r\n", which means that the message is over.
 */
char *get_access_token(char *authorization_code) {

	char *client_secret = getenv("GOOGLE_CLIENT_SECRET");
	struct hostent *host = gethostbyname("oauth2.googleapis.com");

	// Connecting to google oauth
	struct sockaddr_in server_address;
	server_address.sin_port = htons(HTTPS_PORT);
	server_address.sin_family = AF_INET;
	memcpy(&server_address.sin_addr, host->h_addr, 4);

		
	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == -1) {
		perror("failed initializing access token server socket");
		return NULL;
	}

	if (connect(server, (struct sockaddr *)&server_address, sizeof(server_address))) {
		perror("failed connecting to access token server");
		return NULL;
	}

	// Initializing SSL for HTTPS communication
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

	SSL *ssl = SSL_new(ctx);
	SSL_set_fd(ssl, server);
	SSL_connect(ssl);

	// Specifing the format of the request's body
	char *body_format = 
		"code=%s&"
		"client_id=%s&"
		"client_secret=%s&"
		"redirect_uri=http://localhost:8080/&"
		"grant_type=authorization_code";

	size_t body_len = strlen(body_format) + strlen(authorization_code) - 2 + strlen(client_secret) - 2 + strlen(GOOGLE_CLIENT_ID) - 2;

	char *body = malloc(body_len);
	sprintf(body, body_format, authorization_code, GOOGLE_CLIENT_ID, client_secret);
	
	// Specifing the format of the request
	char *request_format = 
		"POST /token HTTP/1.1\r\n"
		"Host: oauth2.googleapis.com\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: %lu\r\n"
		"\r\n"
		"%s";
	
	// Con il log_10(body_len) + 1 so quante cifre ha body_len!
	size_t req_len = strlen(request_format) + (log10(body_len) + 1) - 3 + body_len;

	char *request = malloc(req_len);
	memset(request, 0, req_len);
	sprintf(request, request_format, body_len, body);

	// Sending the request
	int ret = SSL_write(ssl, request, strlen(request));
	if (ret < strlen(request)) {
		perror("failed sending POST request to access tokens server");
		return NULL;
	}

	// Reading the answer
	char *buffer = malloc(8193);
	memset(buffer, 0, 8193);

	int offset = 0;
	while (offset < 8192) {
		
		int ret = SSL_read(ssl, buffer + offset, 8192 - offset);
		offset += ret;

		if (ret == -1) {
			perror("failed receiving access token server response");
			return NULL;
		}

		if (ret == 0) 
			break;

		if (strstr(buffer, "\r\n0\r\n\r\n") != NULL)
			break;
	}

	// Parsing the access token
	char *json_start = strstr(buffer, "{");

	struct json_object *parsed;
	struct json_object *parsed_access_token;

	parsed = json_tokener_parse(json_start);

	json_object_object_get_ex(parsed, "access_token", &parsed_access_token);
	char *access_token = strdup(json_object_get_string(parsed_access_token));

	// Freeing memory
	json_object_put(parsed);

	free(request);
	free(body);
	free(buffer);

	SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN);
   	SSL_shutdown(ssl);
    	SSL_free(ssl);
    	SSL_CTX_free(ctx);

	close(server);

	return access_token;
}



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
 *	This function performs a request to google oauth2 to retreive the authorization code. It setups a socket based on the redirect_uri specified 
 *	in google_button_clicked: google sends the authorization to that redirect_uri and this function catches the code.
 */
char *get_authorization_code() {
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(8080);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == -1) {

		perror("failed initializing authorization server socket");
		return NULL;
	}

	// Allowing to immediately re-use the address: otherwise the app will crash in the next launch because the kernel can take some seconds to close the socket
	int opt = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address))) {

		perror("failed binding authorization server socket");
		return NULL;
	}
		

	if (listen(server_socket, 1)) {

		perror("failed listening on authorization server socket");
		return NULL;
	}
	

	char *buffer = malloc(4096);
	if (buffer == NULL) {
		
		perror("failed initializing authorization response buffer");
		return NULL;
	}

	struct sockaddr client_address;
	int client_addrlen;

	int client = accept(server_socket, (struct sockaddr *) &client_address, (socklen_t *) &client_addrlen);
	if (client == -1) {

		perror("failed accepting on authorization server socket");
		return NULL;
	}


	int offset = 0;
	int temp = 0;

	while (offset < 4096) {
		temp = read(client, buffer + offset, 4096 - offset);

		// Termine 
		if (strstr(buffer, "\r\n\r\n"))
			break;

		if (temp == -1) {

			perror("failed reading from authorization server socket");
			return NULL;
		}

		offset += temp;
	}


	// The received code has to be parsed from the server's response (which is located in buffer). This code is encoded using percentage encoding, so
	// it also has to be decoded.
	char *authorization_code = get_decoded_code_from_response(buffer);

	close(server_socket);
	close(client);
	free(buffer);

	return authorization_code;
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
void *perform_google_login_communication(void *callback) {

	gboolean (*access_complete_callback)(gpointer) = *((gboolean (**) (gpointer)) callback);

	char *authorization_code = get_authorization_code();
	printf("Received authorization code: [%s]\n", authorization_code);

	char *access_token = get_access_token(authorization_code);
	printf("Received access token: [%s]\n", access_token);

	char *email = get_user_email(access_token);
	printf("Received email: [%s]\n", email);

	pthread_mutex_init(&sync_mutex, NULL);
	pthread_mutex_lock(&sync_mutex);

	g_idle_add(access_complete_callback, email);

	pthread_mutex_lock(&sync_mutex);
	
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
void google_button_clicked(GtkWidget *widget, gpointer user_data) {
	
	GoogleLoginButtonArg *arg = (GoogleLoginButtonArg *) user_data;

	const char *url_format = "https://accounts.google.com/o/oauth2/v2/auth?"
		"access_type=offline&"
		"scope=openid%%20email%%20profile&"
		"include_granted_scopes=true&"
		"response_type=code&"
		"redirect_uri=http://localhost:8080/&"
		"client_id=%s";

	// Building the command (xdg-open https://acc.......)
	char *url = malloc(strlen(url_format) + strlen(GOOGLE_CLIENT_ID) + 1);
	snprintf(url, strlen(url_format) + strlen(GOOGLE_CLIENT_ID), url_format, GOOGLE_CLIENT_ID);
	char *command = malloc(strlen(url) + strlen("xdg-open '%s'"));
	snprintf(command, strlen(url) + strlen("xdg-open '%s'") + 1, "xdg-open '%s'", url);

	system(command);

	GtkStack *stack = GTK_STACK(gtk_builder_get_object(arg->builder, "login_mode_stack"));
	// Telling the user to check his browser
	gtk_stack_set_visible_child_name(stack, "waiting_for_google_page");

	// This thread will start an http local server. This server will catch google's response after the user accepts
	// to login using his google account.
	// It has to run on a separate thread because the main thread (which runs GTK) can't be blocked, otherwise the 
	// GTK app will crash.
	pthread_create(&http_server, NULL, perform_google_login_communication, (void *)&(arg->callback));

	//pthread_detach(http_server);
}

