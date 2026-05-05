#include <SDL2/SDL.h>
#include <stdio.h>
#include "network.h"
#include "constants.h"

static int passes = 0, failures = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { printf("[PASS] %s\n", msg); passes++; } \
    else       { printf("[FAIL] %s\n", msg); failures++; } \
} while (0)

/* Wait up to timeout_ms for cond to become true, polling every 10ms. */
#define WAIT_FOR(cond, timeout_ms) do { \
    for (int _i = 0; _i < (timeout_ms) / 10; _i++) { \
        if (cond) break; \
        SDL_Delay(10); \
    } \
} while (0)

static void test_server_starts(void)
{
    int r = startServerThread();
    ASSERT(r == 0, "startServerThread returns 0");
    SDL_Delay(150); /* give server time to bind port */
}

static void test_client_connects(void)
{
    int r = networkClientConnect("127.0.0.1");
    ASSERT(r == 0, "networkClientConnect returns 0");

    WAIT_FOR(networkClientIsConnected(), 1000);
    ASSERT(networkClientIsConnected(), "client is connected within 1s");
}

static void test_initial_game_state(void)
{
    SDL_Delay(50); /* wait for at least one state broadcast */

    NetGameState state = {0};
    int running = networkClientGetState(&state);

    ASSERT(running,                        "getState returns running=1");
    ASSERT(state.lives == PLAYER_LIVES,    "initial lives == PLAYER_LIVES");
    ASSERT(state.gameOver == 0,            "game not over at start");
    ASSERT(state.score == 0,               "initial score is 0");
    ASSERT(state.connectedPlayers == 1,    "1 connected player");
    ASSERT(state.players[0].active,        "player 0 is active");
}

static void test_input_accepted(void)
{
    /* Drive input for a few ticks and verify the state keeps arriving. */
    networkClientSetInput(1, 0, 0, 0);
    SDL_Delay(50);

    NetGameState state = {0};
    int running = networkClientGetState(&state);
    ASSERT(running, "getState still running after sending input");

    networkClientSetInput(0, 0, 0, 0);
}

static void test_disconnect(void)
{
    networkClientDisconnect();
    SDL_Delay(200); /* wait for client thread to exit */

    NetGameState state = {0};
    int running = networkClientGetState(&state);
    ASSERT(!running, "getState returns running=0 after disconnect");
    ASSERT(!networkClientIsConnected(), "isConnected is false after disconnect");
}

int main(void)
{
    SDL_Init(SDL_INIT_TIMER);

    printf("=== Network Tests ===\n");
    test_server_starts();
    test_client_connects();
    test_initial_game_state();
    test_input_accepted();
    test_disconnect();

    printf("\n%d passed, %d failed\n", passes, failures);

    SDL_Quit();
    return failures > 0 ? 1 : 0;
}
