#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL_mixer.h>

typedef struct {
    Mix_Chunk *shoot;
    Mix_Chunk *hit;
    Mix_Music *bgMusic;
} GameSounds;

int        initAudio(void);
Mix_Chunk *loadSound(const char *path);
void       playSound(Mix_Chunk *sound);
void       cleanupAudio(void);

void loadGameSounds(GameSounds *sounds);
void freeGameSounds(GameSounds *sounds);

#endif
