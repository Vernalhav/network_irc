#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>


int main(){
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	//server_addr.sin_addr.s_addr = INADDR_ANY;

	/* IP of the machine to connect */
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	server_addr.sin_port = htons(8888);
	connect(sockfd, (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr));

	char msg[4096] = {0};
	while (strcmp(msg, "/quit")){
		printf(">> ");
		scanf("%[^\n]%*c", msg);
		send(sockfd, msg, 4096, 0);
	}

	close(sockfd);
}