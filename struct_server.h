#ifndef STRUCT_SERVER_MODULE
#define STRUCT_SERVER_MODULE

#include <poll.h>

#include "constants.h"
#include "session.h"
#include "cards.h"

enum server_state{
    ss_lobby,
    ss_game_init,
    ss_game,
    ss_end
};

struct server{
    struct pollfd fds[NUMBER_OF_PLAYERS + SERVER_FDS];
    struct session *sessions[NUMBER_OF_PLAYERS];
    struct game *g;
    char  buf[BUF_SIZE];
    nfds_t nfds;
    int number_of_sessions;
    int buf_size;
    int ready;
    int redraw;
    enum server_state state;
};

#endif
