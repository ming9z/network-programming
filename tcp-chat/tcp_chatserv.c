//-------------------------------------------------------------
// 파일명 : tcp_chatserv.c
// 기  능 : 채팅 참가자 관리, 채팅 메시지 수신 및 방송
// 컴파일 : cc -o chat_server tcp_chatserv.c
// 사용법 : chat_server 4001
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
#define MAX_SOCK 512 // 솔라리스의 경우 64

char *EXIT_STRING = "exit";   // 클라이언트의 종료요청 문자열
char *LOGOUT_STRING = "logout";
char *START_STRING = "  (로그인:login   회원가입:join)\n";
char *LOGOUT = " 로그아웃 되었습니다.)\n";
    // 클라이언트 환영 메시지
int maxfdp1;                // 최대 소켓번호 +1
int num_chat = 0;          // 채팅 참가자 수
int clisock_list[MAX_SOCK]; // 채팅에 참가자 소켓번호 목록
int listen_sock;         // 서버의 리슨 소켓
char list[100][MAX_SOCK];
char *ID_REG = "  (가입할 ID를 입력하세요) : ";
char *ID_ID = "  (이미 가입된 아이디가 있습니다.)\n  (join를 입력후 다른 아이디를 입력해 주세요) : ";
char *ID_INPUT = "  (ID를 입력하세요.)\n";
char *LOGON = " 로그인 되었습니다.)\n";
char *LOGFAIL = "  (로그인이 되지 않았습니다.)\n  (가입 또는 로그인 해주세요)\n";
char *ID_RESULT = "  (가입되었습니다.)\n  (로그인 해주세요)\n";
char *ID_REMOVE = "  (탈퇴하시겠습니까?)\n  (ID를 입력하세요)\n";
char *ID_OUT = "  (정상적으로 탈퇴되었습니다.)\n";
char *ID_NULL = "  (잘못 입력하셨습니다. 자신의 ID를 입력하세요.)\n";
char *USERNAME = "  (현재 로그인된 사용자 리스트입니다.)\n";
char *ENTER = " \n";

struct user{
	char ID[20];
	int cli_sock;
	int login_check;
	int chat_check;
	};
struct user db[MAX_SOCK+1];


// 새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr);
int getmax();               // 최대 소켓 번호 찾기
void removeClient(int s);    // 채팅 탈퇴 처리 함수
int tcp_listen(int host, int port, int backlog); // 소켓 생성 및 listen
void errquit(char *mesg) { perror(mesg); exit(1); }
void Login(int clnt);        // 로그인 함수 호출
void Logout(int clnt);
void Join(int clnt);        // 가입함수 호출
void Remove(int clnt);      // 탈퇴함수 호출 
void List(int clnt);		// 리스트함수 호출


