#include "csapp.h"

ssize_t rio_readn(int fd, void *usrbuf, size_t n)   // usrbuf: 시작 위치, n: 버퍼크기(최대 읽을 수 있는 바이트 수)
{
    size_t nleft = n;       // 남은 바이트 수 (읽어야 할)
    ssize_t nread;          // 읽은 바이트 수
    char *bufp = usrbuf;    // 현재 읽을 위치 포인터

    while (nleft > 0) {     // 읽을게 남아있다면 반복
	if ((nread = read(fd, bufp, nleft)) < 0) {      // 만약 read가 -1을 반환한다면 error
	    if (errno == EINTR) /* Interrupted by sig handler return */
		    nread = 0;      /* and call read() again */
	    else
		    return -1;      /* errno set by read() */ 
	}
	else if (nread == 0)    // read가 0을 반환한다면, 이는 파일의 끝 = EOF
	    break;              // 루프 종료하고 데이터 읽기 마침
	nleft -= nread;         // 남은 바이트 수에서 읽은 바이트 수 빼줌 = 남은 읽을 바이트 수 갱신
	bufp += nread;          // 다음 읽기 위치 지정
    }
    return (n - nleft);     // 실제 읽은 바이트 수(통신하면서 받은 실제 크기)
}