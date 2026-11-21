CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L
TARGET = stopwait_sim
OBJS = main.o stopwait.o channel.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c stopwait.h channel.h
	$(CC) $(CFLAGS) -c $<

stopwait.o: stopwait.c stopwait.h
	$(CC) $(CFLAGS) -c $<

channel.o: channel.c channel.h stopwait.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
