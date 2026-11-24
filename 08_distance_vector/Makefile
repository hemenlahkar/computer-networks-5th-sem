CC     = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -Wconversion
TARGET = dv_sim
OBJS   = main.o dv.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c dv.h
	$(CC) $(CFLAGS) -c $<

dv.o: dv.c dv.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean