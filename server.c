#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <irc_utils.h>
#include <signal.h>
#include <pthread.h>

#include <server.h>

#define LOBBY 0

#define QUIT_CMD "/quit"
#define PING_CMD "/ping"
#define RENAME_CMD "/nickname"
#define JOIN_CMD "/join"
#define KICK_CMD "/kick"
#define MUTE_CMD "/mute"
#define UNMUTE_CMD "/unmute"
#define WHOIS_CMD "/whois"

#define VALID_NAME_CHAR(c) (c != '<' && c != '>' && c != ':' && c != '@' && c != '\n')
#define VALID_CHANNEL_CHAR(c) (c != ' ' && c != ',' && c != 7)


pthread_mutex_t clients_lock;
pthread_mutex_t channels_lock;

Client *clients[MAX_USERS];
int current_users = 0;

Channel *channels[MAX_CHANNELS];
int current_channels = 0;


typedef struct client {
    Socket *socket;
    int id;
    pthread_t thread;
    char username[MAX_NAME_LEN + 1];
    char channel[MAX_CHANNEL_LEN];
} Client;


typedef struct channel {
    int muted_users[MAX_USERS];
    int current_mutes;

    int admin;
    int current_users;

    Client *users[MAX_USERS];
    char name[MAX_CHANNEL_LEN];
} Channel;


enum COMMANDS {
    QUIT, PING, RENAME, JOIN, NO_CMD
};



/*
	Creates an empty channel with the given name and admin.
	Admin should only be NULL for the initial Lobby chat.
*/
Channel *channel_create(char name[MAX_CHANNEL_LEN], Client *admin){

	Channel *c = (Channel *)malloc(sizeof(Channel));

	memset(c->muted_users, -1, sizeof(c->muted_users));
	memset(c->users, 0, sizeof(c->users));
	strncpy(c->name, name, MAX_CHANNEL_LEN);
	
	c->current_users = 0;
	c->current_mutes = 0;
	c->admin = (admin == NULL) ? LOBBY : admin->id;

	return c;
}


/* Does not free the channel's users */
void channel_free(Channel *c){
	free(c);
}


int find_channel(char channel_name[MAX_CHANNEL_LEN]){
	int i;
	for (i = 0; i < current_channels; i++){
		if (!strcmp(channel_name, channels[i]->name)) return i;
	}

	return -1;
}


int invalid_channel_name(char channel_name[MAX_CHANNEL_LEN]){
	int i;
	for (i = 0; i < strlen(channel_name); i++)
		if (!VALID_CHANNEL_CHAR(channel_name[i])) return 1;

	return 0;
}


