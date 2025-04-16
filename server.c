#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/eventfd.h>

#include "server.h"
#include "cards.h"
#include "constants.h"
#include "session.h"
#include "game.h"

int server_init(struct server *s)
{
    printf("[server_init]\n");
    int ls, opt, efd;
    struct sockaddr_in addr;

    efd = eventfd(0, EFD_NONBLOCK);
    if (efd == -1) {
        perror("eventfd");
        return -1;
    }

    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == -1) {
        perror("socket");
        return -1;
    }

    opt = 1;
    if (setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(ls);
        return -1;
    }

    memset((void*)&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(ls);
        return -1;
    }

    if (listen(ls, NUMBER_OF_PLAYERS) == -1) {
        perror("listen");
        close(ls);
        return -1;
    }

    memset(s->fds, 0, sizeof(struct pollfd) * (NUMBER_OF_PLAYERS + SERVER_FDS));
    s->fds[LISTEND_FD].fd = ls;
    s->fds[LISTEND_FD].events = POLLIN;
    s->fds[EVENT_FD].fd = efd;
    s->fds[EVENT_FD].events = POLLIN;

    for (int i = 0; i < NUMBER_OF_PLAYERS; i++)
        s->sessions[i] = NULL;

    s->g = NULL;
    s->nfds = SERVER_FDS;
    s->buf_size = 0;
    s->state = ss_lobby;

    return 0;
}

void server_accept(struct server *s)
{
    printf("[server_accept]\n");
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    fd = accept(s->fds[0].fd, (struct sockaddr*)&addr, &addr_len);
    if (fd == -1)
        errRet("accept");

    s->fds[s->nfds].fd = fd;
    s->fds[s->nfds].events = POLLIN;
    s->sessions[s->nfds - SERVER_FDS] = session_init(fd, s->nfds - SERVER_FDS);
    s->nfds++;
    if (s->nfds == NUMBER_OF_PLAYERS + SERVER_FDS) {
        close(s->fds[0].fd);
        s->fds[0].fd = -1;
    }
}

void server_notification(struct server *s)
{
    printf("[server_notification]\n");
    int size;
    uint64_t u = 1;
    size = write(s->fds[EVENT_FD].fd, &u, sizeof(u));
    if (size != sizeof(u))
        perror("write efd");
}

void server_reset_notification(struct server *s)
{
    printf("[server_reset_notification]\n");
    int size;
    uint64_t u = 1;
    size = read(s->fds[EVENT_FD].fd, &u, sizeof(u));
    if (size != sizeof(u))
        perror("read efd");
    s->fds[1].revents = 0;
}

void server_lobby(struct server *s)
{
    printf("[server_lobby]\n");
    int ready = 0;

    for (int i = 0; i < NUMBER_OF_PLAYERS; i++)
        if (s->sessions[i] && s->sessions[i]->state == ss_ready)
            ready++;

    s->buf_size = snprintf(s->buf, BUF_SIZE, "\033[2J\033[80A\r"
             "Press ENTER to ready\n"
             "Number of player in lobby: (%lu/%d) ================= Ready: (%d/%d)\n", 
             s->nfds - SERVER_FDS, NUMBER_OF_PLAYERS, ready, NUMBER_OF_PLAYERS);

    if (ready == NUMBER_OF_PLAYERS) {
        printf("notify server that we are in game\n");
        s->state = ss_game_init;
        server_notification(s);
    }
}

void server_draw_game(struct server *s)
{
    printf("[server_draw_game]\n");
    s->buf_size = snprintf(s->buf + s->buf_size, BUF_SIZE, "\033[2J\033[80A\r");
    strcpy(s->buf + s->buf_size, GAME_RULES);
    s->buf_size += strlen(GAME_RULES);
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "==============================GAME===============================\n");
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "attacker: %d | defender: %d | trump suit: %c\n", 
                            s->g->attacker, s->g->defender, card_suit_imgs[s->g->trump_suit]);

    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "Remaining round time: %lu\n", s->g->round_time);
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "Desk size: %d\n", s->g->cards_count);
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "Table Attack:\n");
    for (int i = 0; i < DESK_SIZE; i++) {
        int card = s->g->table_attack[i];
        if (card == -1)
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "%d:%s%c ", i, " ", ' ');
        else
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "%d:%s%c ", i,
                                    card_imgs[card / NUMBER_OF_SUITS],
                                    card_suit_imgs[card % NUMBER_OF_SUITS]);
    }
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "\nTable Defensive:\n");
    for (int i = 0; i < DESK_SIZE; i++) {
        int card = s->g->table_defensive[i];
        if (card == -1)
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "%d:%s%c ", i, " ", ' ');
        else
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "%d:%s%c ", i,
                                    card_imgs[card / NUMBER_OF_SUITS],
                                    card_suit_imgs[card % NUMBER_OF_SUITS]);
    }
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "\nHand:\n");
}

void server_game_init(struct server *s)
{
    printf("[server_game_init]\n");
    s->g = game_init();
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++)
        session_change_state(s->sessions[i], ss_command);
    s->state = ss_game;
    server_draw_game(s);
}

void server_game(struct server *s)
{
    printf("[server_game]\n");
    /* FIX IT
     * no need to redraw all
     * add timer
     * */
    server_draw_game(s);
}

void server_fsm(struct server *s)
{
    printf("[server_fsm]\n");
    switch (s->state) {
    case ss_lobby:
        server_lobby(s);
        break;
    case ss_game_init:
        server_game_init(s);
        break;
    case ss_game:
        server_game(s);
        break;
    }
}

void server_start(struct server *s)
{
    printf("[server_start]\n");
    int ret;

    for (;;) {
        printf("poll() waiting...\n");
        ret = poll(s->fds, s->nfds, -1);    
        if (ret == -1)
            errRet("poll");

        if (s->fds[LISTEND_FD].revents & POLLIN)
            server_accept(s);

        if (s->fds[EVENT_FD].revents & POLLIN)
            server_reset_notification(s);


        for (int i = SERVER_FDS; i < NUMBER_OF_PLAYERS + SERVER_FDS; i++) {
            if (s->fds[i].revents & POLLIN) {
                s->fds[i].revents = 0;
                /* FIX IT 
                 * refactoring 
                 * */
                session_do_read(s->sessions[i - SERVER_FDS]);
                if (s->sessions[i - SERVER_FDS]->state == ss_command)
                    game_handle_command(s->g, s->sessions[i - SERVER_FDS]);
            }
        }

        /* FIX IT 
         * if session send wrong message dont redraw 
         * */
        server_fsm(s);

        for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
            if (s->sessions[i])
                session_do_write(s, i);
        }
    }
}
