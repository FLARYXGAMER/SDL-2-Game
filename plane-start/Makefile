CC = gcc

# Homebrew path (Apple Silicon)
SDL_PATH = /opt/homebrew

# Include paths
CFLAGS = -Iinclude -I$(SDL_PATH)/include

# Linker flags
LDFLAGS = -L$(SDL_PATH)/lib -lSDL2_mixer -lSDL2_ttf -lSDL2_image -lSDL2 -lm

# Source files
SRC = $(wildcard src/*.c)

# Output binary
TARGET = game

# Build rule
all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

# Run rule
run: all
	./$(TARGET)

# Clean rule
clean:
	rm -f $(TARGET)