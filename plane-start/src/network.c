#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "../include/constants.h"
#include "../include/network.h"

#define NET_MAGIC 0x504C4E45u
#define PLAYER_SIZE 64
#define BULLET_SIZE 16
#define PLAYER_SPEED 5
#define BULLET_SPEED 8
#define ENEMY_SPEED 2
#define SHOOT_DELAY 10

typedef struct { Uint32 magic; int playerId; } NetWelcomePacket;
typedef struct { Uint32 magic; int left, right, up, down; } NetInputPacket;
typedef struct { TCPsocket socket; int connected; NetInputPacket input; } ServerClient;

static SDL_mutex* clientMutex = NULL;
static SDL_Thread* clientThreadHandle = NULL;
static TCPsocket clientSocket = NULL;
static int clientConnected = 0;
static int clientRunning = 0;
static int clientPlayerId = -1;
static NetInputPacket clientInput = {NET_MAGIC, 0, 0, 0, 0};
static NetGameState clientState;

static int networkInit(void)
{
    static int initialized = 0;
    if (initialized)
        return 0;

    if (SDLNet_Init() < 0) {
        printf("Network: SDL_net init failed: %s\n", SDLNet_GetError());
        return -1;
    }

    initialized = 1;
    return 0;
}

static int sendAll(TCPsocket socket, const void* data, int size)
{
    return SDLNet_TCP_Send(socket, data, size) == size;
}

static int recvAll(TCPsocket socket, void* data, int size)
{
    char* cursor = (char*)data;
    int received = 0;

    while (received < size) {
        int result = SDLNet_TCP_Recv(socket, cursor + received, size - received);
        if (result <= 0)
            return 0;
        received += result;
    }

    return 1;
}

