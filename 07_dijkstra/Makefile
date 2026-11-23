CC     = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -Wconversion
TARGET = dijkstra_sim
OBJS   = main.o graph.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c graph.h
	$(CC) $(CFLAGS) -c $<

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean