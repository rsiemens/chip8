CFLAGS := -std=c99 -Wall

all:
	cc $(CFLAGS) -O2 src/*.c -L/opt/homebrew/lib -lSDL2 -I/opt/homebrew/include -o chip8

dev:
	cc $(CFLAGS) -DDEBUG -O2 src/*.c -L/opt/homebrew/lib -lSDL2 -I/opt/homebrew/include -o chip8

