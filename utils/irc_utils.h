#ifndef IRC_UTILS_H
#define IRC_UTILS_H

#define PRINT_LOG 1
#define console_log(msg) do{ if(PRINT_LOG) puts(msg); }while(0)
#define exit_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)

#define MAX_MSG_LEN 4096
#define MAX_BACKLOG 2
#define LOOPBACK "127.0.0.1"

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct socket_{
	int sockfd;
	socklen_t addr_size;
	sockaddr_in address;
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
	Configures socket to lsten
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

	NOTE: This function blocks the thread until
		  a message is available.
*/
void socket_receive(Socket *client_socket, char buffer[MAX_MSG_LEN]);

/*
	Sends \0-terminated message to the socket.

	This function guarantees that the whole
	message will be sent, so long as it ends
	with \0.
*/
void socket_send(Socket *socket, const char msg[MAX_MSG_LEN]);

/*
	Closes socket's file descriptor and
	frees dynamic memory.
*/
void socket_free(Socket *socket);

#endif