static float clampf(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static int intersects(float ax, float ay, int aw, int ah, float bx, float by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void setClientStatus(int connected, int running)
{
    SDL_LockMutex(clientMutex);
    clientConnected = connected;
    clientRunning = running;
    SDL_UnlockMutex(clientMutex);
}

static int isClientRunning(void)
{
    int running;
    SDL_LockMutex(clientMutex);
    running = clientRunning;
    SDL_UnlockMutex(clientMutex);
    return running;
}

static void closeClientSocket(void)
{
    if (clientSocket) {
        SDLNet_TCP_Close(clientSocket);
        clientSocket = NULL;
    }
}

static void addBullet(NetGameState* state, float x, float y)
{
    for (int i = 0; i < NET_MAX_BULLETS; i++) {
        if (!state->bullets[i].active) {
            state->bullets[i] = (NetObject){x, y, 1};
            return;
        }
    }
}

static void initServerState(NetGameState* state)
{
    memset(state, 0, sizeof(*state));
    state->lives = PLAYER_LIVES;
    state->players[0] = (NetObject){SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT - 80, 0};
    state->players[1] = (NetObject){SCREEN_WIDTH / 2 + 56, SCREEN_HEIGHT - 80, 0};
}

static int openServer(TCPsocket* serverSocket, SDLNet_SocketSet* socketSet)
{
    IPaddress ip;

    if (networkInit() != 0)
        return 0;

    if (SDLNet_ResolveHost(&ip, NULL, PORT) < 0) {
        printf("Server: could not resolve host: %s\n", SDLNet_GetError());
        return 0;
    }

    *serverSocket = SDLNet_TCP_Open(&ip);
    if (!*serverSocket) {
        printf("Server: could not open socket: %s\n", SDLNet_GetError());
        return 0;
    }

    *socketSet = SDLNet_AllocSocketSet(NET_MAX_PLAYERS);
    if (*socketSet)
        return 1;

    printf("Server: could not allocate socket set: %s\n", SDLNet_GetError());
    SDLNet_TCP_Close(*serverSocket);
    return 0;
}

static void disconnectServerClient(SDLNet_SocketSet socketSet, ServerClient* client)
{
    if (!client->connected)
        return;

    SDLNet_TCP_DelSocket(socketSet, client->socket);
    SDLNet_TCP_Close(client->socket);
    client->socket = NULL;
    client->connected = 0;
}

static void acceptClients(TCPsocket serverSocket, SDLNet_SocketSet socketSet, ServerClient clients[])
{
    TCPsocket socket = SDLNet_TCP_Accept(serverSocket);
    if (!socket)
        return;

    int slot = -1;
    for (int i = 0; i < NET_MAX_PLAYERS && slot < 0; i++)
        if (!clients[i].connected)
            slot = i;

    if (slot < 0) {
        SDLNet_TCP_Close(socket);
        return;
    }

    NetWelcomePacket welcome = {NET_MAGIC, slot};
    clients[slot] = (ServerClient){socket, 1, {NET_MAGIC, 0, 0, 0, 0}};
    SDLNet_TCP_AddSocket(socketSet, socket);
    sendAll(socket, &welcome, sizeof(welcome));
    printf("Server: player %d connected\n", slot + 1);
}

static void readInputs(SDLNet_SocketSet socketSet, ServerClient clients[])
{
    SDLNet_CheckSockets(socketSet, 0);

    for (int i = 0; i < NET_MAX_PLAYERS; i++) {
        if (!clients[i].connected || !SDLNet_SocketReady(clients[i].socket))
            continue;

        if (!recvAll(clients[i].socket, &clients[i].input, sizeof(clients[i].input)) ||
            clients[i].input.magic != NET_MAGIC) {
            disconnectServerClient(socketSet, &clients[i]);
            printf("Server: player %d disconnected\n", i + 1);
        }
    }
}

static void sendState(SDLNet_SocketSet socketSet, ServerClient clients[], NetGameState* state)
{
    for (int i = 0; i < NET_MAX_PLAYERS; i++) {
        if (!clients[i].connected)
            continue;

        state->playerId = i;
        if (!sendAll(clients[i].socket, state, sizeof(*state)))
            disconnectServerClient(socketSet, &clients[i]);
    }
}

static void updateServerGame(NetGameState* state, ServerClient clients[], int* shootTimer)
{
    state->connectedPlayers = 0;

    for (int i = 0; i < NET_MAX_PLAYERS; i++) {
        NetObject* player = &state->players[i];
        player->active = clients[i].connected;
        if (!clients[i].connected)
            continue;

        state->connectedPlayers++;
        if (clients[i].input.left)  player->x -= PLAYER_SPEED;
        if (clients[i].input.right) player->x += PLAYER_SPEED;
        if (clients[i].input.up)    player->y -= PLAYER_SPEED;
        if (clients[i].input.down)  player->y += PLAYER_SPEED;
        player->x = clampf(player->x, 0, SCREEN_WIDTH - PLAYER_SIZE);
        player->y = clampf(player->y, 0, SCREEN_HEIGHT - PLAYER_SIZE);
    }

    if (++(*shootTimer) >= SHOOT_DELAY) {
        *shootTimer = 0;
        for (int i = 0; i < NET_MAX_PLAYERS; i++) {
            if (state->players[i].active) {
                addBullet(state, state->players[i].x + 8, state->players[i].y);
                addBullet(state, state->players[i].x + 40, state->players[i].y);
            }
        }
    }

    for (int i = 0; i < NET_MAX_BULLETS; i++) {
        if (!state->bullets[i].active)
            continue;

        state->bullets[i].y -= BULLET_SPEED;
        if (state->bullets[i].y < 0)
            state->bullets[i].active = 0;
    }

    for (int i = 0; i < NET_MAX_ENEMIES; i++) {
        NetObject* enemy = &state->enemies[i];
        if (!enemy->active && rand() % 1000 < 5)
            *enemy = (NetObject){rand() % (SCREEN_WIDTH - PLAYER_SIZE), -PLAYER_SIZE, 1};

        if (enemy->active) {
            enemy->y += ENEMY_SPEED;
            enemy->x += sinf((float)SDL_GetTicks() / 250.0f + i) * 1.5f;
            if (enemy->y > SCREEN_HEIGHT)
                enemy->active = 0;
        }
    }

    for (int i = 0; i < NET_MAX_BULLETS; i++)
    for (int j = 0; j < NET_MAX_ENEMIES; j++) {
        NetObject* bullet = &state->bullets[i];
        NetObject* enemy = &state->enemies[j];
        if (!bullet->active || !enemy->active)
            continue;

        if (intersects(bullet->x, bullet->y, BULLET_SIZE, BULLET_SIZE,
                       enemy->x, enemy->y, PLAYER_SIZE, PLAYER_SIZE)) {
            bullet->active = 0;
            enemy->active = 0;
            state->score += 10;
        }
    }

    for (int i = 0; i < NET_MAX_PLAYERS; i++)
    for (int j = 0; j < NET_MAX_ENEMIES; j++) {
        NetObject* player = &state->players[i];
        NetObject* enemy = &state->enemies[j];
        if (!player->active || !enemy->active)
            continue;

        if (intersects(player->x, player->y, PLAYER_SIZE, PLAYER_SIZE,
                       enemy->x, enemy->y, PLAYER_SIZE, PLAYER_SIZE)) {
            enemy->active = 0;
            state->lives--;
            if (state->lives <= 0)
                state->gameOver = 1;
        }
    }
}

void testServerFun(void)
{
    TCPsocket serverSocket = NULL;
    SDLNet_SocketSet socketSet = NULL;
    ServerClient clients[NET_MAX_PLAYERS] = {0};
    NetGameState state;
    int shootTimer = 0;

    if (!openServer(&serverSocket, &socketSet))
        return;

    initServerState(&state);
    printf("Server: waiting for up to %d players on port %d...\n", NET_MAX_PLAYERS, PORT);

    while (!state.gameOver) {
        acceptClients(serverSocket, socketSet, clients);
        readInputs(socketSet, clients);
        updateServerGame(&state, clients, &shootTimer);
        sendState(socketSet, clients, &state);
        SDL_Delay(8);
    }

    for (int i = 0; i < NET_MAX_PLAYERS; i++)
        disconnectServerClient(socketSet, &clients[i]);

    SDLNet_FreeSocketSet(socketSet);
    SDLNet_TCP_Close(serverSocket);
}

static int clientThread(void* arg)
{
    const char* host = (const char*)arg;
    IPaddress ip;
    NetWelcomePacket welcome;

    if (networkInit() != 0 ||
        SDLNet_ResolveHost(&ip, host, PORT) < 0 ||
        !(clientSocket = SDLNet_TCP_Open(&ip))) {
        printf("Client: connection failed: %s\n", SDLNet_GetError());
        setClientStatus(0, 0);
        return -1;
    }

    if (!recvAll(clientSocket, &welcome, sizeof(welcome)) || welcome.magic != NET_MAGIC) {
        printf("Client: invalid welcome packet\n");
        setClientStatus(0, 0);
        closeClientSocket();
        return -1;
    }

    SDL_LockMutex(clientMutex);
    clientPlayerId = welcome.playerId;
    clientConnected = 1;
    SDL_UnlockMutex(clientMutex);
    printf("Client: connected as player %d\n", welcome.playerId + 1);

    while (isClientRunning()) {
        NetInputPacket input;
        NetGameState state;

        SDL_LockMutex(clientMutex);
        input = clientInput;
        SDL_UnlockMutex(clientMutex);

        if (!sendAll(clientSocket, &input, sizeof(input)) ||
            !recvAll(clientSocket, &state, sizeof(state)))
            break;

        SDL_LockMutex(clientMutex);
        clientState = state;
        clientPlayerId = state.playerId;
        SDL_UnlockMutex(clientMutex);
    }

    setClientStatus(0, 0);
    closeClientSocket();
    return 0;
}

void testClientFun(void)
{
    networkClientConnect(SERVER_IP);
}

static int serverThread(void* arg)
{
    (void)arg;
    testServerFun();
    return 0;
}

int startServerThread(void)
{
    SDL_Thread* thread = SDL_CreateThread(serverThread, "serverThread", NULL);
    if (!thread) {
        printf("Server: could not start thread: %s\n", SDL_GetError());
        return -1;
    }

    SDL_DetachThread(thread);
    return 0;
}

int networkClientConnect(const char* host)
{
    if (!clientMutex) {
        clientMutex = SDL_CreateMutex();
        if (!clientMutex)
            return -1;
    } else if (isClientRunning()) {
        return 0;
    }

    SDL_LockMutex(clientMutex);
    memset(&clientState, 0, sizeof(clientState));
    clientInput = (NetInputPacket){NET_MAGIC, 0, 0, 0, 0};
    clientConnected = 0;
    clientPlayerId = -1;
    clientRunning = 1;
    SDL_UnlockMutex(clientMutex);

    clientThreadHandle = SDL_CreateThread(clientThread, "clientThread", (void*)host);
    if (clientThreadHandle) {
        SDL_DetachThread(clientThreadHandle);
        return 0;
    }

    printf("Client: could not start thread: %s\n", SDL_GetError());
    setClientStatus(0, 0);
    return -1;
}

void networkClientDisconnect(void)
{
    if (clientMutex)
        setClientStatus(0, 0);
}

int networkClientIsConnected(void)
{
    int connected;
    if (!clientMutex)
        return 0;

    SDL_LockMutex(clientMutex);
    connected = clientConnected;
    SDL_UnlockMutex(clientMutex);
    return connected;
}

int networkClientGetPlayerId(void)
{
    int playerId;
    if (!clientMutex)
        return -1;

    SDL_LockMutex(clientMutex);
    playerId = clientPlayerId;
    SDL_UnlockMutex(clientMutex);
    return playerId;
}

void networkClientSetInput(int left, int right, int up, int down)
{
    if (!clientMutex)
        return;

    SDL_LockMutex(clientMutex);
    clientInput = (NetInputPacket){NET_MAGIC, left, right, up, down};
    SDL_UnlockMutex(clientMutex);
}

int networkClientGetState(NetGameState* state)
{
    int running;
    if (!clientMutex || !state)
        return 0;

    SDL_LockMutex(clientMutex);
    *state = clientState;
    running = clientRunning;
    SDL_UnlockMutex(clientMutex);
    return running;
}
