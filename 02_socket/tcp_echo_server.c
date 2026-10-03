#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "net.h"

#define BACKLOG 16
#define BUF_SIZE 4096

static void reap_children(int sig) {
    (void)sig;
    int saved = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved;
}

static void format_peer(const struct sockaddr_storage *ss, socklen_t slen, char *out, size_t out_size) {
    char host[NI_MAXHOST], serv[NI_MAXSERV];
    if (getnameinfo((const struct sockaddr *)ss, slen, host, sizeof host, serv, sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
        snprintf(out, out_size, "(unknown)");
    } else {
        snprintf(out, out_size, "%s:%s", host, serv);
    }
}

static void handle_client(int cfd) {
    char buf[BUF_SIZE];
    while(true) {
        ssize_t n = read(cfd, buf, sizeof buf);
        if (n == 0) return;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            return;
        }
        if (write_all(cfd, buf, (size_t)n) < 0) {
            if (errno != EPIPE) perror("write");
            return;
        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    install_sigpipe_ignore();
    signal(SIGCHLD, reap_children);
    
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rc = getaddrinfo(NULL, argv[1], &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return 1;
    }

    int lfd = -1;
    for (rp = res; rp; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd < 0) continue;

        int yes = 1;
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0) break;

        close(lfd);
        lfd = -1;
    }
    freeaddrinfo(res);

    if (lfd < 0) {
        perror("bind");
        return 1;
    }

    if (listen(lfd, BACKLOG) < 0)
    {
        perror("listen");
        close(lfd);
        return 1;
    }

    printf("TCP echo server listening on port %s (pid=%d)\n", argv[1], (int)getpid());

    while(true) {
        struct sockaddr_storage peer;
        socklen_t plen = sizeof peer;

        int cfd = accept(lfd, (struct sockaddr *)&peer, &plen);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        char pname[NI_MAXHOST + NI_MAXSERV + 2];
        format_peer(&peer, plen, pname, sizeof pname);
        printf("[server] connection from %s\n", pname);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(cfd);
            continue;
        }
        if (pid == 0) {
            close(lfd);

            printf("[child %d] serving %s\n", (int)getpid(), pname);
            handle_client(cfd);
            printf("[child %d] closing %s\n", (int)getpid(), pname);
            close(cfd);
            _exit(0);
        }
        close(cfd);
    }

    close(lfd);
    return 0;
}
