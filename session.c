#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "session.h"
#include "constants.h"
#include "game.h"
#include "server.h"
#include "cards.h"

struct session* session_init(int fd, int id)
{
    printf("{session.c}:[ss_init]\n");
    struct session *ss = malloc(sizeof(struct session));
    ss->fd = fd;
    ss->game_id = id;
    ss->buf = malloc(BUF_SIZE);
    ss->buf_size = 0;
    ss->state = ss_connected;
    return ss;
}

void session_change_state(struct session *ss, enum session_state state)
{
    printf("{session.c}:[session_change_state]\n");
    ss->state = state;
}

void session_connected(struct session *ss)
{
    printf("{session.c}:[session_connected]\n");
    if (strcmp(ss->buf, "\r\n") == 0) {
        ss->state = ss_ready;
        printf("client is ready\n");
    }
}

int is_number(char str)
{
    printf("{session.c}:[is_number]\n");
    if (str >= 0x30 && str <= 0x39)
        return 1;
    return 0;
}

int is_letter(char str)
{
    printf("{session.c}:[is_letter]\n");
    if (str >= 0x61 && str <= 0x7A)
        return 1;
    return 0;
}

int is_valid_command(char *str)
{
    printf("{session.c}:[is_valid_command]\n");
    for (; *str; str++) {
        if (*str == '\r' || *str == '\n') {
            *str = '\0';
            continue;
        }
        if (*str == ' ')
            continue;

        if (!is_number(*str) && !is_letter(*str))
            return 0;
    }
    return 1;
}

void session_command(struct session *ss)
{
    printf("{session.c}:[session_command]\n");
    if (!is_valid_command(ss->buf)) {
        printf("?invalid command?\n");
        ss->state = ss_error;
        return;
    }
    ss->state = ss_command;
}

void session_fsm(struct session *ss)
{
    printf("{session.c}:[session_fsm]\n");
    switch (ss->state) {
    case ss_connected:
        session_connected(ss);
        break;
    case ss_ready:
        break;
    case ss_error:
    case ss_command:
        session_command(ss);
        break;
    }
}

void session_do_read(struct session *ss)
{
    printf("{session.c}:[session_do_read]\n");
    ss->buf_size = read(ss->fd, ss->buf, BUF_SIZE);
    if (ss->buf_size == -1) {
        perror("read");
        return;
    }
    ss->buf[ss->buf_size] = '\0';
    printf("server recived %d bytes, data: {%s}\n", ss->buf_size, ss->buf);

    session_fsm(ss);
}

void session_do_write(struct server *s, int index)
{
    printf("{session.c}:[session_do_write]\n");
    if (!s->g) {
        write(s->sessions[index]->fd, s->buf, s->buf_size);
        return;
    }
    int old_buf_size = s->buf_size;
    for (int i = 0; i < s->g->players[index].cards_count; i++) {
        int card = s->g->players[index].cards[i];
        if (card == -1)
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "%d:%s%c ", i, " ", ' ');
        else
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "%d:%s%c ", i,
                                    card_imgs[s->g->players[index].cards[i] / NUMBER_OF_SUITS],
                                    card_suit_imgs[s->g->players[index].cards[i] % NUMBER_OF_SUITS]);

    }
    s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "\nYou are player №: %d", s->sessions[index]->game_id);
    if (s->g->defender == index)
        s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "(DEFENSE)\n");
    else if (s->g->state == gs_fist_attack) {
        if (s->g->attacker == index)
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "(ATTACK)\n");
        else
            s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "(WAIT)\n");
    } else 
        s->buf_size += snprintf(s->buf + s->buf_size, BUF_SIZE, "(ATTACK)\n");

    write(s->sessions[index]->fd, s->buf, s->buf_size);
    s->buf_size = old_buf_size;
}
