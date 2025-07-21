#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../shared/shared.h"
#include "clients_list.h"

ClientsList *clients_list;

void *client_routine(void *arg) {
	
	printf("Client connected!");
	sleep(1);

	return EXIT_SUCCESS;
}

int setup_server_socket() {

	struct sockaddr_in server_address;

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_socket == -1)
		return -1;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)))
		return -1;

	if (listen(server_socket, BACKLOG)) 
		return -1;

	printf("Server is online and listening on port %d!\n", PORT);
		
	clients_list = initialize_clients_list();
	if (clients_list == NULL)
		return -1;
	
	struct sockaddr client_address;
	int client_addrlen;

	while (1) {
		
		int client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen);

		if (client_socket == -1)
			continue;

		ClientNode *client_node = create_client_node(client_socket);
		if (client_node == NULL)
			continue;

		if (add_client(clients_list, client_node)) {
			
			close(client_socket);
			free(client_node);
			continue;
		}

		if (pthread_create(&(client_node->tid), NULL, client_routine, (void *)client_node)) {
			close(client_socket);
			remove_client(clients_list, client_node);
			continue;
		}

		pthread_detach(client_node->tid);
	}
}

int main(int argc, char **argv)
{
	
	setup_server_socket();

	return EXIT_SUCCESS;
}
