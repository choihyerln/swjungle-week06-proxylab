#include "csapp.c"

void *thread(void *vargp);

// main thread
int main() {  
    pthread_t tid;  // peer thread의 thread ID 저장 변수
    Pthread_create(&tid, NULL, thread, NULL);   // 새로운 peer thread 생성 -> 매인+피어 동시에 돌고있음
    Pthread_join(tid, NULL);    // 메인 쓰레드는 피어 쓰레드가 종료하기를 기다림
    exit(0);    // 현재 프로세스에서 돌고 있는 모든 쓰레드(이 경우에는 main)를 종료
}

// peer thread
void *thread(void *vargp) {
    printf("Hello, World!\n");
    return NULL;    // peer thread 종료
}