/*
	Sends msg to all clients that have a different
	ID than sender. To send to all clients on a channel, set
	sender to -1. To send to all clients regardles of channel,
	set channel to NULL
*/
void send_to_clients(int sender_id, char msg[], char channel[]){
	
	int j, status, send_retries;
	
	if (channel == NULL){
		for (j = 0; j < current_users; j++){
			if (clients[j]->id == sender_id) continue;

			send_retries = 0;
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
	else {
		pthread_mutex_lock(&channels_lock);

		int channel_index = find_channel(channel);
		if (channel_index == -1){
			console_log("Could not find channel to send.");
			return;
		}

		Channel *channel = channels[channel_index];
		pthread_mutex_unlock(&channels_lock);

		for (j = 0; j < channel->current_users; j++){
			if (channel->users[j]->id == sender_id) continue;

			int send_retries = 0;
			status = socket_send(channel->users[j]->socket, msg, WHOLE_MSG_LEN);
			
			while (status < 0 && send_retries < MAX_RETRIES){
				send_retries++;
				console_log("chat_worker: Error sending message to client %s. Attempt %d", channel->users[j]->username, send_retries);
				status = socket_send(channel->users[j]->socket, msg, WHOLE_MSG_LEN);
			}

			if (send_retries >= MAX_RETRIES){
				console_log("chat_worker: Client %s unresponsive. Disconnecting.", channel->users[j]->username);
				
				leave_channel(channel->users[j]);
				remove_client(channel->users[j]);
			}
		}

	}
}


/*
	Removes client from current channel.

	If the client is the only one in the
	channel, deletes the channel from the
	list (except for the lobby).
	
	Returns 0 on failure, 1 on success.

	NOTE: this function uses channels_lock.
*/
int leave_channel(Client *client){
	
	pthread_mutex_lock(&channels_lock);
	
	int channel_index = find_channel(client->channel);
	if (channel_index == -1){
		console_log("Could not find channel %s", client->channel);
		return 0;
	}

	Channel *current_channel = channels[channel_index];

	int i, j;
	for (i = 0; i < current_channel->current_users; i++){
		if (current_channel->users[i]->id == client->id){
			for (j = i; j < current_channel->current_users - 1; j++){
				current_channel->users[j] = current_channel->users[j + 1];
			}
		}
	}

	current_channel->current_users--;
	console_log("Channel %s has %d users", current_channel->name, current_channel->current_users);

	char leave_msg[50 + MAX_NAME_LEN];
	sprintf(leave_msg, "SERVER: %s left the channel.", client->username);

	pthread_mutex_unlock(&channels_lock);
	send_to_clients(client->id, leave_msg, client->channel);
	pthread_mutex_lock(&channels_lock);

	if (current_channel->current_users == 0 && strcmp(current_channel->name, "lobby")){
		/* Remove channel from list */
		console_log("Removing channel %s", current_channel->name);
		for (i = 0; i < current_channels; i++){
			if (!strcmp(current_channel->name, channels[i]->name)){
				for (j = i; j < current_channels - 1; j++){
					channels[j] = channels[j + 1];
				}
			}
		}

		channel_free(current_channel);
		current_channels--;
		console_log("Current channels: %d", current_channels);
	}

	pthread_mutex_unlock(&channels_lock);
	return 1;
}


/*
	Adds client to the list of users of channel
	with channel_name, leaving previous channel.

	If no channel with the name exists, it is created
	and client becomes its admin. If the channel
	name is invalid, does nothing.

	Returns 0 on failure, 1 on success.
*/
int join_channel(char channel_name[MAX_CHANNEL_LEN], Client *client){

	if (invalid_channel_name(channel_name)) return 0;

	if (!strcmp(channel_name, client->channel)) return 0;

	/* Remove client from current channel, deleting it if necessary */	
	if (strcmp(client->channel, "NO CHANNEL"))
		leave_channel(client);

	strcpy(client->channel, channel_name);

	pthread_mutex_lock(&channels_lock);
	
	/* Adding client to new channel, possibly creating it */
	int channel_index = find_channel(channel_name);
	Channel *new_channel;

	if (channel_index == -1){
		new_channel = channel_create(channel_name, client);
		channels[current_channels++] = new_channel;
	}
	else new_channel = channels[channel_index];
	
	new_channel->users[new_channel->current_users++] = client;

	pthread_mutex_unlock(&channels_lock);

	char join_msg[50 + MAX_NAME_LEN];
	sprintf(join_msg, "SERVER: %s joined the channel.", client->username);
	send_to_clients(-1, join_msg, client->channel);

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
	pthread_mutex_lock(&clients_lock);
	
	int i;
	for (i = 0; i < current_users; i++){
		if (clients[i]->id == client->id) break;
	}

	if (i >= current_users){
		console_log("remove_client: Client not found.");
		pthread_mutex_unlock(&clients_lock);
		return 0;
	}

	for (i = i; i < current_users - 1; i++){
		clients[i] = clients[i + 1];
	}

	clients[current_users - 1] = NULL;	/* Small safety feature */

	current_users--;
	console_log("remove_client: Current users: %d", current_users);

	pthread_mutex_unlock(&clients_lock);

	return 1;
}


void disconnect_clients(){
	char CLOSE_MSG[] = "SERVER: Closing server. Terminating connection.";
	send_to_clients(-1, CLOSE_MSG, NULL);
	char QUIT_MSG[] = "SERVER: /quit";
	send_to_clients(-1, QUIT_MSG, NULL);

	while (current_users > 0){
		remove_client(clients[0]);
	}
}


void handle_interrupt(int sig){
	if (current_users > 0){
		printf("\nCan't terminate because users are still connected\n");
	} else exit(0);
}


/* Creates a client with temporary username "User <id>" */
Client *client_create(int id, Socket *socket){
	Client *client = (Client *)malloc(sizeof(Client));

	client->id = id;
	client->socket = socket;
	sprintf(client->username, "User %02d", id);
	strcpy(client->channel, "NO CHANNEL");

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
	pthread_mutex_lock(&clients_lock);

	if (current_users >= MAX_USERS){
		console_log("add_client: did not add user. Max users online.");
		pthread_mutex_unlock(&clients_lock);
		return 0;
	}
	
	clients[current_users++] = client;
	console_log("add_client: Current users: %d", current_users);

	pthread_mutex_unlock(&clients_lock);

	return 1;
}


/*
	Given a username, return whether
	it is unique among the connected users.
*/
int unique_name(char *name){

	int i;
	for (i = 0; i < current_users; i++)
		if (!strcmp(name, clients[i]->username)) return 0;

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


int join_command(Client *client, char *buffer){

	char channel_name[MAX_CHANNELS];
	sscanf(buffer, "%*s %[^\n]%*c", channel_name);

	console_log("Attempting to join channel %s...", channel_name);

	join_channel(channel_name, client);

	return JOIN;
}


int quit_command(Client *client){

	const char QUIT_MSG[] = "SERVER: /quit";
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
		sprintf(msg, "SERVER: %s disconnected.", client->username);
		send_to_clients(client->id, msg, client->channel);
		leave_channel(client);
	}

	return QUIT;
}


int ping_command(Client *client){

	const char PING_MSG[] = "SERVER: pong";
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
		char msg[] = "SERVER: Rename syntax is not correct. Usage is: /nickname <new name>";
		socket_send(client->socket, msg, WHOLE_MSG_LEN);
		return RENAME;
	}

	/* This message gets overwritten if the rename is successful */
	char RENAME_MSG[MAX_MSG_LEN + 64] = "SERVER: Failed to rename. Make sure your name does not exceed the maximum character limit, contain special symbols and is unique.";

	char new_name[MAX_NAME_LEN + 1];

	int is_valid = parse_name(buffer, new_name) && unique_name(new_name);

	if (!is_valid){
		socket_send(client->socket, RENAME_MSG, WHOLE_MSG_LEN);
		return RENAME;
	}

	sprintf(RENAME_MSG, "SERVER: User %s renamed to %s", client->username, new_name);
	strncpy(client->username, new_name, MAX_NAME_LEN + 1);
	send_to_clients(client->id, RENAME_MSG, NULL);

	return RENAME;
}


int invalid_command(Client *client){
	char msg[] = "SERVER: Invalid command. Available commands are:\n  > /ping\n  > /nickname <new name>\n  > /quit\n";
	socket_send(client->socket, msg, MAX_MSG_LEN);
	return NO_CMD;
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

	if (!strncmp(buffer, JOIN_CMD, strlen(JOIN_CMD))){
		return join_command(client, buffer);
	}

	return invalid_command(client);
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

	/* Disable this thread from handling SIGINT */
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

	Client *client = (Client *)args;

	char nickname[3*MAX_NAME_LEN];
	socket_receive(client->socket, nickname, MAX_MSG_LEN);

	if (nickname[0] != ':'){
		if (unique_name(nickname)){
			strncpy(client->username, nickname, MAX_NAME_LEN + 1);
		} else {
			char not_unique[MAX_MSG_LEN];
			sprintf(not_unique, "SERVER: the username %s is already taken. Assigning default nickname %s (try /nickname)", nickname, client->username);
			socket_send(client->socket, not_unique, MAX_MSG_LEN);
		}
	}

	add_client(client);
	join_channel("lobby", client);

	char ip[64];
	socket_ip(client->socket, ip);

	char welcome_msg[3*MAX_NAME_LEN];
	sprintf(welcome_msg, "SERVER: %s connected to chat!", client->username);
	send_to_clients(client->id, welcome_msg, NULL);

	char HELP_MSG[] = "SERVER: Type /help to see available commands.";
	socket_send(client->socket, HELP_MSG, MAX_MSG_LEN);

	int msg_len, command;
	char buffer[MAX_MSG_LEN + 1] = {0};
	char msg[WHOLE_MSG_LEN];

	while (strcmp(buffer, QUIT_CMD)){
		msg_len = socket_receive(client->socket, buffer, MAX_MSG_LEN);

		if (msg_len <= 0){
			console_log("User disconnected unpredictably!");
			sprintf(msg,"SERVER: %s disconnected.", client->username);
			send_to_clients(client->id, msg, NULL);
			leave_channel(client);
			break;
		}

		if (buffer[0] == '/'){
			command = interpret_command(client, buffer);
			if (command == QUIT) break;

		} else {
			/* Send regular message */
			sprintf(msg, "%s: (@%s) %s", client->username, client->channel, buffer);
			send_to_clients(client->id, msg, client->channel);			
		}
	}

	remove_client(client);
	client_free(client);

	return NULL;
}


void *accept_clients(void *args){

	/* Disable this thread from handling SIGINT */
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

	Socket *socket = (Socket *)args;

	unsigned int current_id = 1;
	Client *current_client;
	Socket *current_client_socket;

	do {
		current_client_socket = socket_accept(socket);
		current_client = client_create(current_id++, current_client_socket);
		
		pthread_create(&(current_client->thread), NULL, chat_worker, current_client);
	} while (1);

	return NULL;
}


int main(){

	/* Handle SIGINT */
	struct sigaction signal;
	signal.sa_handler = handle_interrupt;
	sigemptyset(&(signal.sa_mask));
	signal.sa_flags = 0;
	sigaction(SIGINT, &signal, NULL);

	Socket *socket = socket_create();
	socket_bind(socket, SERVER_PORT, SERVER_ADDR);
	socket_listen(socket);

	pthread_mutex_init(&clients_lock, NULL);
	pthread_mutex_init(&channels_lock, NULL);

	channels[0] = channel_create("lobby", NULL);
	current_channels = 1;
	
	pthread_t acc_daemon;
	pthread_create(&acc_daemon, NULL, accept_clients, socket);

	pthread_join(acc_daemon, NULL);

	socket_free(socket);
	return 0;
}
