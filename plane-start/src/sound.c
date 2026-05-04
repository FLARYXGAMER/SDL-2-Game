#include "sound.h"
#include <stdio.h>

int initAudio(void)
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mixer Error: %s\n", Mix_GetError());
        return 0;
    }
    return 1;
}

Mix_Chunk *loadSound(const char *path)
{
    Mix_Chunk *sound = Mix_LoadWAV(path);
    if (!sound)
        printf("Failed to load sound: %s\n", Mix_GetError());
    return sound;
}

void playSound(Mix_Chunk *sound)
{
    if (sound)
        Mix_PlayChannel(-1, sound, 0);
}

void cleanupAudio(void)
{
    Mix_CloseAudio();
    Mix_Quit();
}

void loadGameSounds(GameSounds *sounds)
{
    sounds->shoot   = loadSound("resources/sounds/bullet.wav");
    sounds->hit     = loadSound("resources/sounds/roblox.wav");
    sounds->bgMusic = Mix_LoadMUS("resources/sounds/music.wav");

    if (!sounds->bgMusic)
        printf("Failed to load music.wav: %s\n", Mix_GetError());
    else {
        Mix_VolumeMusic(40);
        Mix_PlayMusic(sounds->bgMusic, -1);
    }
}

void freeGameSounds(GameSounds *sounds)
{
    Mix_FreeChunk(sounds->shoot);
    Mix_FreeChunk(sounds->hit);
    Mix_FreeMusic(sounds->bgMusic);
}
