#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <irc_utils.h>


Socket *socket_create(){
	/*
		Creates an TCP/IP socket structure and fills in its FD.
		AF_INET indicates that this socket can be used for
		internet communications. SOCK_STREAM indicates that
		the socket is sequenced, two-way, reliable and
		connection-based (TCP). 0 uses adequade protocol (IP)
		
		For more info, check
			man 2 socket
			man 7 ip
			man 7 tcp
	*/

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		exit_error("socket_create: Could not get socket file descriptor");

	Socket *s = (Socket *)malloc(sizeof(Socket));
	if (s == NULL)
		exit_error("socket_create: Could not allocate socket structure");

	s->sockfd = sockfd;
	s->addr_size = 0;
	memset(&(s->address), 0, sizeof(sockaddr_in));

	console_log("Socket created successfully!");
	return s;
}


void socket_bind(Socket *socket, int port, const char *ip){
	/*
		struct sockaddr_in;
		
		For info on how to correctly fill
		this struct (in our case, AF_INET)
		and what each field means, see:
			man 7 ip

		For other address families, check
			man 2 bind

		To have our socket acting as a server,
		we need to designate it an address and a
		port. This is known as binding. The struct
		below contains all the required information
		for the binding to occur.

		AF_INET specifies that we are using IP
		htons(port) gives us the port number in
			network byte order
	*/

	socket->address.sin_family = AF_INET;
	/* LOOPBACK address points to local machine */
	socket->address.sin_addr.s_addr = inet_addr(ip != NULL ? ip : LOOPBACK);
	socket->address.sin_port = htons(port);

	/* For possible errors, see man 2 bind */
	int status = bind(socket->sockfd, (sockaddr *)&(socket->address),\
				 	  (socklen_t)sizeof(sockaddr_in));

	if (status < 0)
		exit_error("bind_socket: Could not bind socket.");

	socket->addr_size = sizeof(sockaddr_in);
	console_log("Socket bound successfully!");
}


int socket_connect(Socket *socket, int port, const char *ip){
	
	socket->address.sin_family = AF_INET;
	socket->address.sin_addr.s_addr = inet_addr(ip != NULL ? ip : LOOPBACK);
	socket->address.sin_port = htons(port);
	
	int status = connect(socket->sockfd, (struct sockaddr *)&(socket->address),\
			(socklen_t)sizeof(sockaddr_in));

	console_log(status == 0 ? "Socket is connected" : "ERROR CONNECTING");

	return status == 0 ? 1 : -1;
}


void socket_listen(Socket *socket){
	/*
		Sets up socket to receive connections.
		This means other sockets can connect
		to it and send/receive information.

		MAX_BACKLOG defines the maximum number
		of sockets that can be enqueued at one time.

		For more info, check
			man 2 listen
	*/
	listen(socket->sockfd, MAX_BACKLOG);
	console_log("Socket is listening");
}


Socket *create_custom_socket(int sockfd, sockaddr_in addr){
	Socket *socket = (Socket *)malloc(sizeof(Socket));
	if (socket == NULL)
		exit_error("create_custom_socket: Failed to allocate memory");

	socket->sockfd = sockfd;
	socket->addr_size = sizeof(sockaddr_in);
	socket->address = addr;

	return socket;
}


/*
	Accepts a single connection. This function
	blocks the program until a connection is
	available.
	
	Returns a dynamically-allocated structure
	with the connected socket's information
	This structure must be freed later with socket_free.
*/
Socket *socket_accept(Socket *server_socket){
	
	sockaddr_in peer_addr;
	socklen_t addr_size = sizeof(peer_addr);

	console_log("Awaiting connections to accept...");
	int connected_fd = accept(server_socket->sockfd,\
							  (sockaddr *)&peer_addr, &addr_size);


	if (connected_fd < 0)
		exit_error("accept_connection: Invalid connection");

	console_log("Connection established!");

	return create_custom_socket(connected_fd, peer_addr);
}


int min(int a, int b){
	return a < b ? a : b;
}


int socket_receive(Socket *socket, char buffer[], int buffer_size){
	int received_bytes = recv(socket->sockfd, buffer, buffer_size, 0);
	if (received_bytes < 0)
		console_log("socket_receive: Error reading message");
	
	return received_bytes;
}


int socket_send(Socket *socket, const char msg[], int buffer_size){
	int msg_len = min(strlen(msg), buffer_size);

	int sent_bytes = 0, error;
	while (sent_bytes < msg_len){
		error = send(socket->sockfd, msg + sent_bytes,\
						   buffer_size - sent_bytes, MSG_NOSIGNAL);
		
		if (error < 0) return -1;
		sent_bytes += error;
	}

	return 1;
}


int socket_shutdown(Socket *socket, int how){
	return shutdown(socket->sockfd, how);
}


void socket_free(Socket *socket){
	close(socket->sockfd);
	free(socket);
}