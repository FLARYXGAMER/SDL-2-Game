#ifndef NETWORK_H
#define NETWORK_H

#define NET_MAX_PLAYERS 2
#define NET_MAX_BULLETS 100
#define NET_MAX_ENEMIES 20
#define PORT 8080
#define SERVER_IP "127.0.0.1"

typedef struct {
    float x;
    float y;
    int active;
} NetObject;

typedef struct {
    int playerId;
    int connectedPlayers;
    int lives;
    int score;
    int gameOver;
    NetObject players[NET_MAX_PLAYERS];
    NetObject bullets[NET_MAX_BULLETS];
    NetObject enemies[NET_MAX_ENEMIES];
} NetGameState;

void testServerFun(void);
void testClientFun(void);
int startServerThread(void);
int networkClientConnect(const char* host);
void networkClientDisconnect(void);
int networkClientIsConnected(void);
int networkClientGetPlayerId(void);
void networkClientSetInput(int left, int right, int up, int down);
int networkClientGetState(NetGameState* state);

#endif
