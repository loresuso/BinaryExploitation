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

#include "malloc-2.7.2.c"

int readnum()
{
	char num[2];
	if (read(0, num, 2) < 2) {
		return -1;
	}
	if (num[0] < '0' || num[0] > '9' || num[1] < '0' || num[1] > '9') {
		return -1;
	}
	return (num[0] - '0') * 10 + (num[1] - '0');
}

void *operator new(size_t sz)
{
	return malloc(sz);
}

void operator delete(void *p)
{
	free(p);
}

class Base {
protected:
	int i;
public:
	Base(int i_): i(i_) {}
	virtual void foo(char *p, size_t s) {
		snprintf(p, s, "Base %d", i);
	}
};

class Derived1: public Base {
public:
	Derived1(int i_): Base(i_) {}
	virtual void foo(char *p, size_t s) {
		snprintf(p, s, "Derived1 %d", i);
	}
};

class Derived2: public Base {
public:
	Derived2(int i_): Base(i_) {}
	virtual void foo(char *p, size_t s) {
		snprintf(p, s, "Derived2 %d", i);
	}
};

char* values[256];
int lengths[256];
Base *objects[256];
void createkey(unsigned char key)
{
	char *v;
	int n;

	n = readnum();
	if (n < 0) {
		fprintf(stderr, "error reading keay lenght");
		return;
	}
	v = static_cast<char *>(malloc(n));
	if (v == NULL) {
		fprintf(stderr, "out of memory\n");
		return;
	}
	printf("%p\n", v);
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
	values[key] = NULL;
	lengths[key] = 0;
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

void createobj(unsigned char key)
{
	char type;

	if (read(0, &type, 1) < 1) {
		fprintf(stderr, "error reading type\n");
		return;
	}

	int n = readnum();
	if (n < 0) {
		fprintf(stderr, "error reading object data");
		return;
	}

	switch (type) {
	case 'b':
		objects[key] = new Base(n);
		break;
	case '1':
		objects[key] = new Derived1(n);
		break;
	case '2':
		objects[key] = new Derived2(n);
		break;
	default:
		fprintf(stderr, "unknown type: %c\n", type);
		break;
	}
	if (objects[key] == NULL) {
		fprintf(stderr, "out of memory\n");
	} else {
		printf("%p\n", objects[key]);
	}
}

void useobj(unsigned char key)
{
	static const size_t BUFSZ = 128;
	char buf[BUFSZ];

	Base *b = objects[key];

	if (b == NULL) {
		fprintf(stderr, "no such object\n");
		return;
	}
	memset(buf, 0, BUFSZ);
	b->foo(buf, BUFSZ);
	printf("%s\n", buf);
}

void deleteobj(unsigned char key)
{
	Base *b = objects[key];

	delete(b);
}

void child()
{
	char cmd;
	while (read(0, &cmd, 1) > 0) {
		unsigned char key;
		if (index("acdDouqs", cmd) == NULL) {
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
		case 'o':
			createobj(key);
			break;
		case 'D':
			deleteobj(key);
			break;
		case 'u':
			useobj(key);
			break;
		default:
			break;
		}
	}
}

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
