#ifndef SESSION_MODULE
#define SESSION_MODULE

#include "constants.h"

struct server;

enum session_state {
    ss_connected,
    ss_disconnect,
    ss_ready,
    ss_command,
    ss_error
};

struct session{
    int fd;
    int game_id;
    char *buf;
    int buf_size;
    enum session_state state;
};

struct session* session_init(int fd, int id);
void session_change_state(struct session *ss, enum session_state state);

void session_connected(struct session *ss);

void session_disconnected(struct session *ss);

int is_number(char str);
int is_letter(char str);
int is_valid_command(char *str);
void session_command(struct session *ss);

void session_fsm(struct session *ss);
void session_do_read(struct session *ss);

void session_do_write(struct server *s, int index);

#endif
