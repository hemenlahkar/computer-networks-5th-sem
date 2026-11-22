# Makefile
CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -Wconversion
TARGET = gobackn_sim
OBJS = main.o gobackn.o channel.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c gobackn.h channel.h
	$(CC) $(CFLAGS) -c $<

gobackn.o: gobackn.c gobackn.h
	$(CC) $(CFLAGS) -c $<

channel.o: channel.c channel.h gobackn.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
