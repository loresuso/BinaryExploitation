#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#ifndef PORT
#define PORT 10000
#endif

extern char *gets(char *buf);
#if !defined(SERVER_0b)
#include "malloc-2.7.2.c"
#endif

#if defined(SERVER_0) || defined(SERVER_0b)
void vuln(size_t s)
{
	char * a = malloc(s);
	char * b = malloc(s);

	printf("a %p b %p\n", a, b);

	read(0, a, s + 2 * sizeof(size_t));

	free(b);
	puts("OK");
	free(a);
}
void child()
{
	vuln(256);
}
#elif defined(SERVER_1)
#define BUFSZ 32
char* values[256];
void assignkey(unsigned char key)
{
	int n;
	char buf[BUFSZ];
	char *v;

	if (values[key] != NULL)
		free(values[key]);
	n = read(0, buf, BUFSZ - 1);
	if (n <= 0) {
		fprintf(stderr, "Error/EOF while reading value\n");
		return;
	}
	buf[n] = '\0';
	v = malloc(n + 1);
	if (v == NULL) {
		fprintf(stderr, "out of memory\n");
		return;
	}
	memcpy(v, buf, n + 1);
	values[key] = v;
}

void deletekey(unsigned char key)
{
	char *v = values[key];

	if (v == NULL) {
		fprintf(stderr, "No such key: %c\n", key);
		return;
	}
	free(v);
}

void child()
{
	char cmd;
	while (read(0, &cmd, 1) > 0) {
		unsigned char key;
		if (cmd != 'a' && cmd != 'd' && cmd != 's' && cmd != 'q') {
			if (cmd != '\n')
				fprintf(stderr, "Unknown command: '%c'\n", cmd);
			continue;
		}
		if (cmd == 'q')
			break;
		if (read(0, &key, 1) <= 0) {
			fprintf(stderr, "Error/EOF while reading key\n");
			return;
		}
		switch (cmd) {
		case 'a':
			assignkey(key);
			break;
		case 'd':
			deletekey(key);
			break;
		case 's':
			fprintf(stderr, "sorry, not implemented yet\n");
			break;
		default:
			break;
		}
	}
	system("echo goodbye");
}
#elif defined(SERVER_2)
#define BUFSZ 32
char* values[256];
int lengths[256];
void createkey(unsigned char key)
{
	char len[2];
	char *v;
	int n;

	if (read(0, len, 2) < 2) {
		fprintf(stderr, "error reading key length\n");
		return;
	}
	if (len[0] < '0' || len[0] > '9' || len[1] < '0' || len[1] > '9') {
		fprintf(stderr, "invalid length: %c%c\n", len[0], len[1]);
		return;
	}
	n = (len[0] - '0') * 10 + (len[1] - '0');
	v = malloc(n);
	if (v == NULL) {
		fprintf(stderr, "out of memory\n");
		return;
	}
	lengths[key] = n;
	if (values[key] != NULL)
		free(values[key]);
	values[key] = v;
}
void assignkey(unsigned char key)
{
	int n;
	char *v;

	if (values[key] == NULL) {
		fprintf(stderr, "no such key\n");
		return;
	}

	n = read(0, values[key], lengths[key]);
	if (n <= 0) {
		fprintf(stderr, "Error/EOF while reading value\n");
		return;
	}
}

void deletekey(unsigned char key)
{
	char *v = values[key];

	if (v == NULL) {
		fprintf(stderr, "No such key: %c\n", key);
		return;
	}
	free(v);
}

void searchkey(unsigned char key)
{
	int n;
	char *v;

	if (values[key] == NULL) {
		fprintf(stderr, "no such key\n");
		return;
	}
	n = lengths[key];
	v = values[key];
	while (n > 0) {
		int m = write(1, v, n);
		if (m < 0) {
			fprintf(stderr, "error sending key value\n");
			return;
		}
		n -= m;
		v += m;
	}
}

void child()
{
	char cmd;
	while (read(0, &cmd, 1) > 0) {
		unsigned char key;
		if (cmd != 'c' && cmd != 'a' && cmd != 'd' && cmd != 's' && cmd != 'q') {
			if (cmd != '\n')
				fprintf(stderr, "Unknown command: '%c'\n", cmd);
			continue;
		}
		if (cmd == 'q')
			break;
		if (read(0, &key, 1) <= 0) {
			fprintf(stderr, "Error/EOF while reading key\n");
			return;
		}
		switch (cmd) {
		case 'c':
			createkey(key);
			break;
		case 'a':
			assignkey(key);
			break;
		case 'd':
			deletekey(key);
			break;
		case 's':
			searchkey(key);
			break;
		default:
			break;
		}
	}
}
#endif

static void cleanup()
{
	char buf[4096];

	memset(buf, 0, 4096);
}

int main()
{
	int lstn;
	int enable;
	struct sockaddr_in lstn_addr;

	lstn = socket(AF_INET, SOCK_STREAM, 0);
	if (lstn < 0) {
		perror("socket");
		return 1;
	}
	enable = 1;
	if (setsockopt(lstn, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		perror("setsockopt");
		return 1;
	}
	bzero(&lstn_addr, sizeof(lstn_addr));

	lstn_addr.sin_family = AF_INET;
	lstn_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	lstn_addr.sin_port = htons(PORT);

	if (bind(lstn, (struct sockaddr *)&lstn_addr, sizeof(lstn_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (listen(lstn, 10) < 0) {
		perror("listen");
		return 1;
	}
	printf("Listening on port %d\n", PORT);

	signal(SIGCHLD, SIG_IGN);

	for (;;) {
		int con = accept(lstn, NULL, NULL);
		if (con < 0) {
			perror("accept");
			return 1;
		}

		switch (fork()) {
		case -1:
			perror("fork");
			return 1;
		case 0:
			printf("New connection, child %d\n", getpid());

			close(0);
			dup(con);
			close(1);
			dup(con);
			close(2);
			dup(con);
			cleanup();
			child();
			exit(0);
			break;
		default:
			close(con);
			break;
		}
	}
	return 0;
}
