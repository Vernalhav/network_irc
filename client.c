#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <irc_utils.h>


int main(){

	Socket *connected_socket;

	Socket *socket = socket_create();
	socket_bind(socket, 8888);
	socket_listen(socket);

	while (1){
		connected_socket = socket_accept(socket);
		socket_free(connected_socket);
	}
	
	socket_free(socket);

	/*
		Helpful man pages to implement
		remaining features.

		man 2 listen
		man 2 connect
		man 2 bind
		man 2 send
		man 2 recv
	*/

	return 0;
}