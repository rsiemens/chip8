CFLAGS := -std=c99 -Wall

all:
	cc $(CFLAGS) -O2 src/*.c -L/opt/homebrew/lib -lSDL2 -I/opt/homebrew/include -o chip8

debug:
	cc $(CFLAGS) -DDEBUG -O0 -g src/*.c -L/opt/homebrew/lib -lSDL2 -I/opt/homebrew/include -o chip8

