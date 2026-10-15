# compiler setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99

# define targets
TARGETS=marching_squares

build: $(TARGETS)

marching_squares: marching_squares.c
	$(CC) $(CFLAGS) marching_squares.c -o marching_squares

clean:
	rm -f $(TARGETS)

.PHONY: pack clean
