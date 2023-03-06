#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

int get_pid(char *path)
{
    int pid;
    char *c;
    FILE *file = fopen(path, "r");

    if (file == NULL)
        return -1;

    fscanf(file, "%ms\n", &c);

    if (c == NULL)
        return -1;
    pid = atoi(c);
    free(c);
    fclose(file);
    return pid;
}

int set_pid(int pid, char *path)
{
    FILE *file = fopen(path, "w");

    if (file == NULL)
        return -1;

    fprintf(file, "%d\n", pid);
    fclose(file);
    return 0;
}
