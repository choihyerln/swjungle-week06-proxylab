#include "csapp.c"
// #include "csapp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    int fd1, fd2;

    fd1 = Open("umask.txt", O_RDONLY, 0);
    Close(fd1);

    fd2 = Open("umask2.txt", O_RDONLY, 0);
    printf("fd2 = %d\n", fd2);
    exit(0);

    char c;
    while (Read(STDIN_FILENO, &c, 1) != 0) {
        Write(STDOUT_FILENO, &c, 1);
    }
    exit(0);
}