#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL_mixer.h>

int initAudio();
Mix_Chunk* loadSound(const char* path);
void playSound(Mix_Chunk* sound);
void cleanupAudio();

#endif