/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/* 
 * serve_dynamic - 동적 콘텐츠를 제공하기 위해 자식 프로세스를 생성하고
 * 해당 자식 프로세스 내에서 CGI 프로그램을 실행하는 함수
 */
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method) {
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* 응답의 첫 줄과 서버 정보를 클라이언트에게 보냄 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (!strcasecmp(method, "HEAD"))  // 과제 11.11 - HEAD면 바로 return
    return;

  if (Fork() == 0) { // 자식 프로세스
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // QUERY_STRING 환경 변수 초기화
    Dup2(fd, STDOUT_FILENO);  // 자식의 표준 출력을 클라이언트와 연결된 fd로 리다이렉션
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  // 부모 프로세스
  Wait(NULL); // 자식 프로세스의 종료 대기
}

/* serve_static - 정적 콘텐츠를 제공하는 함수 */
void serve_static(int fd, char *filename, int filesize, char *method) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);   // 파일 확장자에 따라 MIME 타입 결정
  // sprintf(buf, "HTTP/1.1 200 OK\r\n");
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  if (!strcasecmp(method, "HEAD"))    // 과제 11.11 - head면 바로 return
    return;

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  srcp = (char*)malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
  // Munmap(srcp, filesize);
}

/* get_filetype - 받아야 하는 filetype을 명시하는 함수 */
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".MP4"))
    strcpy(filetype, "image/MP4");
  else
    strcpy(filetype, "text/plain");
}

/* 
 * parse_uri - 클라이언트가 요청한 URI를 파일 이름과 선택적인 CGI 문자열로 파싱하는 함수
 * tiny 서버의 모든 동적 컨텐츠를 위한 실행파일은 cgi-bin이라는 디렉토리에 넣는 방식
 * 따라서 uri에 cgi-bin이라는 경로가 있는지 확인해서 정적 or 동적 보내야하는지 판단
 */
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  
  // 요청이 정적 콘텐츠라면
  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");    // uri에 cgi-bin과 일치하는 문자열이 없다면 cgiargs에는 빈 문자열 저장
    
    // 상대 리눅스 경로이름으로 변환 (./index.html과 같은)
    strcpy(filename, ".");
    strcat(filename, uri);

    if (uri[strlen(uri) - 1] == '/')  // uri가 '/' 문자로 끝난다면
      strcat(filename, "home.html");  // 기본 파일 이름 추가
    return 1;
  }
  // 요청이 동적 콘텐츠라면
  else {
    ptr = index(uri, '?');  // ? 뒤에 인자 있음
    if (ptr) {    
      strcpy(cgiargs, ptr + 1); // cgi 인자 카피
      *ptr = '\0';  // 뒤에 인자 떼기 위해 string NULL 표시
    }
    else  // ?가 없다면₩
      strcpy(cgiargs, "");  // cgiargs에 빈 문자열 저장

    strcpy(filename, ".");  // 경로 시작점 지정
    strcat(filename, uri);  // uri 붙여줌
    return 0;
  }
}

/* read_requesthdrs - request header를 읽는 함수 */
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);  // 요청 라인 읽기
  // 빈 텍스트 라인 확인하기 위해 캐리지 리턴 + 줄바꿈
  // strcmp 함수를 사용해서 현재 읽은 줄(buf)이 \r\n과 같지 않을 때까지 계속해서 읽는다. 같으면 0 -> while문 탈출
  while (strcmp(buf, "\r\n")) {
    rio_readlineb(rp, buf, MAXLINE);  // 요청 헤더의 나머지 부분 읽는 작업
    printf("%s", buf);  // 각 헤더 필드를 읽어 들인 후 화면 출력
  }
  return;
}

/* 
 * clienterror - Tiny 서버에서 발생한 일부 에러를 처리하고
 *               이를 클라이언트에 보고하는 함수
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/* doit - 하나의 HTTP 트랜잭션을 처리하는 함수 */
void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);            // read 버퍼 초기화

  /* 요청 라인 읽기 */
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);    // GET/godzilla.gifHTTP/1.1
  sscanf(buf, "%s %s %s", method, uri, version);  // 문자열 파싱 -> GET /godzilla.gif HTTP/1.1

  // Tiny는 GET 메서드만 지원
  // if (strcasecmp(method, "GET"))
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))  // 과제 11.11
  {
    // 0이면 같다는 의미 -> if문 안: get이 아닐 때
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);   // request header

  // uri를 파일 이름과 cgi arg문자열로 구문 분석하고 요청이 정적인지 동적인지 나타내는 플래그 설정
  is_static = parse_uri(uri, filename, cgiargs);
  // 파일이 디스크에 없는 경우 클라이언트에게 오류 메세지 전송
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  if (is_static) {    // 정적 콘텐츠이면
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {  // 일반 파일, 읽기 권한 둘 중 하나라도 아니면 에러
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read this file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method); // 정적 콘텐츠 클라이언트에게 제공
  }

  else {    // 동적 콘텐츠이면
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {  // 파일이면서 실행 권한 가능한지 확인
      clienterror(fd, filename, "403", "Forbidden",               // 아니면 에러
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);   // 동적 콘텐츠 클라이언트에게 제공
  }
}

int main(int argc, char **argv) {
  int listenfd, connfd;   // 디스크립터
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  // 연결 대기 무한 루프
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 요청 수락, line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}