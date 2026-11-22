CC     = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -Wconversion
TARGET = sr_sim
OBJS   = main.o sr.o channel.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c sr.h channel.h
	$(CC) $(CFLAGS) -c $<

sr.o: sr.c sr.h
	$(CC) $(CFLAGS) -c $<

channel.o: channel.c channel.h sr.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean