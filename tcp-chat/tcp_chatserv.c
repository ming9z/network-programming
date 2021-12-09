//-------------------------------------------------------------
// ���ϸ� : tcp_chatserv.c
// ��  �� : ä�� ������ ����, ä�� �޽��� ���� �� ���
// ������ : cc -o chat_server tcp_chatserv.c
// ���� : chat_server 4001
//-------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAXLINE  511
#define MAX_SOCK 512 // �ֶ󸮽��� ��� 64

char *EXIT_STRING = "exit";   // Ŭ���̾�Ʈ�� �����û ���ڿ�
char *LOGOUT_STRING = "logout";
char *START_STRING = "  (�α���:login   ȸ������:join)\n";
char *LOGOUT = " �α׾ƿ� �Ǿ����ϴ�.)\n";
    // Ŭ���̾�Ʈ ȯ�� �޽���
int maxfdp1;                // �ִ� ���Ϲ�ȣ +1
int num_chat = 0;          // ä�� ������ ��
int clisock_list[MAX_SOCK]; // ä�ÿ� ������ ���Ϲ�ȣ ���
int listen_sock;         // ������ ���� ����
char list[100][MAX_SOCK];
char *ID_REG = "  (������ ID�� �Է��ϼ���) : ";
char *ID_ID = "  (�̹� ���Ե� ���̵� �ֽ��ϴ�.)\n  (join�� �Է��� �ٸ� ���̵� �Է��� �ּ���) : ";
char *ID_INPUT = "  (ID�� �Է��ϼ���.)\n";
char *LOGON = " �α��� �Ǿ����ϴ�.)\n";
char *LOGFAIL = "  (�α����� ���� �ʾҽ��ϴ�.)\n  (���� �Ǵ� �α��� ���ּ���)\n";
char *ID_RESULT = "  (���ԵǾ����ϴ�.)\n  (�α��� ���ּ���)\n";
char *ID_REMOVE = "  (Ż���Ͻðڽ��ϱ�?)\n  (ID�� �Է��ϼ���)\n";
char *ID_OUT = "  (���������� Ż��Ǿ����ϴ�.)\n";
char *ID_NULL = "  (�߸� �Է��ϼ̽��ϴ�. �ڽ��� ID�� �Է��ϼ���.)\n";
char *USERNAME = "  (���� �α��ε� ����� ����Ʈ�Դϴ�.)\n";
char *ENTER = " \n";

struct user{
	char ID[20];
	int cli_sock;
	int login_check;
	int chat_check;
	};
struct user db[MAX_SOCK+1];


// ���ο� ä�� ������ ó��
void addClient(int s, struct sockaddr_in *newcliaddr);
int getmax();               // �ִ� ���� ��ȣ ã��
void removeClient(int s);    // ä�� Ż�� ó�� �Լ�
int tcp_listen(int host, int port, int backlog); // ���� ���� �� listen
void errquit(char *mesg) { perror(mesg); exit(1); }
void Login(int clnt);        // �α��� �Լ� ȣ��
void Logout(int clnt);
void Join(int clnt);        // �����Լ� ȣ��
void Remove(int clnt);      // Ż���Լ� ȣ�� 
void List(int clnt);		// ����Ʈ�Լ� ȣ��


int main(int argc, char *argv[]) {
    struct sockaddr_in cliaddr;
    char buf[MAXLINE+1], buftmp[MAXLINE+1], *bufmsg;
    int i, j, nbyte, accp_sock, addrlen = sizeof(struct
        sockaddr_in), namelen;
    pid_t pid;
    struct sigaction sact;
    fd_set read_fds;  // �б⸦ ������ fd_set ����ü

    for(i=0 ; i<20 ; i++){
	db[i].ID[i] = '\0';
    }

    if(argc != 2) {
        printf("���� :%s port\n", argv[0]);
        exit(0);
    }

    
    if((pid==fork())!=0)
	exit(0);   // �θ� ���μ����� �����Ų��, ��, �ڽ� ���μ����� ����
    setsid();      

    sact.sa_handler = SIG_IGN;
    sact.sa_flags = 0;
    sigemptyset(&sact.sa_mask);
    sigaddset(&sact.sa_mask, SIGHUP);
    sigaction(SIGHUP, &sact, NULL);

    if((pid==fork())!=0)    // �ٽ� �ڽ����μ���(����)�� �����
	exit(0);	    // �θ����μ����� �����Ų��
//    chdir("/");
    umask(0);
    // Ȥ�� �����Ǿ� ������ �𸣴� ������ �ݴ´�
    for(i=0 ; i<MAX_SOCK ; i++)
	close(i);
    // end of daemon

    // tcp_listen(host, port, backlog) �Լ� ȣ��
    listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);
        for(i=0; i<num_chat; i++)
            FD_SET(clisock_list[i], &read_fds);
        maxfdp1 = getmax() + 1;     // maxfdp1 �� ���
        puts("wait for client");
        if(select(maxfdp1, &read_fds,NULL,NULL,NULL)<0)
            errquit("select fail");

        if(FD_ISSET(listen_sock, &read_fds)) {
		accp_sock=accept(listen_sock, (struct sockaddr *)&cliaddr, &addrlen);

 		if(accp_sock == -1)
        	        errquit("accept fail");
	
		if(num_chat > MAX_SOCK) {
			close(accp_sock);
		}
			
		else {
			addClient(accp_sock,&cliaddr);
	        	send(accp_sock, START_STRING, strlen(START_STRING), 0);
	        	printf("%d��° ����ڰ� �߰��Ǿ����ϴ�.\n", num_chat);
				
		} 
	}
	//�α��� �ҷ�����

        for(i=0;i<num_chat;i++){
	    if(FD_ISSET(clisock_list[i], &read_fds)){
		nbyte=recv(clisock_list[i], buf, MAXLINE, 0);

		// Ŭ���̾�Ʈ�� �Է��� ���� ���ؼ� "login"�̸� �Լ� ȣ��
	        if(strstr(buf, "login")!=NULL){
	                Login(clisock_list[i]);
			sprintf(buftmp, "(%s ȸ����", db[clisock_list[i]].ID);
			for(i=0 ; i<num_chat ; i++){
				send(clisock_list[i], buftmp, strlen(buftmp), 0);
				send(clisock_list[i], LOGON, strlen(LOGON),0);
			}
			continue;
        	}
		// "join"�̸� �Լ� ȣ��
	        if(strstr(buf, "join")!=NULL){
        	        Join(clisock_list[i]);
	        }

		if(nbyte<= 0) {
                    removeClient(i);    // Ŭ���̾�Ʈ�� ����
                    continue;
                }
                buf[nbyte] = 0;

                // ���Ṯ�� ó��
                if(strstr(buf, EXIT_STRING) != NULL) {
                    removeClient(i);    // Ŭ���̾�Ʈ�� ����
                    continue;
                }
		// �α׾ƿ� ����ó��
		if(strstr(buf, LOGOUT_STRING) != NULL) {
			db[clisock_list[i]].login_check=0;
			send(clisock_list[i], LOGOUT, strlen(LOGOUT), 0);
			sprintf(buftmp, "(%s ȸ����", db[clisock_list[i]].ID);
			for(i=0 ; i<num_chat ; i++){
				send(clisock_list[i], buftmp, strlen(buftmp), 0);
				send(clisock_list[i], LOGOUT, strlen(LOGOUT),0);
			}
			continue;
		}
		// ȸ��Ż�� ����ó��
	if(db[clisock_list[i]].login_check==1){
		if(strstr(buf, "remove")!=NULL){
			Remove(clisock_list[i]);
			continue;
		}
		if(strstr(buf, "list")!=NULL){
			List(clisock_list[i]);
			continue;
		}
                // ��� ä�� �����ڿ��� �޽��� ���
		// ID�� ��ȭ������ �����ϱ� ���� ���ȣ([]) ���
		sprintf(buftmp, "[%s]", db[clisock_list[i]].ID);

                for (i = 0; i < num_chat; i++){
			// ID�� ��ȭ������ ���ÿ� ����
		if(db[clisock_list[i]].login_check==1){
			send(clisock_list[i], buftmp, strlen(buftmp), 0);
       	   		send(clisock_list[i], buf, MAXLINE, 0);
			}		
		}
                printf("%s\n", buf);
          }    
	}
       }
    }  // end of while
	return 0;
}

// Ż�� �Լ�
void Remove(int clnt)
{

	int j=0, recv_len, check=0, i=0;
        char buf[MAXLINE+1];
	char null[MAXLINE];
        FILE *fp;
	FILE *fw;

	// ID�� ����Ǿ� �ִ� ���� ȣ��
        if((fp = fopen("data.txt","a+"))==NULL){
                printf("File open error....");
                exit(-1);
        }
	// ���ο� ������ ���� ����� ����� ����
	fw = fopen("rem.txt", "a+");
	send(clnt, ID_REMOVE, strlen(ID_REMOVE), 0);
        recv_len = recv(clnt, buf, MAXLINE, 0);
        buf[recv_len-1] = 0;
	// while�� ����
	// ���� �α��ε� �ڽ��� ID�� Ż���� �� �ֵ��� ����
	if(!strcmp(buf,db[clnt].ID)){

	        while(!feof(fp)){
			// ���Ͽ� �ִ� ȸ�� ����� ������
	                fscanf(fp, "%s", list[j]);
			// ���� �Է��� ID�� ���Ͽ�  ������� �ƹ� �ϵ� ���� ����
	                if(!strcmp(buf,list[j])){
	                }
			else{
				list[j][strlen(list[j])]='\n';
			// �����Ͽ� ȸ������� ��
				fwrite(list[j], strlen(list[j]), 1, fw);
			}
	                j++;
	        }// end while
		fclose(fp);

		// ���´� ���� ��������

		fp = fopen("data.txt","w");    // ���� �ʱ�ȭ
		fclose(fp);

		fp = fopen("data.txt", "a+");  // ���� ���� ���ؼ� ���� ����

		for(i=0 ; i<j-1 ; i++)
		{
			if(!strcmp(buf,list[i])){
			}
			else{
			// ���� ���Ͽ� �� ���Ͽ� �ִ� ������ ��
				fscanf(fw, "%s", list[i]);
				fwrite(list[i], strlen(list[i]), 1, fp);
				check = 1;
			}
		}
	}
	    else{
		send(clnt, ID_NULL, strlen(ID_NULL), 0);
		}	
 
		if(check==1){
			send(clnt, ID_OUT, strlen(ID_OUT), 0);
		}
		fclose(fw);
		fclose(fp);
		// �纻 ���� �ʱ�ȭ
		fw = fopen("rem.txt", "w");
		// list �迭 �ʱ�ȭ
		for(j=0 ; j<=100 ; j++) {
	                list[j-1][MAX_SOCK] = '\0';
	        }
		// Ż���� ȸ�����Դ� ä�ñ����� ���� ����
		db[clnt].login_check = 0;
}

