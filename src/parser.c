#define _POSIX_C_SOURCE 200809L
#include "parser.h"

#include <stdlib.h>
#include <string.h>

struct global parse_global(char *line, FILE *file, size_t *len)
{
    char *log_file = NULL;
    bool log = false;
    char *pid_file = NULL;

    ssize_t nread;
    char *saveptr = NULL;

    while ((nread = getline(&line, len, file)) != -1
           && strcmp(line, "[[vhosts]]\n") != 0)
    {
        char *arg_name = strtok_r(line, " =", &saveptr);
        char *arg_value = strtok_r(NULL, " =\n", &saveptr);

        if (strcmp(arg_name, "log_file") == 0)
        {
            log_file = strdup(arg_value);
        }
        else if (strcmp(arg_name, "log") == 0)
        {
            if (strcmp(arg_value, "true") == 0)
            {
                log = true;
            }
            else
            {
                log = false;
            }
        }
        else if (strcmp(arg_name, "pid_file") == 0)
        {
            pid_file = strdup(arg_value);
        }
    }

    return global_init(log_file, log, pid_file);
}

struct vhost parse_vhost(char *line, FILE *file, size_t *len)
{
    char *server_name = NULL;
    char *port = NULL;
    char *ip = NULL;
    char *root_dir = NULL;
    char *default_file = NULL;

    ssize_t nread;
    char *saveptr = NULL;

    while ((nread = getline(&line, len, file)) != -1
           && strcmp(line, "\n") != 0)
    {
        char *arg_name = strtok_r(line, " =", &saveptr);
        char *arg_value = strtok_r(NULL, " =\n", &saveptr);

        if (strcmp(arg_name, "server_name") == 0)
        {
            server_name = strdup(arg_value);
        }
        else if (strcmp(arg_name, "port") == 0)
        {
            port = strdup(arg_value);
        }
        else if (strcmp(arg_name, "ip") == 0)
        {
            ip = strdup(arg_value);
        }
        else if (strcmp(arg_name, "root_dir") == 0)
        {
            root_dir = strdup(arg_value);
        }
        else if (strcmp(arg_name, "default_file") == 0)
        {
            default_file = strdup(arg_value);
        }
    }
    
    return vhost_init(server_name, port, ip, root_dir, default_file);
}

void free_global(struct global g)
{
    free(g.pid_file);
    free(g.log_file);
}

void free_vhost(struct vhost v)
{
    free(v.server_name);
    free(v.port);
    free(v.ip);
    free(v.root_dir);
    free(v.default_file);
}

struct server *fileconfig_parser(char *file)
{
    FILE *f = fopen(file, "r");
    if (!f)
    {
        perror("cannot open file");
        return NULL;
    }

    char *line = NULL;
    size_t len = 0;

    ssize_t nread = getline(&line, &len, f);
    if (nread == -1 || strcmp(line, "[global]\n") != 0)
    {
        free(line);
        perror("invalid file");
        fclose(f);
        return NULL;
    }

    struct global global = parse_global(line, f,&len);
    if (global.pid_file == NULL)
    {
        free_global(global);
        fclose(f);
        free(line);
        return NULL;
    }

    struct vhost vhost = parse_vhost(line, f,&len);
    if (vhost.server_name == NULL || vhost.port == NULL || vhost.ip == NULL
        || vhost.root_dir == NULL)
    {
        free_global(global);
        free_vhost(vhost);
        fclose(f);
        free(line);
        return NULL;
    }

    struct server *server = server_init(global, vhost);

    free(line);
    fclose(f);

    return server;
}
