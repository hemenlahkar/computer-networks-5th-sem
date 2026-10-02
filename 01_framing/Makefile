CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2
TARGET = framing_sim
OBJS = main.o framing.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c framing.h
	$(CC) $(CFLAGS) -c main.c

framing.o: framing.c framing.h
	$(CC) $(CFLAGS) -c framing.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean