#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "../shared/shared.h"
#include "ui/ui.h"

int main(int argc, char **argv)
{

	setup_ui();

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = inet_addr(argv[1]);
	

	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    	if (server == -1)
        	perror("failed initializing server socket");
	
    	if (connect(server, (struct sockaddr *)&server_address, sizeof(server_address))) 
        	perror("failed connecting to server!");

	return EXIT_SUCCESS;
}
