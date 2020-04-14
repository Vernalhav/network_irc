#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define MAX_CONNECTIONS 2
#define SERVER_PORT 8888
#define SERVER_ADDR "127.0.0.1"
#define N_THREADS 2

#define QUIT_CMD "/quit"

typedef struct thread_args{
	int client;
	Socket **clients;
} thread_args;


void *chat_worker(void *args){
	int client = ((thread_args *)args)->client;
	Socket **clients = ((thread_args *)args)->clients;

	Socket *client_socket = clients[client];

	int msg_len, j;
	char msg[MAX_MSG_LEN] = {0};
	
	while (strcmp(msg, QUIT_CMD)){
		msg_len = socket_receive(client_socket, msg);
		if (msg_len == 0) continue;

		if (msg_len < 0){
			console_log("Error receiving message!");
			break;
		}

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

	thread_args args[MAX_CONNECTIONS];
	for (i = 0; i < MAX_CONNECTIONS; i++){
		args[i].clients = clients;
		args[i].client = i;
	}

	for (i = 0; i < MAX_CONNECTIONS; i++)
		pthread_create(threads + i, NULL, chat_worker, args + i);

	for (i = 0; i < MAX_CONNECTIONS; i++)
		pthread_join(threads[i], NULL);

	for (i = 0; i < MAX_CONNECTIONS; i++)
		socket_free(clients[i]);

	socket_free(socket);
	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
