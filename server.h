#ifndef SERVER_MODULE
#define SERVER_MODULE

#include <poll.h>

#include "constants.h"
#include "session.h"
#include "cards.h"

struct game;

#define GAME_RULES "==============================RULES==============================\n" \
                   "For attacker use:\n<card number><CR><LF>\n5<CR><LF>\n" \
                   "For Defender use:\n<card number in table attack> <card number to defense><CR><LF>\n3 2<CR><LF>\n" \
                   "@ - Hearts\n+ - Clubs\n^ - Diamonds\n# - Spades\n"

#define SERVER_FDS 2
#define LISTEND_FD 0
#define EVENT_FD 1

#define LOG_FILE "./log.txt"

#define WELCOME_MESSAGE "OK\n"

#define errExit(msg) do {perror(msg); exit(EXIT_FAILURE); \
                     } while (0);

#define errRet(msg) do {perror(msg); return; \
                    } while(0);

#define errRetVal(msg, val) do {perror(msg); return val; \
                    } while(0);

enum server_state{
    ss_lobby,
    ss_game_init,
    ss_game
};

struct server{
    struct pollfd fds[NUMBER_OF_PLAYERS + SERVER_FDS];
    struct session *sessions[NUMBER_OF_PLAYERS];
    struct game *g;
    char  buf[BUF_SIZE];
    nfds_t nfds;
    int buf_size;
    enum server_state state;
};

int server_init(struct server *s);

void server_accept(struct server *s);

void server_notification(struct server *s);
void server_reset_notification(struct server *s);

void server_lobby(struct server *s);

void server_draw_game(struct server *s);
void server_game_init(struct server *s);

void server_game(struct server *s);

void server_fsm(struct server *s);

void server_start(struct server *s);

#endif
