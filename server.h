#ifndef SERVER_H

#define SERVER_H

#define MAX_USERS 5
#define MAX_RETRIES 5
#define N_THREADS MAX_USERS

#define MAX_CHANNELS 32


Channel *channel_create(char name[MAX_CHANNEL_LEN], Client *admin);

void channel_free(Channel *c);

int find_channel(char channel_name[MAX_CHANNEL_LEN]);

int invalid_channel_name(char channel_name[MAX_CHANNEL_LEN]);

void send_to_clients(int sender_id, char msg[], char channel[]);

int leave_channel(Client *client);

int join_channel(char channel_name[MAX_CHANNEL_LEN], Client *client);

int remove_client(Client *client);

void disconnect_clients();

void handle_interrupt(int sig);

Client *client_create(int id, Socket *socket);

void client_free(Client *client);

int add_client(Client *client);

int parse_name(char *buffer, char *name);

int join_command(Client *client, char *buffer);

int quit_command(Client *client);

int ping_command(Client *client);

int rename_command(Client *client, char *buffer);

int invalid_command(Client *client);

int interpret_command(Client *client, char *buffer);

void *chat_worker(void *args);

void *accept_clients(void *args);

#endif