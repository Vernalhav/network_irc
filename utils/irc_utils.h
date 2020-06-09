#ifndef IRC_UTILS_H
#define IRC_UTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PRINT_LOG 1
#define console_log(s, args...) do{ if(PRINT_LOG) printf(s "\n", ##args); }while(0)
#define exit_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)

#define MAX_MSG_LEN 4096
#define MAX_NAME_LEN 50
#define MAX_CHANNEL_LEN 200
#define WHOLE_MSG_LEN MAX_MSG_LEN + MAX_NAME_LEN + MAX_CHANNEL_LEN + 16

#define LOOPBACK "127.0.0.1"

#define SERVER_PORT 8888
#define SERVER_ADDR "127.0.0.1"
#define MAX_BACKLOG 6


typedef struct socket_{
	int sockfd;
	socklen_t addr_size;
	struct sockaddr_in address;
} Socket;


/*
	Creates TCP/IP socket. Its address is
	NULL until a call to socket_bind is made.
	Socket must be freed by the user with a
	call to socket_free.

	Exits if socket allocation fails.
*/
Socket *socket_create();


/*
	Binds socket tp port.
	Exits if socket binding fails.
	This function should be called
	only if the socket address size is 0,
	after which it will become filled in,
	as well as the address structure.

	port: 	port number to assign the socket
	ip:		string with the IP address to connect to
			(leave as NULL to connect to local machine)

	NOTE: port numbers below 1024
		  require privileged access.
*/
void socket_bind(Socket *socket, int port, const char *ip);


/*
	Connects socket to port/IP.
	Returns 1 on success and -1 on failure.
*/
int socket_connect(Socket *socket, int port, const char *ip);


/*
	Configures socket to listen
	to incoming connections
*/
void socket_listen(Socket *socket);


/*
	Accepts a single connection.
	
	Returns a dynamically-allocated structure
	with the connected socket's information
	This structure must be freed later with socket_free.

	NOTE: this function blocks the thread until
		  a connection is available.
*/
Socket *socket_accept(Socket *server_socket);


/*
	Fills buffer with messages sent by
	the client socket.
	Returns number of bytes read, and -1
	if failed.

	NOTE: This function blocks the thread until
		  a message is available.
*/
int socket_receive(Socket *socket, char buffer[], int buffer_size);


/*
	Sends \0-terminated message to the socket.

	This function guarantees that the whole
	message will be sent, so long as it ends
	with \0. That is, it will send strlen(msg)
	bytes OR buffer_size bytes, whichever is
	lesser.

	Returns 1 on success and -1 on failure.

	NOTE: if the socket's file descriptor is closed,
		  the SIGPIPE signal will be ignored
*/
int socket_send(Socket *socket, const char msg[], int buffer_size);


/*
	Fills ipv4 buffer with the IPv4 address of socket
*/
void socket_ip(Socket *socket, char ipv4[32]);


/*
	Shuts down part of a full duplex connection.
	how can be either SHUT_RD, SHUT_WR or SHUT_RDWR
*/
int socket_shutdown(Socket *socket, int how);


/*
	Closes socket's file descriptor and
	frees dynamic memory.
*/
void socket_free(Socket *socket);


#endif