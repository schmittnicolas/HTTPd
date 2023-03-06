#define _GNU_SOURCE
#include "request.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "parser.h"

#define SIZE 1024

char *date(void)
{
    char outstr[200];
    time_t t = time(NULL);
    strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T GMT", localtime(&t));
    return strdup(outstr);
}

struct request parse_request(char *buf)
{
    char *line = strdup(buf);
    struct request request;
    char *saveptr = NULL;

    request.Method = strdup(strtok_r(line, " ", &saveptr));
    request.Request_target = strdup(strtok_r(NULL, " ", &saveptr));
    request.HTTP_version = strdup(strtok_r(NULL, " ", &saveptr));
    free(line);
    return request;
}

struct response init_response(struct server *server, struct request request)
{
    struct response response;
    response.HTTP_version = request.HTTP_version;
    if (strcmp(request.Method, "GET") != 0
        && strcmp(request.Method, "HEAD") != 0)
    {
        response.Status_code = 405;
        response.Reason_phrase = "Method Not Allowed";
    }
    else if (strcmp(request.HTTP_version, "HTTP/1.1") != 0)
    {
        response.Status_code = 505;
        response.Reason_phrase = "HTTP Version Not Supported";
    }
    else
    {
        struct stat sb;
        const char *file_name = request.Request_target;
        const char *root = server->vhost.root_dir;
        char *path = malloc(strlen(file_name) + strlen(root) + 1);
        strcpy(path, root);
        strcat(path, file_name);

        if (path != NULL)
        {
            FILE *f;
            if (stat(path, &sb) == -1)
            {
                response.Status_code = 404;
                response.Reason_phrase = "Not Found";
            }
            else if (!(f = fopen(path, "r")))
            {
                response.Status_code = 403;
                response.Reason_phrase = "Forbidden";
                fclose(f);
            }
            else
            {
                response.Status_code = 200;
                response.Reason_phrase = "OK";
            }
            free(path);
        }
    }
    return response;
}

char *response(struct server *server, char *ip, struct request request,
               struct response response)
{
    char buf[4096];
    char *request_date = date();
    FILE *f = stdout;
    if (server->global.log)
    {
        if (server->global.log_file != NULL)
            f = fopen(server->global.log_file, "a");
        if (f != NULL)
        {
            if (response.Status_code != 200)
            {
                fprintf(f, "%s [%s] received Bad Request from %s\n",
                        request_date, server->vhost.server_name, ip);
            }
            else
            {
                fprintf(f, "%s [%s] received %s on '%s' from %s\n",
                        request_date, server->vhost.server_name, request.Method,
                        request.Request_target, ip);
            }
        }
    }

    struct stat sb;
    const char *file_name = request.Request_target;
    const char *root = server->vhost.root_dir;
    char *path = malloc(strlen(file_name) + strlen(root) + 1);
    strcpy(path, root);
    strcat(path, file_name);

    off_t nb_char = 0;
    if (stat(path, &sb) != -1)
        nb_char = sb.st_size;
    if (!S_ISREG(sb.st_mode))
        nb_char = 0;

    sprintf(buf,
            "%s %d %s\r\nDate: %s\r\nServer: %s\r\nContent-Length: "
            "%lu\r\nConnection: close\r\n\r\n",
            response.HTTP_version, response.Status_code, response.Reason_phrase,
            request_date, server->vhost.server_name, nb_char);

    if (server->global.log && response.Status_code == 200)
    {
        fprintf(f, "%s [%s] responding with %d %s to %s with %s on '%s'\n",
                request_date, server->vhost.server_name, response.Status_code,
                response.Reason_phrase, ip, request.Method,
                request.Request_target);
    }

    if (f != stdout)
        fclose(f);

    free(request_date);
    free(path);
    return strdup(buf);
}

void get_method(int client, struct request request, struct server *server)
{
    if (strcmp(request.Method, "GET") == 0)
    {
        const char *file_name = request.Request_target;
        const char *root = server->vhost.root_dir;
        char *path = malloc(strlen(file_name) + strlen(root) + 1);
        strcpy(path, root);
        strcat(path, file_name);

        int in = open(path, O_RDONLY);
        sendfile(client, in, NULL, 104857600);
        close(in);
        free(path);
    }
}

char *get_response(struct server *server, char *ip, struct request request)
{
    struct response res = init_response(server, request);
    char *result = response(server, ip, request, res);
    return result;
}

void free_request(struct request request)
{
    free(request.Request_target);
    free(request.Method);
    free(request.HTTP_version);
}
