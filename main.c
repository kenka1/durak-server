#include "server_init.h"
#include "server.h"

int main()
{
    struct server s;
    if (server_init(&s) == -1)
        return 1;
    server_start(&s);
}
