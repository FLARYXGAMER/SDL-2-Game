CC = gcc

ifeq ($(OS),Windows_NT)
SDL_PATH = C:/msys64/mingw64
TARGET = game.exe
LDFLAGS = -L$(SDL_PATH)/lib -lmingw32 -lSDL2main -lSDL2_net -lSDL2_mixer -lSDL2_ttf -lSDL2_image -lSDL2 -lws2_32 -lm
else
SDL_PATH = /opt/homebrew
TARGET = game
LDFLAGS = -L$(SDL_PATH)/lib -lSDL2_net -lSDL2_mixer -lSDL2_ttf -lSDL2_image -lSDL2 -lm
endif

# Include paths
CFLAGS = -Iinclude -I$(SDL_PATH)/include

# Source files
SRC     = $(wildcard src/*.c)
SRC_LIB = src/game.c src/UI.c src/sound.c src/network.c src/server.c

# Build rule
all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

# Run rule
run: all
	./$(TARGET)

# Test rules
test_network: tests/test_network.c src/network.c src/server.c
	$(CC) tests/test_network.c src/network.c src/server.c $(CFLAGS) -o test_network $(LDFLAGS)

test_game: tests/test_game.c $(SRC_LIB)
	$(CC) tests/test_game.c $(SRC_LIB) $(CFLAGS) -o test_game $(LDFLAGS)

test: test_network test_game
	./test_network
	./test_game

# Clean rule
clean:
ifeq ($(OS),Windows_NT)
	del /Q $(TARGET)
else
	rm -f $(TARGET) test_network test_game
endif
