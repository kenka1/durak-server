#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "game.h"
#include "cards.h"
#include "constants.h"
#include "session.h"
#include "server.h"

void game_shuffling(struct game *g)
{
    printf("[game_shuffling]\n");
    int flags[NUMBER_OF_CARDS] = {0};
    int index;

    srand(time(NULL));

    for (int i = 0; i < NUMBER_OF_CARDS; i++) {
        index = rand() / ((RAND_MAX + 1u) / NUMBER_OF_CARDS);
        if (flags[index] != 0)
            while (flags[++index % NUMBER_OF_CARDS]);
        g->cards[i] = index % NUMBER_OF_CARDS;
        flags[index % NUMBER_OF_CARDS] = 1;
    }

    g->cards_count = NUMBER_OF_CARDS;
    for (int i = 0; i < DESK_SIZE; i++) {
        g->table_attack[i] = -1;
        g->table_defensive[i] = -1;
    }
    g->attack_count = 0;
    g->defensive_count = 0;
    g->trump_suit = rand() / ((RAND_MAX + 1u) / NUMBER_OF_SUITS);
}

int game_first_attacker(struct game *g)
{
    printf("[game_first_attacker]\n");
    int attacker = -1;
    int min = NUMBER_OF_CARDS + 1;
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
        for (int k = 0; k < NUMBER_OF_INIT_CARDS; k++) {
            int card = g->players[i].cards[k];
            if (card % NUMBER_OF_SUITS == g->trump_suit) {
                if (card < min) {
                    attacker = i;
                    min = card;
                }
            }
        }
    }
    
    printf("min : %d\n", min);
    return attacker;
}

void game_init_players(struct game *g)
{
    printf("[game_init_players]\n");
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
        for (int k = 0; k < NUMBER_OF_INIT_CARDS; k++)
            g->players[i].cards[k] = g->cards[g->cards_count-- - 1];
        g->players[i].cards_count = NUMBER_OF_INIT_CARDS;
    }
    g->attacker = game_first_attacker(g);
    g->defender = (g->attacker + 1) % NUMBER_OF_PLAYERS;
}

struct game* game_init()
{
    printf("[game_init]\n");
    struct game *g = malloc(sizeof(struct game));
    game_shuffling(g);
    game_init_players(g);
    return g;
}

void game_next(struct game *g)
{
    g->attack_count = 0;
    for (int i = 0; i < DESK_SIZE; i++)
        g->table_attack[i] = -1;
    g->defensive_count = 0;
    for (int i = 0; i < DESK_SIZE; i++)
        g->table_defensive[i] = -1;
    while ((g->attacker = (g->attacker + 1) % NUMBER_OF_PLAYERS) == g->defender);
    g->defender = (g->defender + 2) % NUMBER_OF_PLAYERS;
}

void defender_take(struct game *g, int player_id)
{
    printf("[defender_take]\n");
    int *count, *cards;
    if (g->attack_count == 0)
        return;

    count = &g->players[player_id].cards_count;
    cards = g->players[player_id].cards;
    for (int i = 0; i < g->defensive_count; i++) {
        cards[(*count)++] = g->table_defensive[i];
    }
    for (int i = 0; i < g->attack_count; i++) {
        cards[(*count)++] = g->table_attack[i];
    }
    game_next(g);
}

int* player_can_defend(struct game *g, struct session *ss)
{
    printf("[player_can_defend]\n");
    int res, attack_index, def_index;
    int *card_indices;

    /* attack table is empty */
    if (g->attack_count == 0)
        return NULL;

    /* defender has zero cards */
    if (g->players[ss->game_id].cards_count == 0)
        return NULL;

    /* wrong input */
    if (strlen(ss->buf) != 3)
        return NULL;

    res = sscanf(ss->buf, "%d%d", &attack_index, &def_index);
    if (res != 2)
        return NULL;

    /* wrong attack index */
    if (attack_index >= g->attack_count)
        return NULL;

    /* wrong defense index */
    if (def_index >= g->players[ss->game_id].cards_count)
        return NULL;

    /* index already in use */
    if (g->table_defensive[attack_index] != -1)
        return NULL;

    card_indices = malloc(sizeof(int) * res);
    card_indices[0] = attack_index;
    card_indices[1] = def_index;

    return card_indices;
}

