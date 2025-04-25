#ifndef SERVER_MODULE
#define SERVER_MODULE

#include <poll.h>

#include "constants.h"
#include "session.h"
#include "cards.h"

struct game;

#define CLEAR_TEXT 

#define LOBBY_TEXT "\033[2J\033[H" \
                   "Press ENTER to ready\n" \
                   "Number of player in lobby: (%lu/%d) ================= Ready: (%d/%d)\r\n"

#define GAME_RULES "\033[2J\033[H" \
                   "==============================RULES==============================\n" \
                   "For attacker use:\n<card number><CR><LF>\n5<CR><LF>\n" \
                   "For Defender use:\n<card number in table attack> <card number to defense><CR><LF>\n<take> - pick up the cards\n3 2<CR><LF>\n" \
                   "@ - Hearts\n+ - Clubs\n^ - Diamonds\n# - Spades\n" \
                   "Round time: %d seconds\n"


#define GAME_INFO "==============================GAME===============================\n" \
                  "attacker: %d | defender: %d | trump suit: %c\n" \
                  "Round: %d\n" \
                  "Desk size: %d\n" \

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
    ss_game,
    ss_end
};

struct server{
    struct pollfd fds[NUMBER_OF_PLAYERS + SERVER_FDS];
    struct session *sessions[NUMBER_OF_PLAYERS];
    struct game *g;
    char  buf[BUF_SIZE];
    nfds_t nfds;
    int buf_size;
    int ready;
    int redraw;
    enum server_state state;
};

int server_init(struct server *s);

void server_accept(struct server *s);

void server_notification(struct server *s);
void server_reset_notification(struct server *s);

void server_lobby(struct server *s);

void server_draw_rules(struct server *s);
void server_draw_game_info(struct server *s);
void server_draw_attack_table(struct server *s);
void server_draw_defense_table(struct server *s);
void server_draw_hand(struct server *s);
void server_draw_game(struct server *s);
void server_game_init(struct server *s);

void server_game(struct server *s);

void server_end(struct server *s);

void server_fsm(struct server *s);

void server_redraw(struct server *s);
void server_start(struct server *s);

#endif
