#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define PORT 8888
#define ADDR "127.0.0.1"
#define N_THREADS 2

#define QUIT_CMF "/quit"

/* Args must be a single Socket pointer */
void *receive_messages(void *args){
	/*
		Receives messages and prints
		them on the screen
	*/

	Socket *connected_socket = (Socket *)args;
	char msg[MAX_MSG_LEN] = {0};

	int status = 1;
	while (strcmp(msg, QUIT_CMF) && status >= 0){
		status = socket_receive(connected_socket, msg);
		puts(msg);
	}

	return NULL;
}

/* Args must be a single Socket pointer */
void *send_messages(void *args){
	
	Socket *socket = (Socket *)args;
	char msg[MAX_MSG_LEN] = {0};

	int status = 1;
	while (strcmp(msg, QUIT_CMF) && status >= 0){
		scanf("%[^\n]%*c", msg);
		status = socket_send(socket, msg);
	}

	return NULL;
}


int main(){

	int listen_port, connected_port;
	printf("Select port for this application to listen to: ");
	scanf("%d", &listen_port);

	printf("Select port for this application to connect to: ");
	scanf("%d", &connected_port);

	int accept_first;
	printf("Is this client going to accept connection first?: ");
	scanf("%d", &accept_first);

	Socket *socket = socket_create();
	socket_bind(socket, listen_port, "127.0.0.1");
	socket_listen(socket);

	Socket *connected_socket;
	if (accept_first){
		connected_socket = socket_accept(socket);
		socket_connect(socket, connected_port, "127.0.0.1");		
	} else {
		socket_connect(socket, connected_port, "127.0.0.1");
		connected_socket = socket_accept(socket);
	}

	void *(*THR_FUNCTIONS[N_THREADS])(void *) = {
		receive_messages, send_messages
	};

	void *THR_ARGS[N_THREADS] = {
		connected_socket, socket
	};

	pthread_t threads[N_THREADS];

	console_log("THREADS DISPATCHED!!");

	int i;
	for (i = 0; i < N_THREADS; i++)
		pthread_create(threads + i, NULL, THR_FUNCTIONS[i], THR_ARGS[i]);

	
	/* Waits for all threads to exit */
	for (i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);
	
	socket_free(socket);
	socket_free(connected_socket);

	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