int cards_same_suit(int card0, int card1)
{
    if (card0 % NUMBER_OF_SUITS == card1 % NUMBER_OF_SUITS)
        return 1;
    return 0;
}

int check_card_suit(int card, int suit)
{
    printf("[check_card_suit]\n");
    if (card % NUMBER_OF_SUITS == suit)
        return 1;
    return 0;
}

void defense(struct game *g, int player_id, int *cards)
{
    g->table_defensive[cards[0]] = g->players[player_id].cards[cards[1]];
    player_threw_card(&g->players[player_id], cards[1]);
    g->defensive_count++;
}

void player_do_defense(struct game *g, int player_id, int *cards)
{
    printf("[player_do_defense]\n");
    int attack_card, def_card;

    attack_card = g->table_attack[cards[0]];
    def_card = g->players[player_id].cards[cards[1]];
    printf("attack_card: %d\ndef_card: %d\n", attack_card, def_card);
    printf("attack_card_suit: %d\ndef_car_suit: %d\ntrump_suit: %d\n", attack_card % NUMBER_OF_SUITS, def_card % NUMBER_OF_SUITS, g->trump_suit);

    if (cards_same_suit(attack_card, def_card)) {
        if (def_card > attack_card)
            defense(g, player_id, cards);
    }

    if (check_card_suit(def_card, g->trump_suit))
        defense(g, player_id, cards);
}

void game_handle_defender_input(struct game *g, struct session *ss)
{
    printf("[game_handle_defender_input]\n");
    int *card_indices;

    if (strcmp(ss->buf, "take") == 0) {
        defender_take(g, ss->game_id);
        return;
    }

    card_indices = player_can_defend(g, ss);
    if(card_indices == NULL)
        return; 

    player_do_defense(g, ss->game_id, card_indices);

    free(card_indices);

}

int player_can_attack(struct game *g, struct session *ss)
{
    printf("[player_can_attack]\n");
    int res, card_index;

    /* player doesn't have cards */
    if (g->players[ss->game_id].cards_count == 0)
        return -1;

    /* attack table is full */
    if (g->attack_count == DESK_SIZE)
        return -1;

    /* attack table doesn't have card and player is not first attacker */
    if (g->attack_count == 0 && ss->game_id != g->attacker)
        return -1;

    /* wrong input */
    if (strlen(ss->buf) != 1)
        return -1;

    res = sscanf(ss->buf, "%d", &card_index);
    if (res != 1 || card_index >= g->players[ss->game_id].cards_count)
        return -1;

    return card_index;
}

void player_threw_card(struct player *p, int card_index)
{
    printf("[player_threw_card]\n");
    for (int i = card_index; i < p->cards_count - 1; i++)
        p->cards[i] = p->cards[i + 1];
    p->cards_count--;
}

void attack(struct game *g, int player_id, int card_index)
{
    printf("[player_attack]\n");
    g->table_attack[g->attack_count++] = g->players[player_id].cards[card_index];
    player_threw_card(&g->players[player_id], card_index);
}

int card_in_table(struct game *g, int card)
{
    printf("[card_in_table]\n");
    int lvl = card / NUMBER_OF_SUITS;

    printf("lvl: %d\n", lvl);
    for (int i = 0; i < g->attack_count; i++) {
        if (g->table_attack[i] / 4 == lvl)
            return 1;
    }
    for (int i = 0; i < DESK_SIZE; i++) {
        if (g->table_defensive[i] != -1 && g->table_defensive[i] / 4 == lvl)
            return 1;
    }
    return 0;
}

void game_handle_attacker_input(struct game *g, struct session *ss)
{
    printf("[game_handle_attacker_input]\n");
    int card_index, card;

    card_index = player_can_attack(g, ss);
    if (card_index == -1)
        return;
    card = g->players[ss->game_id].cards[card_index];

    if (g->attack_count == 0) {
        printf("first attack\n");
        attack(g, ss->game_id, card_index);
    } else if (card_in_table(g, card)) {
        printf("card find in table\n");
        attack(g, ss->game_id, card_index);
    }
}

void game_handle_command(struct game *g, struct session *ss)
{
    printf("[game_handle_command]\n");
    if (ss->game_id == g->defender)
        game_handle_defender_input(g, ss);
    else
        game_handle_attacker_input(g, ss);
}
