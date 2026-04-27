#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "../include/network.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800
#define PLAYER_LIVES 4
#define NET_MAGIC 0x504C4E45u

typedef struct {
    Uint32 magic;
    int playerId;
} NetWelcomePacket;

typedef struct {
    Uint32 magic;
    int left;
    int right;
    int up;
    int down;
} NetInputPacket;

typedef struct {
    TCPsocket socket;
    int connected;
    NetInputPacket input;
} ServerClient;

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

    if (SDLNet_Init() < 0)
    {
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

    while (received < size)
    {
        int result = SDLNet_TCP_Recv(socket, cursor + received, size - received);
        if (result <= 0)
            return 0;
        received += result;
    }

    return 1;
}

static void addBullet(NetGameState* state, float x, float y)
{
    for (int i = 0; i < NET_MAX_BULLETS; i++)
    {
        if (!state->bullets[i].active)
        {
            state->bullets[i].x = x;
            state->bullets[i].y = y;
            state->bullets[i].active = 1;
            return;
        }
    }
}

static int intersects(float ax, float ay, int aw, int ah, float bx, float by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void updateServerGame(NetGameState* state, ServerClient clients[], int* shootTimer)
{
    for (int i = 0; i < NET_MAX_PLAYERS; i++)
    {
        if (!clients[i].connected)
        {
            state->players[i].active = 0;
            continue;
        }

        state->players[i].active = 1;

        if (clients[i].input.left)  state->players[i].x -= 5;
        if (clients[i].input.right) state->players[i].x += 5;
        if (clients[i].input.up)    state->players[i].y -= 5;
        if (clients[i].input.down)  state->players[i].y += 5;

        if (state->players[i].x < 0) state->players[i].x = 0;
        if (state->players[i].y < 0) state->players[i].y = 0;
        if (state->players[i].x > SCREEN_WIDTH - 64) state->players[i].x = SCREEN_WIDTH - 64;
        if (state->players[i].y > SCREEN_HEIGHT - 64) state->players[i].y = SCREEN_HEIGHT - 64;
    }

    (*shootTimer)++;
    if (*shootTimer >= 10)
    {
        *shootTimer = 0;
        for (int i = 0; i < NET_MAX_PLAYERS; i++)
        {
            if (!state->players[i].active)
                continue;

            addBullet(state, state->players[i].x + 8, state->players[i].y);
            addBullet(state, state->players[i].x + 40, state->players[i].y);
        }
    }

    for (int i = 0; i < NET_MAX_BULLETS; i++)
    {
        if (!state->bullets[i].active)
            continue;

        state->bullets[i].y -= 8;
        if (state->bullets[i].y < 0)
            state->bullets[i].active = 0;
    }

    for (int i = 0; i < NET_MAX_ENEMIES; i++)
    {
        if (!state->enemies[i].active && rand() % 1000 < 5)
        {
            state->enemies[i].x = rand() % (SCREEN_WIDTH - 64);
            state->enemies[i].y = -64;
            state->enemies[i].active = 1;
        }

        if (state->enemies[i].active)
        {
            state->enemies[i].y += 2;
            state->enemies[i].x += sinf((float)SDL_GetTicks() / 250.0f + i) * 1.5f;

            if (state->enemies[i].y > SCREEN_HEIGHT)
                state->enemies[i].active = 0;
        }
    }

    for (int i = 0; i < NET_MAX_BULLETS; i++)
    for (int j = 0; j < NET_MAX_ENEMIES; j++)
    {
        if (!state->bullets[i].active || !state->enemies[j].active)
            continue;

        if (intersects(state->bullets[i].x, state->bullets[i].y, 16, 16,
                       state->enemies[j].x, state->enemies[j].y, 64, 64))
        {
            state->bullets[i].active = 0;
            state->enemies[j].active = 0;
            state->score += 10;
        }
    }

    for (int i = 0; i < NET_MAX_PLAYERS; i++)
    for (int j = 0; j < NET_MAX_ENEMIES; j++)
    {
        if (!state->players[i].active || !state->enemies[j].active)
            continue;

        if (intersects(state->players[i].x, state->players[i].y, 64, 64,
                       state->enemies[j].x, state->enemies[j].y, 64, 64))
        {
            state->enemies[j].active = 0;
            state->lives--;
            if (state->lives <= 0)
                state->gameOver = 1;
        }
    }
}

void testServerFun(void)
{
    IPaddress ip;
    TCPsocket serverSocket;
    SDLNet_SocketSet socketSet;
    ServerClient clients[NET_MAX_PLAYERS] = {0};
    NetGameState state = {0};
    int shootTimer = 0;

    if (networkInit() != 0)
        return;

    if (SDLNet_ResolveHost(&ip, NULL, PORT) < 0)
    {
        printf("Server: could not resolve host: %s\n", SDLNet_GetError());
        return;
    }

    serverSocket = SDLNet_TCP_Open(&ip);
    if (!serverSocket)
    {
        printf("Server: could not open socket: %s\n", SDLNet_GetError());
        return;
    }

    socketSet = SDLNet_AllocSocketSet(NET_MAX_PLAYERS);
    if (!socketSet)
    {
        printf("Server: could not allocate socket set: %s\n", SDLNet_GetError());
        SDLNet_TCP_Close(serverSocket);
        return;
    }

    state.lives = PLAYER_LIVES;
    state.players[0].x = SCREEN_WIDTH / 2 - 120;
    state.players[0].y = SCREEN_HEIGHT - 80;
    state.players[1].x = SCREEN_WIDTH / 2 + 56;
    state.players[1].y = SCREEN_HEIGHT - 80;

    printf("Server: waiting for up to %d players on port %d...\n", NET_MAX_PLAYERS, PORT);

    while (!state.gameOver)
    {
        TCPsocket newSocket = SDLNet_TCP_Accept(serverSocket);
        if (newSocket)
        {
            int slot = -1;
            for (int i = 0; i < NET_MAX_PLAYERS; i++)
                if (!clients[i].connected && slot == -1)
                    slot = i;

            if (slot >= 0)
            {
                NetWelcomePacket welcome = {NET_MAGIC, slot};
                clients[slot].socket = newSocket;
                clients[slot].connected = 1;
                clients[slot].input.magic = NET_MAGIC;
                SDLNet_TCP_AddSocket(socketSet, newSocket);
                sendAll(newSocket, &welcome, sizeof(welcome));
                printf("Server: player %d connected\n", slot + 1);
            }
            else
            {
                SDLNet_TCP_Close(newSocket);
            }
        }

        SDLNet_CheckSockets(socketSet, 0);
        for (int i = 0; i < NET_MAX_PLAYERS; i++)
        {
            if (clients[i].connected && SDLNet_SocketReady(clients[i].socket))
            {
                if (!recvAll(clients[i].socket, &clients[i].input, sizeof(clients[i].input)) ||
                    clients[i].input.magic != NET_MAGIC)
                {
                    SDLNet_TCP_DelSocket(socketSet, clients[i].socket);
                    SDLNet_TCP_Close(clients[i].socket);
                    clients[i].connected = 0;
                    printf("Server: player %d disconnected\n", i + 1);
                }
            }
        }

        state.connectedPlayers = 0;
        for (int i = 0; i < NET_MAX_PLAYERS; i++)
            if (clients[i].connected)
                state.connectedPlayers++;

        updateServerGame(&state, clients, &shootTimer);

        for (int i = 0; i < NET_MAX_PLAYERS; i++)
        {
            if (!clients[i].connected)
                continue;

            state.playerId = i;
            if (!sendAll(clients[i].socket, &state, sizeof(state)))
            {
                SDLNet_TCP_DelSocket(socketSet, clients[i].socket);
                SDLNet_TCP_Close(clients[i].socket);
                clients[i].connected = 0;
            }
        }

        SDL_Delay(8);
    }

    for (int i = 0; i < NET_MAX_PLAYERS; i++)
        if (clients[i].connected)
            SDLNet_TCP_Close(clients[i].socket);

    SDLNet_FreeSocketSet(socketSet);
    SDLNet_TCP_Close(serverSocket);
}

static int clientThread(void* arg)
{
    const char* host = (const char*)arg;
    IPaddress ip;
    NetWelcomePacket welcome;

    if (networkInit() != 0)
        return -1;

    if (SDLNet_ResolveHost(&ip, host, PORT) < 0)
    {
        printf("Client: could not resolve server: %s\n", SDLNet_GetError());
        clientRunning = 0;
        return -1;
    }

    clientSocket = SDLNet_TCP_Open(&ip);
    if (!clientSocket)
    {
        printf("Client: connection failed: %s\n", SDLNet_GetError());
        clientRunning = 0;
        return -1;
    }

    if (!recvAll(clientSocket, &welcome, sizeof(welcome)) || welcome.magic != NET_MAGIC)
    {
        printf("Client: invalid welcome packet\n");
        SDLNet_TCP_Close(clientSocket);
        clientSocket = NULL;
        clientRunning = 0;
        return -1;
    }

    SDL_LockMutex(clientMutex);
    clientPlayerId = welcome.playerId;
    clientConnected = 1;
    SDL_UnlockMutex(clientMutex);

    printf("Client: connected as player %d\n", welcome.playerId + 1);

    while (clientRunning)
    {
        NetInputPacket inputCopy;
        NetGameState stateCopy;

        SDL_LockMutex(clientMutex);
        inputCopy = clientInput;
        SDL_UnlockMutex(clientMutex);

        if (!sendAll(clientSocket, &inputCopy, sizeof(inputCopy)))
            break;

        if (!recvAll(clientSocket, &stateCopy, sizeof(stateCopy)))
            break;

        SDL_LockMutex(clientMutex);
        clientState = stateCopy;
        clientPlayerId = stateCopy.playerId;
        SDL_UnlockMutex(clientMutex);

    }

    SDL_LockMutex(clientMutex);
    clientConnected = 0;
    clientRunning = 0;
    SDL_UnlockMutex(clientMutex);

    if (clientSocket)
    {
        SDLNet_TCP_Close(clientSocket);
        clientSocket = NULL;
    }

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
    if (!thread)
    {
        printf("Server: could not start thread: %s\n", SDL_GetError());
        return -1;
    }

    SDL_DetachThread(thread);
    return 0;
}

int networkClientConnect(const char* host)
{
    if (clientRunning)
        return 0;

    if (!clientMutex)
        clientMutex = SDL_CreateMutex();

    memset(&clientState, 0, sizeof(clientState));
    clientInput = (NetInputPacket){NET_MAGIC, 0, 0, 0, 0};
    clientConnected = 0;
    clientPlayerId = -1;
    clientRunning = 1;

    clientThreadHandle = SDL_CreateThread(clientThread, "clientThread", (void*)host);
    if (!clientThreadHandle)
    {
        printf("Client: could not start thread: %s\n", SDL_GetError());
        clientRunning = 0;
        return -1;
    }

    SDL_DetachThread(clientThreadHandle);
    return 0;
}

void networkClientDisconnect(void)
{
    if (clientMutex)
    {
        SDL_LockMutex(clientMutex);
        clientConnected = 0;
        SDL_UnlockMutex(clientMutex);
    }

    clientRunning = 0;
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
    clientInput.magic = NET_MAGIC;
    clientInput.left = left;
    clientInput.right = right;
    clientInput.up = up;
    clientInput.down = down;
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
