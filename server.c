#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define N_USERS 2	/* Exact number of users to chat */
#define N_THREADS N_USERS

#define QUIT_CMD "/quit"


typedef struct thread_args{
	int client;
	Socket **clients;
	char username[MAX_NAME_LEN + 1];
} thread_args;


typedef struct interpret_args {
	int client;
	Socket *client_socket;
	Socket **clients;
	char *buffer;
} interpret_args;

enum COMMANDS {
	QUIT, NO_CMD
};


/*
	Sends msg to all clients that have a different
	index than sender. To send to all clients, set
	sender to -1.
*/
void send_to_clients(Socket **clients, int sender, char msg[]){
	int j, status;
	for (j = 0; j < N_USERS; j++){
		if (j == sender) continue;
		status = socket_send(clients[j], msg, WHOLE_MSG_LEN);

		if (status < 0){
			console_log("chat_worker: Error sending message to client");
		}
	}
}


int interpret_command(interpret_args *args){
	char msg[WHOLE_MSG_LEN];

	if (!strcmp(args->buffer, QUIT_CMD)){
		/* Pings back to client */
		socket_send(args->client_socket, "<SERVER> /quit", WHOLE_MSG_LEN);
		console_log("User disconnected correctly.");

		sprintf(msg, "<SERVER> User %d disconnected.\n", args->client);
		send_to_clients(args->clients, args->client, msg);

		return QUIT;
	}

	return NO_CMD;
}


void *chat_worker(void *args){
	int client = ((thread_args *)args)->client;
	char *username = ((thread_args *)args)->username;
	Socket **clients = ((thread_args *)args)->clients;

	Socket *client_socket = clients[client];

	int msg_len, command;
	char buffer[MAX_MSG_LEN + 1] = {0};
	char msg[WHOLE_MSG_LEN];

	interpret_args cmd_args = {client, client_socket, clients, buffer};

	while (strcmp(buffer, QUIT_CMD)){
		msg_len = socket_receive(client_socket, buffer, MAX_MSG_LEN);

		if (msg_len <= 0){
			console_log("User disconnected unpredictably!");
			break;
		}

		if (buffer[0] == '/'){
			command = interpret_command(&cmd_args);
			if (command == QUIT)
				break;
		} else {
			/* Send regular message */
			sprintf(msg, "<%s> %s\n", username, buffer);
			send_to_clients(clients, client, msg);			
		}
	}

	return NULL;
}


int main(){

	Socket *socket = socket_create();
	socket_bind(socket, SERVER_PORT, SERVER_ADDR);
	socket_listen(socket);

	Socket *clients[N_USERS];
	pthread_t threads[N_USERS];

	int i;
	for (i = 0; i < N_USERS; i++)
		clients[i] = socket_accept(socket);

	char temp_username[MAX_NAME_LEN + 1];
	thread_args args[N_USERS];

	for (i = 0; i < N_USERS; i++){
		args[i].clients = clients;
		args[i].client = i;
		
		sprintf(temp_username, "User %02d", i);
		strncpy(args[i].username, temp_username, MAX_NAME_LEN + 1);
	}

	for (i = 0; i < N_USERS; i++)
		pthread_create(threads + i, NULL, chat_worker, args + i);

	for (i = 0; i < N_USERS; i++){
		pthread_join(threads[i], NULL);
		socket_free(clients[i]);		
	}

	socket_free(socket);
	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
