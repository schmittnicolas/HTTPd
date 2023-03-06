#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

int main(int argc, char *argv[])
{
    if (argc == 3)
    {
        if (strcmp(argv[1], "--dry-run") == 0)
        {
            struct server *server = fileconfig_parser(argv[2]);

            if (server == NULL)
                return 2;

            free_server(server);
            return 0;
        }
    }
    else if (argc == 4)
    {
        if (strcmp(argv[1], "-a") == 0)
        {
            struct server *server = fileconfig_parser(argv[3]);
            if (server == NULL)
                return 2;

            if (strcmp(argv[2], "start") == 0)
            {
                return start_server(server, true);
            }
            else if (strcmp(argv[2], "stop") == 0)
            {
                return stop_server(server);
            }
            else if (strcmp(argv[2], "restart") == 0)
            {
                return restart_server(server);
            }
            else if (strcmp(argv[2], "reload") == 0)
            {}

            free_server(server);
        }
    }
    else if (argc == 2)
    {
        struct server *server = fileconfig_parser(argv[1]);
        if (server == NULL)
            return 2;
        return start_server(server, false);
    }

    return 1;
}
