#include "clients_list.h"

#include <stdint.h>
#include <stdlib.h>

ClientsList* initialize_clients_list() {
	
	ClientsList *clients_list = malloc(sizeof(ClientsList));

	if (clients_list == NULL)
		return NULL;

	clients_list->n_clients = 0;
	clients_list->head = 0;

	return clients_list;
}

ClientNode* create_client_node(int socket) {

	ClientNode *client_node = malloc(sizeof(ClientNode));

	if (client_node == NULL)
		return NULL;

	client_node->socket = socket;
	client_node->next = NULL;

	return client_node;
}

int add_client(ClientsList *list, ClientNode *new_client) {

	if (list->n_clients >= MAX_CLIENTS)
		return -1;

	if (list->head == NULL) {
		list->head = new_client;
		list->n_clients++;
		return 0;
	}

	ClientNode *temp = list->head;

	while (temp->next != NULL) {
		temp = temp->next;
	}

	temp->next = new_client;
	list->n_clients++;

	return 0;
}

int remove_client(ClientsList *list, ClientNode *client) {

	if (list->n_clients == 0)
		return -1;

	ClientNode *temp = list->head;

	if (temp->socket == client->socket) {
		list->head = list->head->next;
		free(temp);
		list->n_clients--;
		return 0;
	}

	while(temp->next != NULL) {
	
		if (temp->next->socket == client->socket) {
			ClientNode *next_node = temp->next->next;
			free(temp->next);
			temp->next = next_node;
			list->n_clients--;
			return 0;
		}

		temp = temp->next;
	}

	return -1;
}
