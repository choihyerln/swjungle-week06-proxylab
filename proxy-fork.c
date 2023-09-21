#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *new_version = "HTTP/1.0";

void doit(int fd);
void request(int p_clientfd, char *method, char *uri_ptos, char *hostname);
void response(int p_connfd, int p_clientfd);
int parse_uri(char *uri, char *hostname, char *port, char *uri_ptos);

void request(int p_clientfd, char *method, char *uri_ptos, char *hostname) {
  char buf[MAXLINE];
  printf("서버에게 헤더 요청: \n");
  sprintf(buf, "%s %s %s\n", method, uri_ptos, new_version);

  // sprintf(buf, "GET %s %s\r\n", uri_ptos, new_version);     // GET /index.html HTTP/1.0
  // sprintf(buf, "%sHost: %s\r\n", buf, hostname);                // Host: www.google.com     
  sprintf(buf, "%sUser Agent: %s", buf, user_agent_hdr);                // User-Agent: ~(bla bla)
  sprintf(buf, "%sConnections: close\r\n", buf);            // Connections: close
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);   // Proxy-Connection: close

  Rio_writen(p_clientfd, buf, (size_t)strlen(buf));
}

void response(int p_connfd, int p_clientfd) {
  char buf[MAX_CACHE_SIZE];
  ssize_t n;
  rio_t rio;
  printf("----p_clientfd-----");
  Rio_readinitb(&rio, p_clientfd);  // client 요청 읽기
  n = Rio_readnb(&rio, buf, MAX_CACHE_SIZE);
  printf("----p_connfd-----");
  Rio_writen(p_connfd, buf, n);
}

/*
 * parse_uri - client로부터 받은 GET request, Proxy에서 Server로의 GET request를 하기 위해 필요
 * 원주소 = GET http://www.google.com:80/index.html HTTP/1.1 
   파싱 전 (client로부터 받은 request line)
   => GET http://www.google.com:80/index.html HTTP/1.1

   파싱 결과
   - hostname = www.google.com
   - port = 80
   - uri_ptos = /index.html

   파싱 후 (server로 보낼 request line)
   => GET /index.html HTTP/11. 
 */
int parse_uri(char *uri, char *hostname, char *port, char *uri_ptos) {    
  char *ptr;

  if (!(ptr = strstr(uri, "://")))  // ptr = uri에서 첫번째로 나온 "://"의 시작 인덱스 포인터
    return -1;  // "http://"로 시작 안하면 -1
  ptr += 3;     // "://" 다음으로 포인터 이동
  strcpy(hostname, ptr);    // host = www.google.com:80/index.html

  if ((ptr = strchr(hostname, ':'))) {    // strchr() = 문자 하나만 찾는 함수
    *ptr = '\0';        // ':' 부분 NULL처리, host = www.google.com
    ptr += 1;           // ':' 다음으로 포인터 이동
    strcpy(port, ptr);  // port = 80/index.html
  }
  else {    // 만약 포트번호 없으면
    if((ptr = strchr(hostname, '/'))) {   // strchr() = host에서 '/'가 있는 위치를 찾아
      *ptr = '\0';    // '/' 부분 NULL처리
      ptr += 1;       // 포인터 '/' 다음으로 이동
    }
    strcpy(port, "80");   // port번호 기본 80으로 지정
  }

  // port = 80/index.html
  // port 번호에 / 있으면 그 부분 추출
  strcpy(uri_ptos, "/");  // uri_ptos '/'로 시작
  if ((ptr = strchr(port, '/'))) {  // "/" 있으면 ptr 설정
    *ptr = '\0';    // '/'부분 NULL처리
    ptr += 1;       // 그 다음으로 ptr 이동 (i)
    // proxy->server로 보낼 uri에 담기
    strcat(uri_ptos, ptr);    // uri_ptos = /index.html
  }
  return 0;
}

void doit(int p_connfd) {
  int p_clientfd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], hostname[MAXLINE], port[MAXLINE];
  char uri_ptos[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, p_connfd);      // rio버퍼와 proxy connfd 연결
  Rio_readlineb(&rio, buf, MAXLINE);  // rio(==proxy의 connfd)에 있는 한 줄(응답 라인)을 모두 buf로 옮김
  printf("프록시에게 헤더 요청: \n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);  // buf에서 문자열 3개를 읽어와 각각 method, uri, version이라는 문자열에 저장
  
  parse_uri(uri, hostname, port, uri_ptos);       // uri를 hostname / port / uri_ptos로 파싱

  p_clientfd = Open_clientfd(hostname, port);      // proxy의 clientfd 연결됨
  // p_clientfd에 request headers 저장과 동시에 server의 connfd에 쓰여짐
  request(p_clientfd, method, uri_ptos, hostname); 
  response(p_connfd, p_clientfd);

  Close(p_clientfd);
}

// 이 부분 생략해도 코드 돌아가긴 하는데.. 있으면 좀비 자식들 청소해주니까 있으면 장시간 돌아가는 서버에게 좋음
void sigchld_handler(int sig) {
  while (waitpid(-1, 0, WNOHANG) > 0);
  return;
}

int main(int argc, char **argv) {
  int listenfd, p_connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  } 

  Signal(SIGCHLD, sigchld_handler);
  listenfd = Open_listenfd(argv[1]);  // 해당 포트번호에 맞는 대기 소켓fd open
  // 클라이언트 요청이 들어올 때마다 새로 연결할 소켓 만들어 doit() 호출
  while(1) {
    clientlen = sizeof(clientaddr);
    // 클라에게서 받은 연결 요청을 accept, p_connfd = proxy의 connfd
    p_connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    // 연결 성공 메세지를 위해 Getnameinfo 호출하면서 hostname과 port가 채워짐
    // doit(p_connfd);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("프록시 연결 성공! (%s %s)\n", hostname, port);
    if (Fork() == 0) {
      Close(listenfd);
      doit(p_connfd);
      Close(p_connfd);
      exit(0);
    }
    Close(p_connfd);
  }
  return 0;
}