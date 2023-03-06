#ifndef REQUEST_H
#define REQUEST_H

#include "server.h"
struct request
{
    char *Method;
    char *Request_target;
    char *HTTP_version;
};

struct response
{
    char *HTTP_version;
    int Status_code;
    char *Reason_phrase;
};

char *get_response(struct server *server, char *ip, struct request request);
struct request parse_request(char *buf);
void get_method(int client, struct request request, struct server *server);
void free_request(struct request request);

#endif /* REQUEST_H */
