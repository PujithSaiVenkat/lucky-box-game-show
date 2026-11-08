CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: game.dll luckybox.exe

game.dll: game.c game.h
	$(CC) $(CFLAGS) -shared -o game.dll game.c

luckybox.exe: main.c game.h game.dll
	$(CC) $(CFLAGS) -o lahari.exe main.c -L. -lgame

clean:
	rm -f game.dll luckybox.exe results.txt

.PHONY: all clean