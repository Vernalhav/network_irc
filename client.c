#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define PRINT_LOG 0
#define log_msg(msg) do{ if(PRINT_LOG) puts(msg); }while(0)
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
		exit_error("create_socket: Could not get socket file descriptor");

	log_msg("Socket created successfully!");

	return sockfd;
}


/*
	Binds socket _sockfd_ to _port_.
	Exits if socket binding fails.

	NOTE: port numbers below 1024
		require privileged access.
*/
void bind_socket(int sockfd, int port){
	/*
		For info on how to correctly fill
		this struct (in our case, AF_INET)
		and what each field means, see:
			man 7 ip

		For other address families, check
			man 2 bind
	*/
	struct sockaddr_in server_addr;

	/*
		To have our socket acting as a server,
		we need to designate it an address and a
		port. This is known as binding. The struct
		below contains all the required information
		for the binding to occur.

		AF_INET specifies that we are using IP
		INADDR_ANY specifies that we are using
			any available address
		htons(port) gives us the port number in
			network byte order
	*/
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	/* For possible errors, see man 2 bind */
	int status = bind(sockfd, (struct sockaddr *)&server_addr,\
				 	  (socklen_t)sizeof(server_addr));

	if (status < 0)
		exit_error("bind_socket: Could not bind socket.");

	log_msg("Socket bound successfully!");
}


int main(){

	int socket = create_socket();
	bind_socket(socket, 8888);

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
	return 0;
}