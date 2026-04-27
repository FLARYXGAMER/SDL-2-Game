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
SRC = $(wildcard src/*.c)

# Build rule
all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

# Run rule
run: all
	./$(TARGET)

# Clean rule
clean:
ifeq ($(OS),Windows_NT)
	del /Q $(TARGET)
else
	rm -f $(TARGET)
endif
