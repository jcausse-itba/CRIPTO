CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c23 -g -fsanitize=address,undefined
EXEC := stegobmp

.PHONY: all clean

all: $(EXEC)

clean:
	rm -f $(EXEC) > /dev/null 2>&1
	rm *.o > /dev/null 2>&1

#################################################################################################

OBJS := main.o

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c
