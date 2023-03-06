#define _POSIX_C_SOURCE 200112L

#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "request.h"

#define INET_ADDRSTRLEN 16

volatile sig_atomic_t done = 0;

void term(int signum)
{
    if (signum == SIGTERM || signum == SIGSTOP || signum == SIGINT)
        done = 1;
}

int create_and_bind(char *ip, char *port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *list;
    int error = getaddrinfo(ip, port, &hints, &list);
    if (error == -1)
        return -1;

    struct addrinfo *cursor;
    int sockfd;
    for (cursor = list; cursor != NULL; cursor = cursor->ai_next)
    {
        sockfd =
            socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
        if (sockfd == -1)
            continue;
        if (bind(sockfd, cursor->ai_addr, cursor->ai_addrlen) != -1)
            break; // Bind Success
        close(sockfd);
    }

    freeaddrinfo(list);

    if (cursor == NULL)
        return -1;

    return sockfd;
}

size_t get_index_from_buf(size_t begin, size_t end, char *buffer)
{
    for (size_t i = begin; i + 1 < end; i++)
    {
        if (buffer[i] == '\r' && buffer[i + 1] == '\n')
            return i;
    }

    return end;
}

int create_daemon(struct server *server)
{
    /* Fork the program */
    int pid = fork();

    /* Error handling */
    if (pid < 0)
    {
        free_server(server);
        return 1;
    }

    /* Parent process */
    if (pid > 0)
    {
        free_server(server);
        return 0;
    }

    /* Daemon */
    int child_id = getpid();
    set_pid(child_id, server->global.pid_file);
    // chdir(server->vhost.root_dir);
    // close(STDIN_FILENO);
    // close(STDOUT_FILENO);
    // close(STDERR_FILENO);

    return 2;
}

int start_server(struct server *server, bool daemon)
{
    if (daemon)
    {
        int daemon = create_daemon(server);
        if (daemon != 2)
            return daemon;
    }

    int listening_sock = create_and_bind(server->vhost.ip, server->vhost.port);
    if (listening_sock == -1)
    {
        free_server(server);
        return 1;
    }

    if (listen(listening_sock, BACKLOG) == -1)
    {
        free_server(server);
        return 1;
    }

    char buffer[BUFFER_SIZE] = { 0 };

    // Signal handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGSTOP, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    // Log
    if (server->global.log)
    {
        if (server->global.log_file != NULL)
        {
            FILE *f1 = fopen(server->global.log_file, "w");
            if (f1)
                fclose(f1);
        }
    }

    // Main Loop
    while (!done)
    {
        struct sockaddr_in peer_addr;
        socklen_t peer_addr_size = sizeof(struct sockaddr_in);
        int client_sock = accept(listening_sock, (struct sockaddr *)&peer_addr,
                                 &peer_addr_size);
        if (client_sock == -1)
            continue;

        /* Recieve */
        ssize_t nread;
        size_t total_read = 0;
        while ((nread = recv(client_sock, buffer + total_read,
                             BUFFER_SIZE - total_read, 0))
               > 0)
        {
            size_t i =
                get_index_from_buf(total_read, total_read + nread, buffer);

            if (i != total_read + nread)
            {
                total_read = i;
                buffer[i] = '\0';
                break;
            }
            total_read += nread;
        }

        if (nread == -1)
        {
            close(client_sock);
            continue;
        }

        struct request request = parse_request(buffer);
        char *r;
        char ip[INET_ADDRSTRLEN];
        if ((inet_ntop(AF_INET, &peer_addr.sin_addr, ip, sizeof(ip))) != NULL)
        {
            r = get_response(server, ip, request);
        }

        if (r == NULL)
            continue;

        /* Send */
        ssize_t nsent;
        size_t total_sent = 0;
        while ((nsent = send(client_sock, r + total_sent,
                             strlen(r) - total_sent, MSG_NOSIGNAL))
               > 0)
        {
            total_sent += nsent;
        }

        get_method(client_sock, request, server);
        free_request(request);
        close(client_sock);
        free(r);
    }

    remove(server->global.pid_file);
    free_server(server);
    return 0;
}

int stop_server(struct server *server)
{
    char *path = server->global.pid_file;
    int pid = get_pid(path);
    if (pid == -1)
    {
        printf("The server is already stopped\n");
        free_server(server);
        return 0;
    }

    printf("trying to stop the server with the id %d found in %s\n", pid, path);

    free_server(server);
    if (kill(pid, SIGTERM) == -1)
    {
        printf("Stop failed\n");
        return 1;
    }

    printf("Server stopped\n");
    return 0;
}

int restart_server(struct server *server)
{
    char *path = server->global.pid_file;
    int pid = get_pid(path);
    if (pid == -1)
    {
        printf("The server is already stopped\n");
        free_server(server);
        return 0;
    }

    printf("trying to stop the server with the id %d found in %s\n", pid, path);

    if (kill(pid, SIGTERM) == -1)
    {
        free_server(server);
        printf("Stop failed\n");
        return 1;
    }
    return start_server(server, true);
}
