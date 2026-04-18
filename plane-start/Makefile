CC = gcc

# Change this to where SDL2 is installed
SDL_PATH = C:/SDL2

# Include paths (your project + SDL)
CFLAGS = -Iinclude -I$(SDL_PATH)/include

# Linker flags (IMPORTANT: SDL2_mixer added)
LDFLAGS = -L$(SDL_PATH)/lib -lSDL2_mixer -lSDL2_ttf -lSDL2_image -lSDL2 -lm

# Source files
SRC = $(wildcard src/*.c)

# Output binary
TARGET = game.exe

# Build rule
all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

# Run rule
run: all
	./$(TARGET)

# Clean rule
clean:
	del /Q $(TARGET)
