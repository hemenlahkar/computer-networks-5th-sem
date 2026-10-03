#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_SIZE 4096

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }

    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int rc = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

    int fd = -1;
    for (rp = res; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        perror("connect");
        return 1;
    }

    printf("UPD client ready. Type lines; Ctrl-D to quit.\n");

    char line[BUF_SIZE];
    while (fgets(line, sizeof line, stdin))
    {
        size_t len = strlen(line);
        if (write(fd, line, len) < 0) {
            perror("send");
            break;
        }

        ssize_t n = read(fd, line, sizeof line);
        if (n < 0) {
            perror("recv");
            break;
        }
        fwrite(line, 1, (size_t)n, stdout);
    }
    
    close(fd);
    return 0;
}
