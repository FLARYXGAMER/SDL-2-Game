#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "../include/network.h"

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

void testServerFun(void)
{
    IPaddress ip;
    TCPsocket serverSocket;
    TCPsocket clientSocket;
    char buffer[1024] = {0};

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

    printf("Server: waiting for connection on port %d...\n", PORT);

    do
    {
        clientSocket = SDLNet_TCP_Accept(serverSocket);
        SDL_Delay(10);
    } while (!clientSocket);

    SDLNet_TCP_Recv(clientSocket, buffer, sizeof(buffer) - 1);
    printf("Server received: %s\n", buffer);
    SDLNet_TCP_Send(clientSocket, "Hello from server!", 18);

    SDLNet_TCP_Close(clientSocket);
    SDLNet_TCP_Close(serverSocket);
}

void testClientFun(void)
{
    IPaddress ip;
    TCPsocket socket;
    char buffer[1024] = {0};

    if (networkInit() != 0)
        return;

    if (SDLNet_ResolveHost(&ip, SERVER_IP, PORT) < 0)
    {
        printf("Client: could not resolve server: %s\n", SDLNet_GetError());
        return;
    }

    socket = SDLNet_TCP_Open(&ip);
    if (!socket)
    {
        printf("Client: connection failed: %s\n", SDLNet_GetError());
        return;
    }

    printf("Client: connected!\n");
    SDLNet_TCP_Send(socket, "Hello from client!", 18);
    SDLNet_TCP_Recv(socket, buffer, sizeof(buffer) - 1);
    printf("Client received: %s\n", buffer);

    SDLNet_TCP_Close(socket);
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
