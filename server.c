#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define MAX_CONNECTIONS 2
#define N_THREADS 2

#define QUIT_CMD "/quit"


typedef struct thread_args{
	int client;
	Socket **clients;
	char username[MAX_NAME_LEN + 1];
} thread_args;


void *chat_worker(void *args){
	int client = ((thread_args *)args)->client;
	char *username = ((thread_args *)args)->username;
	Socket **clients = ((thread_args *)args)->clients;

	Socket *client_socket = clients[client];

	int msg_len, j;
	char buffer[MAX_MSG_LEN] = {0};
	char msg[MAX_NAME_LEN + MAX_MSG_LEN + 16];

	while (strcmp(buffer, QUIT_CMD)){
		msg_len = socket_receive(client_socket, buffer);

		if (msg_len <= 0){
			console_log("User disconnected unpredictably!");
			break;
		}

		sprintf(msg, "<%s> %s\n", username, buffer);

		for (j = 0; j < MAX_CONNECTIONS; j++){
			if (j == client) continue;
			socket_send(clients[j], msg);
		}
	}

	return NULL;
}


int main(){

	Socket *socket = socket_create();
	socket_bind(socket, SERVER_PORT, SERVER_ADDR);
	socket_listen(socket);

	Socket *clients[MAX_CONNECTIONS];
	pthread_t threads[MAX_CONNECTIONS];

	int i;
	for (i = 0; i < MAX_CONNECTIONS; i++)
		clients[i] = socket_accept(socket);

	char temp_username[MAX_NAME_LEN + 1];
	thread_args args[MAX_CONNECTIONS];

	for (i = 0; i < MAX_CONNECTIONS; i++){
		args[i].clients = clients;
		args[i].client = i;
		
		sprintf(temp_username, "User %02d", i);
		strncpy(args[i].username, temp_username, MAX_NAME_LEN + 1);
	}

	for (i = 0; i < MAX_CONNECTIONS; i++)
		pthread_create(threads + i, NULL, chat_worker, args + i);

	for (i = 0; i < MAX_CONNECTIONS; i++)
		pthread_join(threads[i], NULL);
		socket_free(clients[i]);

	socket_free(socket);
	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
