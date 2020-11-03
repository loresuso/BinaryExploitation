DISABLE=-fno-stack-protector -z norelro -Wno-format-security

all: server_0 server_1 server_2

server_0: server.c
	gcc -g $(DISABLE) -no-pie -DSERVER_0 -DPORT=7000 -o server_0 server.c

server_1: server.c
	gcc -g $(DISABLE) -no-pie -DSERVER_1 -DPORT=7001 -o server_1 server.c

server_2: server.c
	gcc -g $(DISABLE) -pie    -DSERVER_2 -DPORT=7002 -o server_2 server.c

.PHONY: clean
clean:
	rm -f server_?
