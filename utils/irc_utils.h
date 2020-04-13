#ifndef IRC_UTILS_H
#define IRC_UTILS_H

#define PRINT_LOG 1
#define console_log(msg) do{ if(PRINT_LOG) puts(msg); }while(0)
#define exit_error(msg) do{ perror(msg); exit(EXIT_FAILURE); }while(0)

#define MAX_BACKLOG 2

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

	NOTE: port numbers below 1024
		require privileged access.
*/
void socket_bind(Socket *socket, int port);

/*
	Configures socket to lsten
	to incoming connections
*/
void socket_listen(Socket *socket);

/*
	Accepts a single connection. This function
	blocks the program until a connection is
	available.
	
	Returns a dynamically-allocated structure
	with the connected socket's information
	This structure must be freed later with socket_free.
*/
Socket *socket_accept(Socket *server_socket);

/*
	Closes socket's file descriptor and
	frees dynamic memory.
*/
void socket_free(Socket *socket);

#endif