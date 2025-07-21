#ifndef CLIENTS_LIST
#define CLIENTS_LIST

#include <stdint.h>
#include <pthread.h>

#define MAX_CLIENTS 100

typedef struct client_node {
	
	int socket;
	pthread_t tid;
	struct client_node *next;
} ClientNode;

typedef struct clients_list {
	
	int n_clients;
	ClientNode *head;
} ClientsList;

ClientsList* initialize_clients_list();
ClientNode *create_client_node(int socket);
int add_client(ClientsList *list, ClientNode *new_client);
int remove_client(ClientsList *list, ClientNode *client);

#endif
