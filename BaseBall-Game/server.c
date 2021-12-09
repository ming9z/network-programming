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
	int sock_fd, flags;
	struct sockaddr_in server_addr, cli_addr;

	// create socket for UDP
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	// configure server address for UDP
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(DISG_PORT);

	// bind socket and server
	bind(sock_fd, (SA *) &server_addr, sizeof(server_addr));

	// call a function
	user_check(sock_fd, (SA *) &cli_addr, sizeof(cli_addr));
}

void user_check(int sock_fd, SA *pcli_addr, socklen_t cli_len)
{

	int i, n;
    	socklen_t len;
    	char mesg[2];
	pid_t childpid;

	int tcp_sock, cli_fd, port;
	struct sockaddr_in tcp_servaddr, cli_addr;
	socklen_t tcp_len;
	
	// creat socket for TCP
	if((tcp_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Server: Can't open stream socker.");
		exit(0);
	}

	// covert the type
	port = atoi(TCP_PORT);

	// configure server address for TCP
	bzero((char *)&tcp_servaddr, sizeof(tcp_servaddr));  
    	tcp_servaddr.sin_family = AF_INET;              
    	tcp_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	tcp_servaddr.sin_port = htons(port);

	// bind socket and server
	if (bind(tcp_sock, (SA *)&tcp_servaddr, sizeof(tcp_servaddr)) < 0) {
		printf("Server: Can't bind local address.!\n");
	    exit(0);
	}

	// listen
	listen(tcp_sock, 10);
	tcp_len = sizeof(cli_addr);

	// loop - get the numbers from clients
    while(1) {

	len = cli_len;
	n = recvfrom(sock_fd, mesg, 2, 0, pcli_addr, &len); // get the start signal

        if(strcmp(mesg,"Y") == 0){
            sendto(sock_fd, TCP_PORT, 5, 0, pcli_addr, len); // send the reply
			printf("A client connect\n");

			cli_fd = accept(tcp_sock, (SA *)&cli_addr, &tcp_len); // TCP connection establishment
			if(cli_fd == -1)  {
				printf("Accept error\n");
				exit(0);
			}

			if((childpid = fork()) == 0) // make child process
			{ // child process
				close(tcp_sock); // stop child process listeing
				bbgame_server(cli_fd); // call a bbgame_server function
				close(cli_fd); // close parent process connection
				printf("A client disconnect\n");
			} else { // parent process
				close(cli_fd); // close parent process connection
			}
        }
		
	else {
		printf("Error");
	}
    }
}

// bbgame_server function
void bbgame_server(int sock_fd)
{
	char msg[MAX_LINE];
	char num_string[MAX_LINE];
	int num_int;
	char buf[BUF_LEN + 1];
	char num_buf[1];
	int server[3], client[3];
	int i,j,size;
	int strike = 0, ball = 0, trial = 0;

	// create random number without overlapping
	srand(time(NULL));

	server[0] = rand()%9 + 1;

	do{
		server[1] = rand()%9 + 1;
	}while(server[1] == server[0]);

	do
	{
		server[2] = rand()%9 + 1;
	}while( server[1] == server[2] || server[2] == server[3] );

	do{
		ball = 0; // initialize the variables
		strike = 0;
			
		if((size = read(sock_fd, num_string, MAX_LINE)) < 0) { // receive numbers
			printf("Error in read\n");
			close(sock_fd);
			exit(0);
		}

		num_string[size] = '\0';
		trial++;

		// parse the input
		num_int = atoi(num_string);
		client[2] = (num_int % 10); 
		client[1] = (num_int % 100) / 10;
		client[0] = num_int / 100;

		for(i = 0 ; i < 3 ; i++){ // calculate the number of strikes
			if(server[i] == client[i]) strike ++;
		}

		for(i = 0 ; i < 3 ; i++){ // calculate the number of strikes and balls
			for(j = 0 ; j < 3 ; j++){
				if(server[i] == client[j]) ball++;
			}
		}

		ball = ball - strike; // calculate the number of balls

		// make msg
		memset(msg, 0, MAX_LINE);
		if(strike == 3) sprintf(msg,"win");
		else if(trial >= 12) sprintf(msg,"lose");
		else {
			sprintf(msg, "Strike = %d , Ball = %d , Trial = %d \n",strike,ball,trial);
		}
		
		size = strlen(msg);
		if((send(sock_fd, msg, size, 0)) != size) printf("Error in write\n"); // send msg
	}while((strike != 3) && (trial < 12)); // while statement
}
