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

#if defined(SERVER_0)
void child()
{
	char buf[32];
	int n;

	n = read(0, buf, 512);
	if (n < 0)
		return;
	write(0, buf, n);
}
#elif defined(SERVER_1)
#define printf(...)
#define perror(e)

void usefulGadgets()
{
	asm ("addq %rdi, (%r15); ret");
}

void child()
{
	char buf[32];

	read(0, buf, 512);
}
#elif defined(SERVER_2)
#define LARGE_BUF 	512
#define MEDIUM_BUF	128
#define SMALL_BUF 	32

int myatoi(char *buf)
{
	int n = 0;
	while (*buf >= '0' && *buf <= '9') {
		n *= 10;
		n += *buf - '0';
		buf++;
	}
	return n;
}

void do_stuff(char *buf)
{
	char tmp[SMALL_BUF];
	int n;
       
        n = myatoi(buf);
	memcpy(tmp, buf, n);
}

void echo()
{
	char buf[MEDIUM_BUF];
	int n;

	if (read(0, buf, MEDIUM_BUF) < 0)
		return;
	write(1, buf, MEDIUM_BUF);
}

void child()
{
	char buf[LARGE_BUF];
	int n;

	for (;;) {
		n = read(0, buf, LARGE_BUF);
		if (n <= 0)
			return;
		if (buf[0] == '0') {
			echo();
		} else {
			do_stuff(buf);
		}
	}
}
#else
#error "Please define SERVER_[012]"
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
