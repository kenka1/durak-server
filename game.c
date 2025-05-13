#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "game.h"
#include "constants.h"
#include "session.h"
#include "server.h"

void game_shuffling(struct game *g)
{
    printf("{game.c}:[game_shuffling]\n");
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
    g->state = gs_fist_attack;
}

void fill_players_cards(struct game *g, int player_id)
{
    printf("{game.c}:[fill_players_cards]\n");
    int *cards, *desk, *cards_count, *desk_count;

    cards = g->players[player_id].cards;
    desk = g->cards;
    cards_count = &g->players[player_id].cards_count;
    desk_count = &g->cards_count;

    for (; *cards_count < NUMBER_OF_INIT_CARDS && *desk_count > 0; (*cards_count)++, (*desk_count)--)
        cards[*cards_count] = desk[*desk_count - 1];
}

int game_first_attacker(struct game *g)
{
    printf("{game.c}:[game_first_attacker]\n");
    int attacker = -1;
    int min = NUMBER_OF_CARDS + 1;
    for (int i = 0; i < *g->number_of_players; i++) {
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

    if (attacker == -1)
        attacker = rand() / ((RAND_MAX + 1u) / *g->number_of_players);
    
    printf("min : %d\n", min);
    return attacker;
}

void game_first_attacker_and_defender(struct game *g)
{
    printf("{game.c}:[game_first_attacker_and_defender]\n");
    g->attacker = game_first_attacker(g);
    g->defender = (g->attacker + 1) % *g->number_of_players;
    printf("attacker: %d\ndefender: %d\n", g->attacker, g->defender);
}

void game_init_players(struct game *g)
{
    printf("{game.c}:[game_init_players]\n");
    for (int i = 0; i < *g->number_of_players; i++) {
        g->players[i].cards_count = 0;
        fill_players_cards(g, i);
    }
    game_first_attacker_and_defender(g);
}

void game_init_time(struct game_time *t)
{
    printf("{game.c}:[game_init_time]\n");
    t->start_time = time(NULL);
    t->current_time = t->start_time;
    t->round_start = t->start_time;
    t->round_time = 0;
    t->number_of_round = 0;
}

void game_init(struct server *s)
{
    printf("{game.c}:[game_init]\n");
    s->g = malloc(sizeof(struct game));
    s->g->redraw = &s->redraw;
    s->g->number_of_players = &s->number_of_sessions;
    game_shuffling(s->g);
    game_init_players(s->g);
    game_init_time(&s->g->t);
}

int is_game_end(struct game *g)
{
    printf("{game.c}:[is_game_end]\n");
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
        if (g->defender == i)
            continue;
        if (g->players[i].cards_count != 0)
            return 0;
    }

    if (g->players[g->defender].cards_count != 0)
        g->state = gs_game_end;
    else
        g->state = gs_game_end_draw;

    return 1;
}

void reset_table(int *table, int *size)
{
    printf("{game.c}:[reset_table]\n");
    for (int i = 0; i < DESK_SIZE; i++)
        table[i] = -1;
    *size = 0;
}

void select_defender(struct game *g, int offset)
{
    for (int i = g->defender + 1; i < NUMBER_OF_PLAYERS + g->defender; i++) {
        if (g->players[i % NUMBER_OF_PLAYERS].cards_count == 0)
            continue;
        offset--;
        if (offset == 0) {
            g->defender = i % NUMBER_OF_PLAYERS;
            break;
        }
    }
}

void select_attacker(struct game *g, int offset)
{
    for (int i = g->attacker+ 1; i < NUMBER_OF_PLAYERS + g->attacker; i++) {
        if (g->players[i % NUMBER_OF_PLAYERS].cards_count == 0)
            continue;
        offset--;
        if (offset == 0) {
            g->attacker = i % NUMBER_OF_PLAYERS;
            break;
        }
    }
}

void select_next_attacker_and_defender(struct game *g)
{
    printf("{game.c}:[select_next_attacker_and_defender]\n");
    int offset;
    if (g->state == gs_defender_lost)
        offset = 2;
    else
        offset = 1;
    select_defender(g, offset);
    select_attacker(g, offset);
    printf("attacker: %d\ndefender: %d\n", g->attacker, g->defender);
}

void reset_round_time(struct game_time *t)
{
    printf("{game.c}:[reset_round_time]\n");
    t->round_start = time(NULL);
    t->round_time = 0;
    t->number_of_round++;
}

void update_players(struct game *g)
{
    printf("{game.c}:[update_players]\n");
    if (g->cards_count == 0)
        return;

    for (int i = g->attacker; i < NUMBER_OF_PLAYERS + g->attacker; i++) {
        if (i % NUMBER_OF_PLAYERS == g->defender)
            continue;
        fill_players_cards(g, i % NUMBER_OF_PLAYERS);
    }
    fill_players_cards(g, g->defender);
}

void game_next_round(struct game *g)
{
    printf("{game.c}:[game_next_round]\n");
    reset_table(g->table_attack, &g->attack_count);
    reset_table(g->table_defensive, &g->defensive_count);
    update_players(g);
    select_next_attacker_and_defender(g);
    reset_round_time(&g->t);
    *g->redraw = 1;
    g->state = gs_fist_attack;
}

void defender_take_cards(struct game *g)
{
    printf("{game.c}:[defender_take_cards]\n");
    for (int i = 0; i < DESK_SIZE; i++) {
        if (g->table_defensive[i] == -1)
            continue;
        g->players[g->defender].cards[g->players[g->defender].cards_count++] = g->table_defensive[i];
    }
    for (int i = 0; i < DESK_SIZE; i++) {
        if (g->table_attack[i] == -1)
            continue;
        g->players[g->defender].cards[g->players[g->defender].cards_count++] = g->table_attack[i];
    }
}

