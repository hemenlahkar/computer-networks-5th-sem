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
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int rc = getaddrinfo(NULL, argv[1], &hints, &res);
    if (rc != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

    int fd = -1;
    for (rp = res; rp; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
            continue;
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0)
    {
        perror("bind");
        return 1;
    }

    printf("UPD echo server listening on port %s\n", argv[1]);

    while (true)
    {
        char buf[BUF_SIZE];
        struct sockaddr_storage peer;
        socklen_t plen = sizeof peer;

        ssize_t n = recvfrom(fd, buf, sizeof buf, 0, (struct sockaddr *)&peer, &plen);
        if (n < 0)
        {
            perror("recvform");
            continue;
        }

        if (sendto(fd, buf, (size_t)n, 0, (struct sockaddr *)&peer, plen) < 0)
        {
            perror("sendto");
        }
    }

    close(fd);
    return 0;
}
