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
#include "struct_server.h"

void close_server_fd(int *fd)
{
    if (*fd != -1) {
        if (close(*fd) == -1)
            perror("close");
        *fd = -1;
    }
}

void server_accept(struct server *s)
{
    printf("{server.c}:[server_accept]\n");
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
    s->number_of_sessions++;
    s->redraw = 1;

    /* FIX IT ??*/
    if (s->number_of_sessions == NUMBER_OF_PLAYERS)
        close_server_fd(&s->fds[LISTEND_FD].fd);
}

void server_notification(struct server *s)
{
    printf("{server.c}:[server_notification]\n");
    int size;
    uint64_t u = 1;
    size = write(s->fds[EVENT_FD].fd, &u, sizeof(u));
    if (size != sizeof(u))
        perror("write efd");
}

void server_reset_notification(struct server *s)
{
    printf("{server.c}:[server_reset_notification]\n");
    int size;
    uint64_t u = 1;
    size = read(s->fds[EVENT_FD].fd, &u, sizeof(u));
    if (size != sizeof(u))
        perror("read efd");
    s->fds[1].revents = 0;
}

void server_poll_events(struct server *s)
{
    printf("{server.c}:[server_poll_events]\n");
    if (s->state == ss_end)
        return;

    if (s->fds[LISTEND_FD].revents & POLLIN)
        server_accept(s);

    if (s->fds[EVENT_FD].revents & POLLIN)
        server_reset_notification(s);

    for (int i = SERVER_FDS; i < (int)s->nfds; i++) {
        if (s->fds[i].revents & POLLIN) {
            session_do_read(s->sessions[i - SERVER_FDS]);

            /*
            FIX IT ??
            if (s->sessions[i - SERVER_FDS]->state == ss_disconnect)
                server_close_session(s, i);
            */
            if (s->g && s->sessions[i - SERVER_FDS]->state == ss_command)
                game_handle_command(s->g, s->sessions[i - SERVER_FDS]);
        }
    }
}

void server_lobby(struct server *s)
{
    printf("{server.c}:[server_lobby]\n");
    int ready = 0;

    for (int i = 0; i < NUMBER_OF_PLAYERS; i++)
        if (s->sessions[i] && s->sessions[i]->state == ss_ready)
            ready++;

    if (s->ready != ready) {
        s->redraw = 1;
        s->ready = ready;
    }

    if (s->redraw) {
        s->buf_size = snprintf(s->buf, BUF_SIZE, LOBBY_TEXT,
                 s->number_of_sessions, NUMBER_OF_PLAYERS, s->ready, NUMBER_OF_PLAYERS);
    }

    if (s->ready == NUMBER_OF_PLAYERS) {
        printf("notify server that we are in game\n");
        s->state = ss_game_init;
        server_notification(s);
    }
}

void server_draw_rules(struct server *s)
{
    printf("{server.c}:[server_draw_rules]\n");
    strcpy(s->buf, GAME_RULES);
    s->buf_size = snprintf(s->buf, BUF_SIZE, GAME_RULES, ROUND_TIME);
}

void server_draw_game_info(struct server *s)
{
    printf("{server.c}:[server_draw_game_info]\n");
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, GAME_INFO,
                   s->g->attacker, s->g->defender, card_suit_imgs[s->g->trump_suit],
                   s->g->t.number_of_round,
                   s->g->cards_count);
}

void server_draw_attack_table(struct server *s)
{
    printf("{server.c}:[server_draw_attack_table]\n");
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
}

void server_draw_defense_table(struct server *s)
{
    printf("{server.c}:[server_draw_defense_table]\n");
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
}

void server_draw_hand(struct server *s)
{
    printf("{server.c}:[server_draw_hand]\n");
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "\nHand:\n");
}

void server_draw_game(struct server *s)
{
    printf("{server.c}:[server_draw_game]\n");
    if (s->redraw) {
        server_draw_rules(s);
        server_draw_game_info(s);
        server_draw_attack_table(s);
        server_draw_defense_table(s);
        server_draw_hand(s);
    }
}

void server_game_init(struct server *s)
{
    printf("{server.c}:[server_game_init]\n");
    game_init(s);
    for (int i = 0; i < s->number_of_sessions; i++)
        session_change_state(s->sessions[i], ss_error);
    s->state = ss_game;
    s->redraw = 1;
    server_draw_game(s);
}

void server_game(struct server *s)
{
    printf("{server.c}:[server_game]\n");
    game_processing(s->g);
    server_draw_game(s);
    if (s->g->state == gs_game_end || s->g->state == gs_game_end_draw) {
        s->state = ss_end;
        server_notification(s);
    }
}

void server_end(struct server *s)
{
    printf("{server.c}:[server_end]\n");
    if (s->g->state == gs_game_end)
        s->buf_size = snprintf(s->buf, BUF_SIZE, "\033[2J\033[H" \
                               "=====GAME OVER=====\nPlayer %d lose", s->g->defender);
    else
        s->buf_size = snprintf(s->buf, BUF_SIZE, "\033[2J\033[H" \
                               "=====GAME OVER=====\nDRAW");
}

void server_fsm(struct server *s)
{
    printf("{server.c}:[server_fsm]\n");
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
    case ss_end:
        server_end(s);
        break;
    }
}

void server_redraw(struct server *s)
{
    printf("{server.c}:[server_redraw]\n");
    if (!s->redraw)
        return;
    for (int i = 0; i < s->number_of_sessions; i++) {
        if (s->sessions[i])
            session_do_write(s, i);
    }
    s->redraw = 0;
}

void server_close_session(struct server *s, int index)
{
    printf("{server.c}:[server_close_session]\n");
    if (!s->sessions[index])
        return;

    if (close(s->sessions[index]->fd) == -1)
        perror("close");

    free(s->sessions[index]->buf);
    free(s->sessions[index]);
    s->sessions[index] = NULL;
}

int server_close(struct server *s)
{
    printf("{server.c}:[server_close]\n");
    if (s->state != ss_end)
        return 0;

    for (int i = 0; i < s->number_of_sessions; i++)
        server_close_session(s, i);
    free(s->g);
    close_server_fd(&s->fds[LISTEND_FD].fd);
    close_server_fd(&s->fds[EVENT_FD].fd);
    return 1;
}

void server_start(struct server *s)
{
    printf("{server.c}:[server_start]\n");
    int ret;

    for (;;) {
        printf("poll() waiting...\n");
        ret = poll(s->fds, s->nfds, TIMEOUT);    
        if (ret == -1)
            errRet("poll");

        server_poll_events(s);
        server_fsm(s);
        server_redraw(s);
        if (server_close(s))
            break;
    }
}
