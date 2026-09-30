CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c23 -g -fsanitize=address,undefined -Iinclude
EXEC := stegobmp

.PHONY: all clean test

all: $(EXEC)

clean:
	rm -f $(EXEC) > /dev/null 2>&1
	rm -f *.o > /dev/null 2>&1
	$(MAKE) -C test clean

test:
	$(MAKE) -C test

#################################################################################################

OBJS := main.o

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c
