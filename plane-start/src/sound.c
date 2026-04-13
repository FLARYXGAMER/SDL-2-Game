#include "sound.h"
#include <stdio.h>

int initAudio() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Mixer Error: %s\n", Mix_GetError());
        return 0;
    }
    return 1;
}

Mix_Chunk* loadSound(const char* path) {
    Mix_Chunk* sound = Mix_LoadWAV(path);
    if (!sound) {
        printf("Failed to load sound: %s\n", Mix_GetError());
    }
    return sound;
}

void playSound(Mix_Chunk* sound) {
    if (sound) {
        Mix_PlayChannel(-1, sound, 0);
    }
}

void cleanupAudio() {
    Mix_CloseAudio();
    Mix_Quit();
}