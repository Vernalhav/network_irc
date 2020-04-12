#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define exit_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)


/*
	Returns the file descriptor of an IPv4 stream-socket. 
	Exits if socket allocation fails.
*/
int create_socket(){
	/*
	Returns the file descriptor of an IPv4 stream-socket.
	AF_INET indicates that this socket can be used for
	internet communications. SOCK_STREAM indicates that
	the socket is sequenced, two-way, reliable and
	connection-based (TCP). 0 uses adequade protocol (IPv4)
	
	For more info, check
		man 2 socket
		man 7 ip
		man 7 tcp
	*/

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		exit_error("Could not get socket file descriptor");

	return sockfd;
}


int main(){

	int socket = create_socket();

	/*
		Helpful man pages to implement
		remaining features.

		man 2 listen
		man 2 connect
		man 2 bind
		man 2 send
		man 2 recv
	*/

	close(socket);
}