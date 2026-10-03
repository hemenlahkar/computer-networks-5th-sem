CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2
TARGET = crc_sim
OBJS = main.o crc.o channel.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c crc.h channel.h
	$(CC) $(CFLAGS) -c main.c

crc.o: crc.c crc.h
	$(CC) $(CFLAGS) -c crc.c

channel.o: channel.c channel.h
	$(CC) $(CFLAGS) -c channel.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean