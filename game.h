#ifndef GAME_MODULE
#define GAME_MODULE

#include "constants.h"
#include <time.h>

struct session;
struct server;

struct player {
    int cards[NUMBER_OF_CARDS];
    int cards_count;
};

struct game_time {
    time_t start_time;
    time_t current_time;
    time_t round_start;
    time_t round_time;
    int number_of_round;
};

enum game_state {
    gs_fist_attack,
    gs_attack,
    gs_defender_lost,
    gs_game_end_draw,
    gs_game_end
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
    int *redraw;
    enum game_state state;
    struct game_time t;
};

void game_shuffling(struct game *g);
void fill_players_cards(struct game *g, int player_id);
int game_first_attacker(struct game *g);
void game_first_attacker_and_defender(struct game *g);
void game_init_players(struct game *g);
void game_init_time(struct game_time *t);
struct game* game_init();

int is_game_end(struct game *g);
void game_end(struct game *g);

void reset_table(int *table, int *size);
void select_next_attacker_and_defender(struct game *g);
void reset_round_time(struct game_time *t);
void update_players(struct game *g);
void game_next_round(struct game *g);
void defender_take_cards(struct game *g);
void defender_lost_round(struct game *g);

int* player_can_defend(struct game *g, struct session *ss);
int cards_same_suit(int card0, int card1);
int check_card_suit(int card, int suit);
void defense(struct game *g, int *cards);
void player_do_defense(struct game *g, int *cards);
void game_handle_defender_input(struct game *g, struct session *ss);

int player_can_attack(struct game *g, struct session *ss);
void player_threw_card(struct player *p, int card_index);
void attack(struct game *g, int player_id, int card_index);
int card_in_table(struct game *g, int card);
void game_handle_attacker_input(struct game *g, struct session *ss);

void game_handle_command(struct game *g, struct session *ss);

void game_time(struct game_time *t);
int is_defender_lose(struct game *g);
int is_round_time_over(int time);
void game_round_end(struct game *g);
void game_processing(struct game *g);

#endif