int main(int argc, char *argv[]) {
    struct sockaddr_in cliaddr;
    char buf[MAXLINE+1], buftmp[MAXLINE+1], *bufmsg;
    int i, j, nbyte, accp_sock, addrlen = sizeof(struct
        sockaddr_in), namelen;
    pid_t pid;
    struct sigaction sact;
    fd_set read_fds;  // 읽기를 감지할 fd_set 구조체

    for(i=0 ; i<20 ; i++){
	db[i].ID[i] = '\0';
    }

    if(argc != 2) {
        printf("사용법 :%s port\n", argv[0]);
        exit(0);
    }

    
    if((pid==fork())!=0)
	exit(0);   // 부모 프로세스는 종료시킨다, 즉, 자식 프로세스만 실행
    setsid();      

    sact.sa_handler = SIG_IGN;
    sact.sa_flags = 0;
    sigemptyset(&sact.sa_mask);
    sigaddset(&sact.sa_mask, SIGHUP);
    sigaction(SIGHUP, &sact, NULL);

    if((pid==fork())!=0)    // 다시 자식프로세스(손자)를 만든다
	exit(0);	    // 부모프로세스는 종료시킨다
//    chdir("/");
    umask(0);
    // 혹시 개설되어 있을지 모르는 소켓을 닫는다
    for(i=0 ; i<MAX_SOCK ; i++)
	close(i);
    // end of daemon

    // tcp_listen(host, port, backlog) 함수 호출
    listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);
        for(i=0; i<num_chat; i++)
            FD_SET(clisock_list[i], &read_fds);
        maxfdp1 = getmax() + 1;     // maxfdp1 재 계산
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
	        	printf("%d번째 사용자가 추가되었습니다.\n", num_chat);
				
		} 
	}
	//로그인 불러오기

        for(i=0;i<num_chat;i++){
	    if(FD_ISSET(clisock_list[i], &read_fds)){
		nbyte=recv(clisock_list[i], buf, MAXLINE, 0);

		// 클라이언트가 입력한 값을 비교해서 "login"이면 함수 호출
	        if(strstr(buf, "login")!=NULL){
	                Login(clisock_list[i]);
			sprintf(buftmp, "(%s 회원이", db[clisock_list[i]].ID);
			for(i=0 ; i<num_chat ; i++){
				send(clisock_list[i], buftmp, strlen(buftmp), 0);
				send(clisock_list[i], LOGON, strlen(LOGON),0);
			}
			continue;
        	}
		// "join"이면 함수 호출
	        if(strstr(buf, "join")!=NULL){
        	        Join(clisock_list[i]);
	        }

		if(nbyte<= 0) {
                    removeClient(i);    // 클라이언트의 종료
                    continue;
                }
                buf[nbyte] = 0;

                // 종료문자 처리
                if(strstr(buf, EXIT_STRING) != NULL) {
                    removeClient(i);    // 클라이언트의 종료
                    continue;
                }
		// 로그아웃 문자처리
		if(strstr(buf, LOGOUT_STRING) != NULL) {
			db[clisock_list[i]].login_check=0;
			send(clisock_list[i], LOGOUT, strlen(LOGOUT), 0);
			sprintf(buftmp, "(%s 회원이", db[clisock_list[i]].ID);
			for(i=0 ; i<num_chat ; i++){
				send(clisock_list[i], buftmp, strlen(buftmp), 0);
				send(clisock_list[i], LOGOUT, strlen(LOGOUT),0);
			}
			continue;
		}
		// 회원탈퇴 문자처리
	if(db[clisock_list[i]].login_check==1){
		if(strstr(buf, "remove")!=NULL){
			Remove(clisock_list[i]);
			continue;
		}
		if(strstr(buf, "list")!=NULL){
			List(clisock_list[i]);
			continue;
		}
                // 모든 채팅 참가자에게 메시지 방송
		// ID와 대화내용을 구분하기 위해 대괄호([]) 사용
		sprintf(buftmp, "[%s]", db[clisock_list[i]].ID);

                for (i = 0; i < num_chat; i++){
			// ID와 대화내용을 동시에 전송
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

// 탈퇴 함수
void Remove(int clnt)
{

	int j=0, recv_len, check=0, i=0;
        char buf[MAXLINE+1];
	char null[MAXLINE];
        FILE *fp;
	FILE *fw;

	// ID가 저장되어 있는 파일 호출
        if((fp = fopen("data.txt","a+"))==NULL){
                printf("File open error....");
                exit(-1);
        }
	// 새로운 파일을 열어 사용자 목록을 저장
	fw = fopen("rem.txt", "a+");
	send(clnt, ID_REMOVE, strlen(ID_REMOVE), 0);
        recv_len = recv(clnt, buf, MAXLINE, 0);
        buf[recv_len-1] = 0;
	// while문 시작
	// 현재 로그인된 자신의 ID만 탈퇴할 수 있도록 조건
	if(!strcmp(buf,db[clnt].ID)){

	        while(!feof(fp)){
			// 파일에 있는 회원 목록을 가져옴
	                fscanf(fp, "%s", list[j]);
			// 현재 입력한 ID가 파일에  있을경우 아무 일도 하지 않음
	                if(!strcmp(buf,list[j])){
	                }
			else{
				list[j][strlen(list[j])]='\n';
			// 새파일에 회원목록을 씀
				fwrite(list[j], strlen(list[j]), 1, fw);
			}
	                j++;
	        }// end while
		fclose(fp);

		// 보냈던 파일 가져오기

		fp = fopen("data.txt","w");    // 파일 초기화
		fclose(fp);

		fp = fopen("data.txt", "a+");  // 새로 쓰기 위해서 파일 오픈

		for(i=0 ; i<j-1 ; i++)
		{
			if(!strcmp(buf,list[i])){
			}
			else{
			// 원본 파일에 새 파일에 있는 내용을 씀
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
		// 사본 파일 초기화
		fw = fopen("rem.txt", "w");
		// list 배열 초기화
		for(j=0 ; j<=100 ; j++) {
	                list[j-1][MAX_SOCK] = '\0';
	        }
		// 탈퇴한 회원에게는 채팅권한을 주지 않음
		db[clnt].login_check = 0;
}

// 로그인 함수
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

		// 입력한 ID가 회원목록에 있을경우 채팅 허락
		if(!strcmp(buf,list[i])){
			send(clnt, LOGON, strlen(LOGON), 0);
			db[clnt].login_check = 1;
			strcpy(db[clnt].ID, buf);
			break;
		}
		i++;
		
	}// end while
	// 로그인 재요청
	if(db[clnt].login_check==0)
		send(clnt, LOGFAIL, strlen(LOGFAIL), 0);

	fclose(fp);

}

// 리스트 함수
void List(int clnt)
{
	int i=0;
	send(clnt, USERNAME, strlen(USERNAME), 0);
	for(i=3 ; i<num_chat+4 ; i++){
		send(clnt, db[i].ID, strlen(db[i].ID), 0);
		send(clnt, ENTER, strlen(ENTER), 0);
	}
}


// 가입 함수
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
		// 이미 가입한 회원일 경우 메시지 처리
                if(!strcmp(buf,list[j])){
                        send(clnt, ID_ID, strlen(ID_ID), 0);
			check = 1;
                        break;
		}
                j++;                                                                       
        }// end while
	buf[recv_len-1] = '\n';
	buf[recv_len] = 0;
	// 가입한 회원에 대해 회원목록에 저장
	if(check==0){
		fwrite(buf, recv_len, 1, fp);
		send(clnt, ID_RESULT, strlen(ID_RESULT), 0);
	}

	for(j=0 ; j<=100 ; j++) {
                list[j-1][MAX_SOCK] = '\0';
        }
        fclose(fp);
}

// 새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr) {
    char buf[20];
    inet_ntop(AF_INET,&newcliaddr->sin_addr,buf,sizeof(buf));
    printf("new client: %s\n",buf);
     // 채팅 클라이언트 목록에 추가
    clisock_list[num_chat] = s;
    num_chat++;
}

// 채팅 탈퇴 처리
void removeClient(int s) {
    close(clisock_list[s]);
    if(s != num_chat-1)
        clisock_list[s] = clisock_list[num_chat-1];
    num_chat--;
    printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 = %d\n", num_chat);
}

// 최대 소켓번호 찾기
int getmax() {
    // Minimum 소켓번호는 가정 먼저 생성된 listen_sock
    int max = listen_sock;
    int i;
    for (i=0; i < num_chat; i++)
        if (clisock_list[i] > max )
            max = clisock_list[i];

    return max;
}

// listen 소켓 생성 및 listen
// listen 소켓 생성 및 listen
int  tcp_listen(int host, int port, int backlog) {
    int sd;
    struct sockaddr_in servaddr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1) {
        perror("socket fail");
        exit(1);
    }
    // servaddr 구조체의 내용 세팅
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(host);
    servaddr.sin_port = htons(port);
    if (bind(sd , (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind fail");  exit(1);
    }
    // 클라이언트로부터 연결요청을 기다림
    listen(sd, backlog);
    return sd;
}
