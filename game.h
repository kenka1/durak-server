#ifndef GAME_MODULE
#define GAME_MODULE

#include "constants.h"
#include <time.h>

struct session;

struct player {
    int cards[NUMBER_OF_CARDS];
    int cards_count;
};

struct game {
    struct player players[NUMBER_OF_PLAYERS];
    int cards[NUMBER_OF_CARDS];
    int cards_count;
    int table_attack[DESK_SIZE];
    int attack_count;
    int table_defensive[DESK_SIZE];
    int defensive_count;
    int attacker;
    int defender;
    int trump_suit;
    time_t round_time;
};


void game_shuffling(struct game *g);
int game_first_attacker(struct game *g);
void game_init_players(struct game *g);
struct game* game_init();

void game_next(struct game *g);
void defender_take(struct game *g, int player_id);

int* player_can_defend(struct game *g, struct session *ss);
int cards_same_suit(int card0, int card1);
int check_card_suit(int card, int suit);
void defense(struct game *g, int player_id, int *cards);
void player_do_defense(struct game *g, int player_id, int *cards);
void game_handle_defender_input(struct game *g, struct session *ss);

int player_can_attack(struct game *g, struct session *ss);
void player_threw_card(struct player *p, int card_index);
void attack(struct game *g, int player_id, int card_index);
int card_in_table(struct game *g, int card);
void game_handle_attacker_input(struct game *g, struct session *ss);

void game_handle_command(struct game *g, struct session *ss);

#endif
