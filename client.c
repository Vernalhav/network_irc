#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <irc_utils.h>


int main(){

	Socket *connected_socket;

	Socket *socket = socket_create();
	socket_bind(socket, 8888, "127.0.0.1");
	socket_listen(socket);

	connected_socket = socket_accept(socket);

	char msg[4096] = {0};
	while (strcmp(msg, "/quit")){
		console_log("Waiting for message...");
		socket_receive(connected_socket->sockfd, msg);
		puts(msg);
	}
	
	socket_free(connected_socket);
	socket_free(socket);

	/*
		Super helpful reference:
		http://beej.us/guide/bgnet/html/

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