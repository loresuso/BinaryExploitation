extern int verbosity;
extern const char *dbfile;
extern const char *licencefile;
extern int daemonized;

#define msg(fmt, ...) do {\
	if (daemonized) {\
		syslog(LOG_DAEMON, fmt, ## __VA_ARGS__);\
	} else {\
		fprintf(stderr, fmt, ## __VA_ARGS__);\
	}\
} while (0)

#define err(m) msg("%s: %s\n", m, strerror(errno))

#define err_out(m) do {\
	err(m);\
	exit(EXIT_FAILURE);\
} while (0)
