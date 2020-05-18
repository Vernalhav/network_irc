#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define MAX_USERS 5
#define N_THREADS MAX_USERS

#define QUIT_CMD "/quit"


typedef struct client {
	Socket *socket;
	int id;
	pthread_t thread;
	char username[MAX_NAME_LEN + 1];
} Client;


enum COMMANDS {
	QUIT, NO_CMD
};


pthread_mutex_t lock;
Client *clients[MAX_USERS];
int current_users = 0;


/* Creates a client with temporary username "User <id>" */
Client *client_create(int id, Socket *socket){
	Client *client = (Client *)malloc(sizeof(Client));

	client->id = id;
	client->socket = socket;
	sprintf(client->username, "User %02d", id);

	return client;
}


void client_free(Client *client){
	socket_free(client->socket);
	free(client);
}


/*
	Adds client to the last available position
	in the clients array. Mutex is used to
	prevent thread usage inconsistencies.
	Alters current_users value

	Returns boolean whether or not the insertion
	was successful.
*/
int add_client(Client *client){
	pthread_mutex_lock(&lock);

	if (current_users >= MAX_USERS){
		console_log("add_client: did not add user. Max users online.");
		pthread_mutex_unlock(&lock);
		return 0;
	}
	
	clients[current_users++] = client;
	console_log("add_client: Current users: %d", current_users);

	pthread_mutex_unlock(&lock);

	return 1;
}

/*
	Removes client from the clients array.
	The array to the left of the removed
	client is shifted to occupy the vacant
	position.
	Mutex is used to prevent thread usage
	inconsistencies.
	Client must be freed outside this function.
	Alters current_users value

	Returns boolean whether or not the insertion
	was successful.
*/
int remove_client(Client *client){
	pthread_mutex_lock(&lock);
	
	int i;
	for (i = 0; i < current_users; i++){
		if (clients[i]->id == client->id) break;
	}

	if (i >= current_users){
		console_log("remove_client: Could not remove client.");
		pthread_mutex_unlock(&lock);
		return 0;
	}

	for (i = i; i < current_users - 1; i++){
		clients[i] = clients[i + 1];
	}

	clients[current_users - 1] = NULL;	/* Small safety feature */

	current_users--;
	console_log("remove_client: Current users: %d", current_users);

	pthread_mutex_unlock(&lock);

	return 1;
}


/*
	Sends msg to all clients that have a different
	index than sender. To send to all clients, set
	sender to -1.
*/
void send_to_clients(int sender_id, char msg[]){
	
	int j, status;
	for (j = 0; j < current_users; j++){
		if (clients[j]->id == sender_id) continue;

		status = socket_send(clients[j]->socket, msg, WHOLE_MSG_LEN);
		if (status < 0){
			console_log("chat_worker: Error sending message to client");
		}
	}
}


/*
	Function that interprets all available
	commands. args is a structure that contains
	all arguments to this function. See the
	struct definition for a list of all fields.

	returns an integer indicating which command
	was interpreted.
*/
int interpret_command(Client *client, char *buffer){
	char msg[WHOLE_MSG_LEN];
	int status;

	const char QUIT_MSG[] = "<SERVER> /quit\n";

	if (!strcmp(buffer, QUIT_CMD)){
		/* Pings back to client */
		status = socket_send(client->socket, QUIT_MSG, WHOLE_MSG_LEN);
		if (status < 0){
			console_log("interpret_command: Error sending quit message to user!");
			return 0;
		}

		console_log("User disconnected correctly.");

		sprintf(msg, "<SERVER> %s disconnected.\n", client->username);
		send_to_clients(client->id, msg);

		return QUIT;
	}

	return NO_CMD;
}

/*
	Thread that handles a client connection.
	It reads the designated client's messages
	and interprets its commands.

	This function terminates when its client
	sends a quit command, or when there is
	an unexpected disconnect by its client.
*/
void *chat_worker(void *args){
	Client *client = (Client *)args;

	char welcome_msg[3*MAX_NAME_LEN];
	sprintf(welcome_msg, "<SERVER> %s connected to chat!", client->username);
	send_to_clients(client->id, welcome_msg);

	int msg_len, command;
	char buffer[MAX_MSG_LEN + 1] = {0};
	char msg[WHOLE_MSG_LEN];

	while (strcmp(buffer, QUIT_CMD)){
		msg_len = socket_receive(client->socket, buffer, MAX_MSG_LEN);

		if (msg_len <= 0){
			console_log("User disconnected unpredictably!");
			break;
		}

		if (buffer[0] == '/'){
			command = interpret_command(client, buffer);
			if (command == QUIT)
				break;
		} else {
			/* Send regular message */
			sprintf(msg, "<%s> %s\n", client->username, buffer);
			send_to_clients(client->id, msg);			
		}
	}

	remove_client(client);
	client_free(client);

	return NULL;
}


int main(){

	Socket *socket = socket_create();
	socket_bind(socket, SERVER_PORT, SERVER_ADDR);
	socket_listen(socket);

	pthread_mutex_init(&lock, NULL);

	unsigned int current_id = 0;
	Client *current_client;
	Socket *current_client_socket;
	
	do {
		current_client_socket = socket_accept(socket);
		current_client = client_create(current_id++, current_client_socket);
		add_client(current_client);

		pthread_create(&(current_client->thread), NULL, chat_worker, current_client);
	} while (current_users > 0);

	socket_free(socket);
	return 0;
}

/*
	Super helpful reference:
	http://beej.us/guide/bgnet/html/
*/
