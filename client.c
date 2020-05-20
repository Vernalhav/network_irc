#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define MAX_MSG_FACTOR 5
#define BUFFER_LEN MAX_MSG_LEN*MAX_MSG_FACTOR + 16

#define N_THREADS 2
#define QUIT_CMD "/quit"


void parse_message(char *buffer, char *msg_sender, char *msg){

	memset(msg_sender, 0, (MAX_NAME_LEN + 1)*sizeof(char));
	memset(msg, 0, (MAX_MSG_LEN + 1)*sizeof(char));

	if (buffer[0] != '<'){
		strcpy(msg_sender, "?");
		strncpy(msg, buffer, MAX_MSG_LEN + 1);
		return;
	}

	int i;
	for (i = 1; i < MAX_MSG_LEN && buffer[i] != '>'; i++){
		msg_sender[i - 1] = buffer[i];
	}
	msg_sender[i-1] = '\0';

	strncpy(msg, buffer + i + 2, MAX_MSG_LEN);
}


/* Args must be a single Socket pointer */
void *receive_messages(void *args){
	/*
		Receives messages and prints
		them on the screen
	*/

	Socket *socket = (Socket *)args;
	char buffer[WHOLE_MSG_LEN];
	char msg_sender[MAX_NAME_LEN + 1];
	char msg[MAX_MSG_LEN + 1];

	console_log("Receiving messages...");

	int received_bytes;
	while (strcmp(msg, QUIT_CMD)){

		received_bytes = socket_receive(socket, buffer, WHOLE_MSG_LEN);
		if (strlen(buffer) == 0) continue;	/* Possible transmission mistakes */

		if (received_bytes <= 0){
			console_log("receive_messages: Error receiving bytes!");
			break;
		}

		parse_message(buffer, msg_sender, msg);
		if (!strcmp(msg_sender, "SERVER") && !strcmp(msg, QUIT_CMD)) break;

		printf("<%s> %s%c", msg_sender, msg, msg[strlen(msg)-1] == '\n' ? '\0' : '\n');
	}

	console_log("Exiting receive_messages thread.");

	return NULL;
}


/*
	Sends a message to the server. If
	the message size is greater than the
	maximum message length, this function
	will break it up into smaller chunks and
	send them individually.

	Returns 1 on success, -1 on failure.
*/
int send_to_server(Socket *socket, const char *msg){
	int msg_len = strlen(msg), i, status;

	for (i = 0; i < msg_len; i += MAX_MSG_LEN){
		status = socket_send(socket, msg + i, MAX_MSG_LEN);
		
		if (status < 0){
			console_log("send_to_server: Error sending message!");
			return -1;
		}
	}

	return 1;
}


/* Args must be a single Socket pointer */
void *send_messages(void *args){
	
	Socket *socket = (Socket *)args;
	char *msg = (char *)malloc(BUFFER_LEN*sizeof(char));
	msg[0] = '\0';

	console_log("Sending messages...");

	int status = 1;
	size_t max_msg_len = BUFFER_LEN;
	ssize_t real_msg_len;
	while (strcmp(msg, QUIT_CMD) && status > 0){

		real_msg_len = getline(&msg, &max_msg_len, stdin);

		if (real_msg_len <= 0){
			const char QUIT_MSG[] = "/quit";
			send_to_server(socket, QUIT_MSG);
			break;
		}

		msg[real_msg_len - 1] = '\0';	/* Remove trailing \n */
		status = send_to_server(socket, msg);
	}

	free(msg);
	console_log("Exiting send_messages thread.");
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

	console_log("Freeing socket");
	socket_free(socket);
	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
