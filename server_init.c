#include <stdint.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/eventfd.h>

#include "server_init.h"
#include "server.h"

int server_init_eventfd()
{
    printf("{server.c}:[server_init_eventfd]\n");
    int efd = eventfd(0, EFD_NONBLOCK);
    if (efd == -1)
        perror("eventfd");
    return efd;
}

int server_init_listen_socket()
{
    printf("{server.c}:[server_init_listen_socket]\n");
    int ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == -1) {
        perror("socket");
        return -1;
    }
    return ls;
}

int server_bind_listen_socket(int ls)
{
    printf("{server.c}:[server_bind_listen_socket]\n");
    int opt;
    struct sockaddr_in addr;

    opt = 1;
    if (setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        return -1;
    }

    memset((void*)&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(ls, QUEUE_SIZE) == -1) {
        perror("listen");
        return -1;
    }
    return 0;
}

void server_init_struct(struct server *s, int ls, int efd)
{
    printf("{server.c}:[server_init_struct]\n");
    memset(s->fds, 0, sizeof(struct pollfd) * (NUMBER_OF_PLAYERS + SERVER_FDS));
    s->fds[LISTEND_FD].fd = ls;
    s->fds[LISTEND_FD].events = POLLIN;
    s->fds[EVENT_FD].fd = efd;
    s->fds[EVENT_FD].events = POLLIN;

    for (int i = 0; i < NUMBER_OF_PLAYERS; i++)
        s->sessions[i] = NULL;

    s->g = NULL;
    s->nfds = SERVER_FDS;
    s->number_of_sessions = 0;
    s->buf_size = 0;
    s->ready = 0;
    s->state = ss_lobby;

}

void server_close_fd(int *fd)
{
    printf("{server.c}:[server_close_fd]\n");
    if (*fd != -1) {
        if (close(*fd) == -1)
            perror("close");
        *fd = -1;
    }
}

void server_cleanup(struct server *s)
{
    printf("{server.c}:[server_cleanup]\n");
    server_close_fd(&s->fds[LISTEND_FD].fd);
    server_close_fd(&s->fds[EVENT_FD].fd);
}


int server_init(struct server *s)
{
    printf("{server.c}:[server_init]\n");
    int ls, efd;

    s->fds[LISTEND_FD].fd = -1;
    s->fds[EVENT_FD].fd = -1;

    if ((efd = server_init_eventfd()) == -1) {
        server_cleanup(s);
        return -1;
    }

    if ((ls = server_init_listen_socket()) == -1) {
        server_cleanup(s);
        return -1;
    }

    if (server_bind_listen_socket(ls) == -1) {
        server_cleanup(s);
        return -1;
    }

    server_init_struct(s, ls, efd);

    return 0;
}
