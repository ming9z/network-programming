#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#define DISG_PORT 9005 // UDP port - I couldn't test 9000 because most students use this port.
#define TCP_PORT "30661" // TCP port
#define BUF_LEN 3
#define MAX_LINE 512

typedef struct sockaddr SA;

// function define
void baseball_client(int, char*);
void bbgame(int);
void bbgame_server(int sock_fd);
void user_check(int sock_fd, SA* pcli_addr, socklen_t cli_len);


int main(int argc, char *argv[])
{
	int sock_fd, n, port;
	char send[2];
	char recvline[5];
	struct sockaddr_in servaddr;

	if(argc != 2){
		printf("usage: bb_client <server IP address>\n");
		exit(0);
	}

	// configure server for UDP
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(DISG_PORT);
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	// create socket for UDP
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    strcpy(send,"Y");
    sendto(sock_fd, send, 2, 0, (SA *)&servaddr, sizeof(servaddr)); // request to start

    n = recvfrom(sock_fd, recvline, 5, 0, NULL, NULL); // receive a reply

	printf("Accept\n");
	port = atoi(recvline); // convert type
	close(sock_fd);
	baseball_client(port, argv[1]); // call baseball_client function

    return 0;
}

void baseball_client(int port, char *ip) // baseball_client function
{
	int sock_fd;
	struct sockaddr_in serv_addr;
	
	if ((sock_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) { // make socket for TCP
		printf("Client : Can't open stream socket.\n");
		exit(0);
	}
	
	// configure server for TCP
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;              
	serv_addr.sin_addr.s_addr = inet_addr(ip); 
	serv_addr.sin_port = htons(port); 

	if(connect(sock_fd, (SA *) &serv_addr, sizeof(serv_addr)) < 0) { // TCP connection establishment
		printf("Client : Can't connect to server.\n");
		exit(0);
	} else {
		printf("Connected to server\n");
		printf("Input the three number <ex>481\n");
		bbgame(sock_fd); // call bbgame function
	}
}

void bbgame(int sock_fd) // bbgame function
{
	char msg[MAX_LINE];
	char num[4];
	char buf[BUF_LEN + 1];
	int size;

	while(fgets(num, MAX_LINE, stdin) != NULL){ // get three numbers
		size = strlen(num);
		if((write(sock_fd, num, size)) != size) printf("Error in write\n"); // send three numbers in string type

		if((size = recv(sock_fd, msg, MAX_LINE, 0)) < 0) { // receive msg
			printf("Error in read\n");
			close(sock_fd);
			exit(0);
		}

		msg[size] = '\0';

		// parce msg
		if(strcmp(msg,"win") == 0) { // win
			printf("Win!! You got it!\n");
			close(sock_fd);
			exit(0);
		} else if(strcmp(msg,"lose") == 0) { // lose
			printf("12 trial, You LOSE!\n");
			close(sock_fd);
			exit(0);
		} else {
			printf("%s",msg); // show the number of strikes, balls, and trials
		}
	}
}
		
