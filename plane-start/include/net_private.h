#pragma once
// Internal header shared between network.c, server.c, and client.c.
// Not for inclusion from main.c or other game code.

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "../include/network.h"
#include "../include/constants.h"

#define NET_MAGIC         0x504C4E45u
#define CLIENT_TIMEOUT_MS 3000
#define MAX_PKT_SIZE      2048

// Compact wire format: Sint16 positions, Sint32 active for natural alignment.
// sizeof(WireObj) = 8, sizeof(WireState) = 32 + 16 + 800 + 160 = 1008 bytes.
typedef struct {
    Sint16 x, y;
    Sint32 active;
} WireObj;

typedef struct {
    Uint32  magic, sequence, serverTime;
    Sint32  playerId, connectedPlayers, lives, score, gameOver;
    WireObj players[NET_MAX_PLAYERS];
    WireObj bullets[NET_MAX_BULLETS];
    WireObj enemies[NET_MAX_ENEMIES];
} WireState;

typedef struct {
    Uint32 magic, sequence;
    Sint32 left, right, up, down;
} NetInputPacket;

// Shared utilities implemented in network.c
int  networkInit(void);
int  addressEqual(IPaddress a, IPaddress b);
void packState(WireState* wire, const NetGameState* s, int playerId);
void unpackState(NetGameState* s, const WireState* wire);
