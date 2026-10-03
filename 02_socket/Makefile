CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2 -D_POSIC_C_SOURCE=200809L -D_GNU_SOURCE

TARGETS = tcp_echo_server tcp_echo_client udp_echo_server udp_echo_client
COMMON = net.o

all: $(TARGETS)

tcp_echo_server: tcp_echo_server.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

tcp_echo_client: tcp_echo_client.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

udp_echo_server: udp_echo_server.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

udp_echo_client: udp_echo_client.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
