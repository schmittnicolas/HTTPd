#ifndef SERVER_H
#define SERVER_H

#define BACKLOG 10
#define BUFFER_SIZE 4096

#include <stdbool.h>
#include <stddef.h>

struct global
{
    char *pid_file;
    char *log_file; // Optional
    bool log; // Optional
};

struct vhost
{
    char *server_name;
    char *port;
    char *ip;
    char *root_dir;
    char *default_file; // Optional
};

struct server
{
    struct global global;
    struct vhost vhost;
};

/* Initialization functions */
struct global global_init(char *log_file, bool log, char *pid_file);
struct vhost vhost_init(char *name, char *port, char *ip, char *root_dir,
                        char *default_file);
struct server *server_init(struct global global, struct vhost vhost);

/* Destroy */
void free_server(struct server *server);

/* Pid */

/* Return the pid if found, -1 otherwise */
int get_pid(char *file);

/* Return 0 on success */
int set_pid(int pid, char *file);

/* Server actions */
int start_server(struct server *server, bool daemon);
int stop_server(struct server *server);
int reload_server(struct server *server);
int restart_server(struct server *server);

#endif /* SERVER_H */
