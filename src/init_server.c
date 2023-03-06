#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include "server.h"

struct global global_init(char *log_file, bool log, char *pid_file)
{
    struct global global;
    global.log_file = log_file;
    global.log = log;
    global.pid_file = pid_file;

    return global;
}

struct vhost vhost_init(char *name, char *port, char *ip, char *root_dir,
                        char *default_file)
{
    struct vhost vhost;
    vhost.server_name = name;
    vhost.port = port;
    vhost.ip = ip;
    vhost.root_dir = root_dir;

    if (default_file == NULL)
    {
        const char *s = "index.html";
        vhost.default_file = strdup(s);
    }
    else
    {
        vhost.default_file = default_file;
    }

    return vhost;
}

struct server *server_init(struct global global, struct vhost vhost)
{
    struct server *server = malloc(sizeof(struct server));

    if (server == NULL)
        return NULL;

    server->global = global;
    server->vhost = vhost;

    return server;
}

void free_server(struct server *server)
{
    free(server->vhost.port);
    free(server->vhost.ip);
    free(server->vhost.default_file);
    free(server->vhost.server_name);
    free(server->vhost.root_dir);

    free(server->global.pid_file);
    free(server->global.log_file);

    free(server);
}
