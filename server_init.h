#ifndef SERVER_INIT_MODULE
#define SERVER_INIT_MODULE

struct server;

int server_init_eventfd();
int server_init_listen_socket();
int server_bind_listen_socket(int ls);
void server_init_struct(struct server *s, int ls, int efd);
void server_close_fd(int *fd);
void server_cleanup(struct server *s);
int server_init(struct server *s);

#endif
