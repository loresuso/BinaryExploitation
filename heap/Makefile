all: server_0 server_0b server_1 server_2 objects1 objects2

server_0: server.c malloc-2.7.2.c
	gcc -g -z norelro -no-pie -z execstack -fstack-protector -DSERVER_0 -DPORT=7000 -o server_0 server.c

server_0b: server.c
	gcc -g -z norelro -no-pie -z execstack -fstack-protector -DSERVER_0b -DPORT=7000 -o server_0b server.c

server_1: server.c malloc-2.7.2.c
	gcc -g -no-pie -z relro -z now -z noexecstack -fstack-protector -DSERVER_1 -DPORT=7001 -o server_1 server.c

server_2: server.c malloc-2.7.2.c
	gcc -g -no-pie -z relro -z now -z noexecstack -fstack-protector -DSERVER_2 -DPORT=7002 -o server_2 server.c libc.so.6

objects1: objects.cc malloc-2.7.2.c
	g++ -g -no-pie -z norelro -z execstack -DOBJECTS -DPORT=7003 -o objects1 objects.cc libc.so.6

objects2: objects.cc malloc-2.7.2.c
	g++ -g -pie -z relro -z now -z noexecstack -fstack-protector -DPORT=7004 -DOBJECTS -o objects2 objects.cc libc.so.6

.PHONY: clean
clean:
	rm -f server_? objects?
