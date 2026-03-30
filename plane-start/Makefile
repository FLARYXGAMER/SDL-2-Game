CC = gcc

# Change this to where SDL2 is installed
SDL_PATH = C:/SDL2

CFLAGS = -I$(SDL_PATH)/include
LDFLAGS = -L$(SDL_PATH)/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lm

SRC = $(wildcard src/*.c)
TARGET = game.exe

all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

run: all
	$(TARGET)

clean:
	del /Q $(TARGET)