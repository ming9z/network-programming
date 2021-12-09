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
void baseball_client(int, char *);
void bbgame(int);
void bbgame_server(int sock_fd);
void user_check(int sock_fd, SA *pcli_addr, socklen_t cli_len);
