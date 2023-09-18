#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

// int open(char *filename, int flags, mode_t mode);
int main() {
    // int fd = open("foo.txt", O_RDONLY, 0);
    int fd = open("foo.txt", O_WRONLY|O_APPEND, 0);

    if (fd == -1) {
        perror("open");
        return -1;
    }

    printf("%d\n", fd);
    return 0;

    // int close(int fd);
}