// �α��� �Լ�
void Login(int clnt)
{
	int i=0, j=0, recv_len, login_check = 0,len;
	char buf[MAXLINE+1];
	FILE *fp;
	if((fp = fopen("data.txt","r"))==NULL){
		printf("File open error....");
		exit(-1);
	}
	send(clnt, ID_INPUT, strlen(ID_INPUT), 0);
	recv_len = recv(clnt, buf, MAXLINE, 0); 

	buf[recv_len-1] = 0;
	
	// while of start
	while(!feof(fp)){		
		fscanf(fp, "%s", list[i]);

		// �Է��� ID�� ȸ����Ͽ� ������� ä�� ���
		if(!strcmp(buf,list[i])){
			send(clnt, LOGON, strlen(LOGON), 0);
			db[clnt].login_check = 1;
			strcpy(db[clnt].ID, buf);
			break;
		}
		i++;
		
	}// end while
	// �α��� ���û
	if(db[clnt].login_check==0)
		send(clnt, LOGFAIL, strlen(LOGFAIL), 0);

	fclose(fp);

}

// ����Ʈ �Լ�
void List(int clnt)
{
	int i=0;
	send(clnt, USERNAME, strlen(USERNAME), 0);
	for(i=3 ; i<num_chat+4 ; i++){
		send(clnt, db[i].ID, strlen(db[i].ID), 0);
		send(clnt, ENTER, strlen(ENTER), 0);
	}
}


// ���� �Լ�
void Join(int clnt)
{
	int j=0, recv_len, check=0;
        char buf[MAXLINE+1];
        FILE *fp;
        if((fp = fopen("data.txt","a+"))==NULL){
                printf("File open error....");
                exit(-1);
        }
        send(clnt, ID_REG, strlen(ID_REG), 0);
        recv_len = recv(clnt, buf, MAXLINE, 0);
        buf[recv_len-1] = 0;
        while(!feof(fp)){
                fscanf(fp, "%s", list[j]);
		// �̹� ������ ȸ���� ��� �޽��� ó��
                if(!strcmp(buf,list[j])){
                        send(clnt, ID_ID, strlen(ID_ID), 0);
			check = 1;
                        break;
		}
                j++;                                                                       
        }// end while
	buf[recv_len-1] = '\n';
	buf[recv_len] = 0;
	// ������ ȸ���� ���� ȸ����Ͽ� ����
	if(check==0){
		fwrite(buf, recv_len, 1, fp);
		send(clnt, ID_RESULT, strlen(ID_RESULT), 0);
	}

	for(j=0 ; j<=100 ; j++) {
                list[j-1][MAX_SOCK] = '\0';
        }
        fclose(fp);
}

// ���ο� ä�� ������ ó��
void addClient(int s, struct sockaddr_in *newcliaddr) {
    char buf[20];
    inet_ntop(AF_INET,&newcliaddr->sin_addr,buf,sizeof(buf));
    printf("new client: %s\n",buf);
     // ä�� Ŭ���̾�Ʈ ��Ͽ� �߰�
    clisock_list[num_chat] = s;
    num_chat++;
}

// ä�� Ż�� ó��
void removeClient(int s) {
    close(clisock_list[s]);
    if(s != num_chat-1)
        clisock_list[s] = clisock_list[num_chat-1];
    num_chat--;
    printf("ä�� ������ 1�� Ż��. ���� ������ �� = %d\n", num_chat);
}

// �ִ� ���Ϲ�ȣ ã��
int getmax() {
    // Minimum ���Ϲ�ȣ�� ���� ���� ������ listen_sock
    int max = listen_sock;
    int i;
    for (i=0; i < num_chat; i++)
        if (clisock_list[i] > max )
            max = clisock_list[i];

    return max;
}

// listen ���� ���� �� listen
// listen ���� ���� �� listen
int  tcp_listen(int host, int port, int backlog) {
    int sd;
    struct sockaddr_in servaddr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1) {
        perror("socket fail");
        exit(1);
    }
    // servaddr ����ü�� ���� ����
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(host);
    servaddr.sin_port = htons(port);
    if (bind(sd , (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind fail");  exit(1);
    }
    // Ŭ���̾�Ʈ�κ��� �����û�� ��ٸ�
    listen(sd, backlog);
    return sd;
}
