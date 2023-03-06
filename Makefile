CC = gcc
CFLAGS = -std=c99 -Werror -Wall -Wextra -Wvla

SRC = src/main.c src/init_server.c src/parser.c src/pid.c src/request.c src/server.c

all: httpd

httpd:
	$(CC) $(CFLAGS) -o httpd $(SRC)

httpd_mem:
	$(CC) $(CFALGS) -o httpd $(SRC) -fsanitize=address -g

check:
	 python tests/testsuite.py

.PHONY: all httpd httpd_mem check clean

clean:
	$(RM) httpd