void defender_lost_round(struct game *g)
{
    printf("{game.c}:[defender_lost_round]\n");
    if (g->attack_count == 0)
        return;

    if (is_game_end(g)) {
        return;
    }

    defender_take_cards(g);
    g->state = gs_defender_lost;
    game_next_round(g);
}

int* player_can_defend(struct game *g, struct session *ss)
{
    printf("{game.c}:[player_can_defend]\n");
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
    printf("{game.c}:[cards_same_suit]\n");
    if (card0 % NUMBER_OF_SUITS == card1 % NUMBER_OF_SUITS)
        return 1;
    return 0;
}

int check_card_suit(int card, int suit)
{
    printf("{game.c}:[check_card_suit]\n");
    if (card % NUMBER_OF_SUITS == suit)
        return 1;
    return 0;
}

void defense(struct game *g, int *cards)
{
    printf("{game.c}:[defense]\n");
    printf("player: %d\ndefense: %s%c\n", g->defender,
                                    card_imgs[g->players[g->defender].cards[cards[1]] / NUMBER_OF_SUITS],
                                    card_suit_imgs[g->players[g->defender].cards[cards[1]] % NUMBER_OF_SUITS]);
    g->table_defensive[cards[0]] = g->players[g->defender].cards[cards[1]];
    player_threw_card(&g->players[g->defender], cards[1]);
    g->defensive_count++;
    *g->redraw= 1;
}

void player_do_defense(struct game *g, int *cards)
{
    printf("{game.c}:[player_do_defense]\n");
    int attack_card, def_card;

    attack_card = g->table_attack[cards[0]];
    def_card = g->players[g->defender].cards[cards[1]];
    printf("attack_card: %d\ndef_card: %d\n", attack_card, def_card);
    printf("attack_card_suit: %d\ndef_car_suit: %d\ntrump_suit: %d\n", attack_card % NUMBER_OF_SUITS, def_card % NUMBER_OF_SUITS, g->trump_suit);

    if (cards_same_suit(attack_card, def_card)) {
        if (def_card > attack_card)
            defense(g, cards);
    } else if (check_card_suit(def_card, g->trump_suit))
        defense(g, cards);
}

void game_handle_defender_input(struct game *g, struct session *ss)
{
    printf("{game.c}:[game_handle_defender_input]\n");
    int *card_indices;

    if (strcmp(ss->buf, "take") == 0) {
        defender_lost_round(g);
        return;
    }

    card_indices = player_can_defend(g, ss);
    if(card_indices == NULL)
        return; 

    player_do_defense(g, card_indices);

    free(card_indices);

}

int player_can_attack(struct game *g, struct session *ss)
{
    printf("{game.c}:[player_can_attack]\n");
    int res, card_index;

    /* player doesn't have cards */
    if (g->players[ss->game_id].cards_count == 0)
        return -1;

    /* attack table is full */
    if (g->attack_count == DESK_SIZE)
        return -1;

    /* table == defender cards count */
    if (g->players[g->defender].cards_count + g->defensive_count == g->attack_count)
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
    printf("{game.c}:[player_threw_card]\n");
    for (int i = card_index; i < p->cards_count - 1; i++)
        p->cards[i] = p->cards[i + 1];
    p->cards_count--;
}

void attack(struct game *g, int player_id, int card_index)
{
    printf("{game.c}:[player_attack]\n");
    printf("player: %d\nattack: %s%c\n", player_id,
                                    card_imgs[g->players[player_id].cards[card_index] / NUMBER_OF_SUITS],
                                    card_suit_imgs[g->players[player_id].cards[card_index] % NUMBER_OF_SUITS]);
    g->table_attack[g->attack_count++] = g->players[player_id].cards[card_index];
    player_threw_card(&g->players[player_id], card_index);
    g->state = gs_attack;
    *g->redraw = 1;
}

int card_in_table(struct game *g, int card)
{
    printf("{game.c}:[card_in_table]\n");
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
    printf("{game.c}:[game_handle_attacker_input]\n");
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
    printf("{game_handle_command}:[game_handle_command]\n");
    if (ss->game_id == g->defender)
        game_handle_defender_input(g, ss);
    else
        game_handle_attacker_input(g, ss);
}

void game_time(struct game_time *t)
{
    printf("{game.c}:[game_time]\n");
    t->current_time = time(NULL);
    t->round_time = t->current_time - t->round_start;
    printf("round_time: %lu\n", t->round_time);
}

int is_defender_lose(struct game *g)
{
    printf("{game.c}:[is_defender_lose]\n");
    printf("attack_count: %d\ndefensive_count: %d\n", g->attack_count, g->defensive_count);
    if (g->attack_count == g->defensive_count)
        return 0;
    return 1;
}

int is_round_time_over(int time)
{
    printf("{game.c}:[is_round_time_over]\n");
    if (time < ROUND_TIME)
        return 0;
    return 1;
}

void game_round_end(struct game *g)
{
    printf("{game.c}:[game_round_end]\n");
    if (is_defender_lose(g))
        defender_lost_round(g); 
    else
        game_next_round(g);
}

void game_processing(struct game *g)
{
    printf("{game.c}:[game_processing]\n");
    game_time(&g->t);
  
    if (is_game_end(g))
        return;

    if (is_round_time_over(g->t.round_time))
        game_round_end(g);
}
