#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <irc_utils.h>

#define MAX_USERS 5
#define MAX_RETRIES 5
#define N_THREADS MAX_USERS

#define QUIT_CMD "/quit"
#define PING_CMD "/ping"
#define RENAME_CMD "/rename"

#define VALID_NAME_CHAR(c) (c != '<' && c != '>' && c != ':' && c != '\n')


typedef struct client {
	Socket *socket;
	int id;
	pthread_t thread;
	char username[MAX_NAME_LEN + 1];
} Client;


enum COMMANDS {
	QUIT, PING, RENAME, NO_CMD
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
		console_log("remove_client: Client not found.");
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
	Given a rename command, writes the
	parsed name to name and returns 1.
	If the chosen name is invalid, returns
	0 and name is altered.
	
	The contents of the name buffer are
	unpredictable if 0 is returned.

	The name buffer should have at least
	MAX_NAME_LEN + 1 bytes.
*/
int parse_name(char *buffer, char *name){

	buffer += strlen(RENAME_CMD) + 1;	/* Skips rename command */
	memset(name, 0, (MAX_NAME_LEN + 1)*sizeof(char));

	int i;
	for (i = 0; i <= MAX_NAME_LEN; i++){
		if (VALID_NAME_CHAR(buffer[i])) name[i] = buffer[i];
		else return 0;
		if (name[i] == '\0') break;
	}

	return i <= MAX_NAME_LEN;	/* If name is smaller than max, return 1. 0 otherwise */
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

		int send_retries = 0;
		status = socket_send(clients[j]->socket, msg, WHOLE_MSG_LEN);
		
		while (status < 0 && send_retries < MAX_RETRIES){
			send_retries++;
			console_log("chat_worker: Error sending message to client %s. Attempt %d", clients[j]->username, send_retries);
			status = socket_send(clients[j]->socket, msg, WHOLE_MSG_LEN);
		}

		if (send_retries >= MAX_RETRIES){
			console_log("chat_worker: Client %s unresponsive. Disconnecting.", clients[j]->username);
			remove_client(clients[j]);
		}
	}
}


int quit_command(Client *client){

	const char QUIT_MSG[] = "<SERVER> /quit\n";
	char msg[WHOLE_MSG_LEN];
	int send_retries = 0;

	/* Sends quit command to client */
	int status = socket_send(client->socket, QUIT_MSG, WHOLE_MSG_LEN);

	while (status < 0 && send_retries < MAX_RETRIES){
		send_retries++;
		console_log("interpret_command: attemtping to resend quit message to user. Attempt %d", send_retries);
		status = socket_send(client->socket, QUIT_MSG, WHOLE_MSG_LEN);
	}

	if (send_retries < MAX_RETRIES){
		console_log("User disconnected correctly.");
		sprintf(msg, "<SERVER> %s disconnected.\n", client->username);
		send_to_clients(client->id, msg);
	}

	return QUIT;
}


int ping_command(Client *client){

	const char PING_MSG[] = "<SERVER> pong\n";
	int send_retries = 0;

	int status = socket_send(client->socket, PING_MSG, WHOLE_MSG_LEN);

	while (status < 0 && send_retries < MAX_RETRIES){
		send_retries++;
		console_log("interpret_command: attempt %d to ping back client %s", send_retries, client->username);
		status = socket_send(client->socket, PING_MSG, WHOLE_MSG_LEN);
	}

	if (send_retries >= MAX_RETRIES)
		console_log("interpret_command: could not ping back user %s", client->username);
	else
		console_log("interpret_command: successfully pinged user %s", client->username);

	return PING;
}


int rename_command(Client *client, char *buffer){
	
	int RENAME_LEN = strlen(RENAME_CMD);

	if (strlen(buffer) <= RENAME_LEN || buffer[RENAME_LEN] != ' '){
		char msg[] = "Rename syntax is not correct. Usage is: /rename <new name>";
		socket_send(client->socket, msg, WHOLE_MSG_LEN);
		return RENAME;
	}

	/* This message gets overwritten if the rename is successful */
	char RENAME_MSG[MAX_MSG_LEN + 64] = "<SERVER> Failed to rename. Make sure your name does not exceed the maximum character limit or contain special symbols.\n";
	char new_name[MAX_NAME_LEN + 1];

	int is_valid = parse_name(buffer, new_name);

	if (!is_valid){
		socket_send(client->socket, RENAME_MSG, WHOLE_MSG_LEN);
		return RENAME;
	}

	sprintf(RENAME_MSG, "<SERVER> User %s renamed to %s\n", client->username, new_name);
	strncpy(client->username, new_name, MAX_NAME_LEN + 1);
	send_to_clients(client->id, RENAME_MSG);

	return RENAME;
}


/*
	Function that interprets all available
	commands.

	returns an integer indicating which command
	was interpreted.
*/
int interpret_command(Client *client, char *buffer){

	if (!strncmp(buffer, QUIT_CMD, strlen(QUIT_CMD))){
		return quit_command(client);
	}

	if (!strncmp(buffer, PING_CMD, strlen(PING_CMD))){
		return ping_command(client);
	}

	if (!strncmp(buffer, RENAME_CMD, strlen(RENAME_CMD))){
		return rename_command(client, buffer);
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
			sprintf(msg,"<SERVER> %s disconnected.\n", client->username);
			send_to_clients(client->id, msg);
			break;
		}

		if (buffer[0] == '/'){
			command = interpret_command(client, buffer);
			if (command == QUIT) break;
			
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
