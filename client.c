#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define N_THREADS 2
#define QUIT_CMD "/quit"


/* Args must be a single Socket pointer */
void *receive_messages(void *args){
	/*
		Receives messages and prints
		them on the screen
	*/

	Socket *socket = (Socket *)args;
	char buffer[MAX_MSG_LEN + MAX_NAME_LEN + 16] = {0};
	char msg_sender[MAX_NAME_LEN + 1] = {0};
	char msg[MAX_MSG_LEN + 1] = {0};

	console_log("Receiving messages...");

	int received_bytes;
	while (strcmp(msg, QUIT_CMD)){
		received_bytes = socket_receive(socket, buffer);
		sscanf(buffer, "<%[^>]%*c %[^\n]%*c", msg_sender, msg);

		if (received_bytes <= 0) break;
		/* TODO: Check if message is overflowing the buffer! */
		printf("<%s> %s\n", msg_sender, msg);
	}

	return NULL;
}

/* Args must be a single Socket pointer */
void *send_messages(void *args){
	
	Socket *socket = (Socket *)args;
	char msg[MAX_MSG_LEN] = {0};

	console_log("Sending messages...");

	int status = 1;
	while (strcmp(msg, QUIT_CMD) && status > 0){
		scanf("%[^\n]%*c", msg);
		status = socket_send(socket, msg);
	}

	return NULL;
}


int main(){

	Socket *socket = socket_create();
	socket_connect(socket, SERVER_PORT, SERVER_ADDR);

	pthread_t threads[N_THREADS];

	pthread_create(threads + 0, NULL, send_messages, (void *)socket);
	pthread_create(threads + 1, NULL, receive_messages, (void *)socket);

	int i;
	for (i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);

	socket_free(socket);
	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
