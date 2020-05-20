#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <pthread.h>
#include <irc_utils.h>
#include <regex.h>

#define MAX_MSG_FACTOR 5
#define BUFFER_LEN MAX_MSG_LEN*MAX_MSG_FACTOR + 16

#define N_THREADS 2
#define QUIT_CMD "/quit"
#define MAX_CMD_LEN 1023

#define NICKNAME nickname[0] == '\0' ? "not set" : nickname
#define MATCH_IPV4_REGEX "([0-9]{1,3}\\.){3}[0-9]{1,3}"

/*
	Parses received buffer and fills in
	msg_sender with the author's name and
	msg with the actual message content.

	Both msg_sender and msg are initialized
	with '\0'.
*/
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


/*
	Connects to chatting server at addr/port.
	Returns 1 if the connection was successful
	and 0 if server wasn't available.
*/
int connect_and_chat(char *addr, int port, char *nickname){
	
	Socket *socket = socket_create();
	int status = socket_connect(socket, port, addr);

	if (status == -1){
		socket_free(socket);
		return 0;
	}

	pthread_t threads[N_THREADS];

	pthread_create(threads + 0, NULL, send_messages, (void *)socket);
	pthread_create(threads + 1, NULL, receive_messages, (void *)socket);

	int i;
	for (i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);

	console_log("Freeing socket");
	socket_free(socket);

	return 1;
}


/*
	Alters ipv4_addr and port to contain the values
	specified by the user in cmd.
	
	If the string is invalid, the original buffers
	are not altered and 0 is returned.

	Otherwise, the buffers containt the values entered
	in cmd and 1 is returned.
*/
int change_server(char *cmd, char *ipv4_addr, int *port){

	char temp_addr[16] = {0};
	int temp_port = -1;

	sscanf(cmd, "%*s %s %d", temp_addr, &temp_port);

	if (temp_port < 0 || temp_addr[0] == '\0'){
		printf("Invalid syntax.\nUse /server <IPv4> <port>\n");
		return 0;
	}

	regex_t regex;
	int status = regcomp(&regex, MATCH_IPV4_REGEX, REG_EXTENDED);

	if (status){
		console_log("Could not compile regex.");
		regfree(&regex);
		return 0;
	}
	
	status = regexec(&regex, temp_addr, 0, NULL, 0);
	regfree(&regex);
	
	if (status != 0){
		printf("The IPv4 address supplied is invalid.\n");
		return 0;
	}

	*port = temp_port;
	strncpy(ipv4_addr, temp_addr, 16);
	
	printf("Server connection info changed successfully!.\n");
	return 1;
}


int change_nickname(char *cmd, char *nickname){
	return 1;
}


int main(){

	/* Default connection settings */
	char ipv4_addr[16];
	int port = SERVER_PORT;
	strncpy(ipv4_addr, SERVER_ADDR, 16);

	char nickname[MAX_NAME_LEN + 1] = {0};
	char cmd[MAX_CMD_LEN + 1] = {0};

	do {
		printf("Current server is %s port %d\nCurrent nickname is %s\n", ipv4_addr, port, NICKNAME);
		printf(">> ");
		scanf("%[^\n]%*c", cmd);

		if (!strncmp(cmd, "/connect", 8)){
			int status = connect_and_chat(ipv4_addr, port, nickname);
			if (!status)
				printf("Could not reach server.\n");
		}

		if (!strncmp(cmd, "/server", 7)){
			change_server(cmd, ipv4_addr, &port);
		}
		
		if (!strncmp(cmd, "/nickname", 9)){
			change_nickname(cmd, nickname);
		}

		putchar('\n');
	} while (strncmp(cmd, "/quit", 5));

	return 0;
}