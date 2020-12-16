#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "server.h"

#ifndef SRVNAME
#define SRVNAME "remotedb"
#endif

/* shared with child */
int verbosity = 0;
int daemonized = 0;
const char *dbfile = "/var/log/" SRVNAME ".log";
const char *licencefile = "/opt/flags/" SRVNAME "/flag.txt";

static int port = 10000;
static volatile int running = 1;

static void removepid()
{
	unlink("/var/run/" SRVNAME ".pid");
}

static void sigterm_h(int signo)
{
	running = 0;
}

static void usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [-p port][-d][-l licence file][-f db file]\n", progname);
	fprintf(stderr, " -p port      port to listen for connections [%d]\n", port);
	fprintf(stderr, " -d           daemonize (run in background)\n");
	fprintf(stderr, " -l file      path of the licence file [%s]\n", licencefile);
	fprintf(stderr, " -f file      path of the data base [%s]\n", dbfile);
	fprintf(stderr, " -v           increase verbosity\n");
}

extern void child(void);
int main(int argc, char *argv[])
{
	int lstn;
	int enable;
	int opt;
	struct sockaddr_in lstn_addr;
	int isdaemon = 0;

	while ( (opt = getopt(argc, argv, "p:dvl:f:")) != -1 ) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'd':
			isdaemon = 1;
			break;
		case 'v':
			verbosity++;
			break;
		case 'l':
			licencefile = optarg;
			break;
		case 'f':
			dbfile = optarg;
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}	

	if (isdaemon) {
		int i, fd, n, mypipe[2];
		struct rlimit rlim;
		pid_t mypid;
		char dummy;
		sigset_t set, old;
		char buf[16], *ptr;
		struct sigaction sig;

		if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
			err_out("getrlimit");
		}

		for (i = 3; i < rlim.rlim_cur; i++)
			close(i);

		for (i = 0; i < _NSIG; i++)
			signal(i, SIG_DFL);

		sigemptyset(&set);
		if (sigprocmask(SIG_SETMASK, &set, &old) < 0)
			err_out("sigprocmask");

		clearenv();
		if (putenv("PATH=/usr/local/bin:/bin:/usr/bin") < 0)
			err_out("setenv");

		if (pipe(mypipe) < 0) {
			err_out("pipe");
		}

		switch (fork()) {
		case 0:
			close(mypipe[0]);
			break;
		case -1:
			err_out("fork");
		default:
			close(mypipe[1]);
			n = read(mypipe[0], &dummy, 1);
			switch (n) {
			case 0:
				msg("failed to create daemon process\n");
				return EXIT_FAILURE;
			case -1:
				err_out("read(pipe)");
			default:
				return EXIT_SUCCESS;
			}
			break;
		}

		/* 1st child */
		if (setsid() < 0) {
			err_out("setsid");
		}

		switch (fork()) {
		case 0:
			break;
		case -1:
			err_out("fork");
		default:
			return EXIT_SUCCESS;
		}

		/* 2nd child */
		fd = open("/dev/null", O_RDWR);
		if (fd < 0)
			err_out("/dev/null");

		if (dup2(fd, STDIN_FILENO) < 0)
			err_out("dup2(STDIN)");
		if (dup2(fd, STDOUT_FILENO) < 0)
			err_out("dup2(STDOUT)");
		if (dup2(fd, STDERR_FILENO) < 0)
			err_out("dup2(STDERR)");

		if (fd != STDIN_FILENO &&
		    fd != STDOUT_FILENO &&
		    fd != STDERR_FILENO)
			close(fd);

		daemonized = 1;
		
		umask(0);

		if (chdir("/") < 0) {
			err_out("chdir(/)");
		}

		mypid = getpid();
		
		fd = open("/var/run/" SRVNAME ".pid", O_CREAT | O_EXCL | O_WRONLY, 0660);
		if (fd < 0) {
			err_out("/var/run/" SRVNAME ".pid");
		}
		snprintf(buf, 16, "%d\n", getpid());
		n = strlen(buf);
		ptr = buf;
		while (n > 0) {
			int m = write(fd, ptr, n);
			if (m < 0) {
				removepid();
				err_out("write");
			}
			n -= m;
			ptr += m;
		}

		atexit(removepid);
		memset(&sig, 0, sizeof(sig));
		sig.sa_handler = sigterm_h;
		sigaction(SIGTERM, &sig, NULL);

		if (write(mypipe[1], &dummy, 1) < 0)
			err_out("write(pipe)");

	}

	lstn = socket(AF_INET, SOCK_STREAM, 0);
	if (lstn < 0) {
		err_out("socket");
	}
	enable = 1;
	if (setsockopt(lstn, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		err_out("setsockopt");
	}
	bzero(&lstn_addr, sizeof(lstn_addr));

	lstn_addr.sin_family = AF_INET;
	lstn_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	lstn_addr.sin_port = htons(port);

	if (bind(lstn, (struct sockaddr *)&lstn_addr, sizeof(lstn_addr)) < 0) {
		err_out("bind");
	}

	if (listen(lstn, 10) < 0) {
		err_out("listen");
	}
	msg("listening on *:%d\n", port);

	signal(SIGCHLD, SIG_IGN);

	while (running) {
		struct sockaddr_in sin;
		unsigned int slen = sizeof(sin);
		int con = accept(lstn, (struct sockaddr *)&sin, &slen);
		if (con < 0) {
			err_out("accept");
		}
		msg("new connection from %s:%d\n", inet_ntoa(sin.sin_addr), sin.sin_port);

		switch (fork()) {
		case -1:
			err_out("fork");
		case 0:
			dup2(con, STDIN_FILENO);
			dup2(con, STDOUT_FILENO);
			dup2(con, STDERR_FILENO);
			if (con != STDIN_FILENO  &&
			    con != STDOUT_FILENO &&
			    con != STDERR_FILENO)
				close(con);
			child();
			break;
		default:
			close(con);
			break;
		}
	}
	return EXIT_SUCCESS;
}
