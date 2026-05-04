# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Build (Linux/macOS)
make

# Build and run
make run

# Clean
make clean

# Windows (MSYS2/MinGW) — use Makefile_win
make -f Makefile_win
```

**Linux dependency note:** The Makefile defaults `SDL_PATH` to `/opt/homebrew` (macOS). On Linux, override it:
```bash
make SDL_PATH=/usr
```
or adjust the Makefile directly to point at the correct SDL2 installation prefix.

**Runtime:** The game must be launched from the project root so relative resource paths (`resources/...`) resolve correctly.

**Multiplayer:** Pass a server IP as the first argument to connect as a client:
```bash
./game 192.168.1.100
```
Without an argument it defaults to `127.0.0.1` (defined as `SERVER_IP` in `include/network.h`).

## Architecture

Single-executable SDL2 shooter with optional 2-player network co-op.

### Game modes (main.c)

`PlayMode` switches between `MODE_LOCAL` (single-player, all logic in `main.c`) and `MODE_CLIENT` (multiplayer, all game logic runs on the server).

**Local mode:** `main.c` owns the full game loop — player movement, bullet spawning, enemy spawning (sinusoidal motion), collision detection, and score/lives tracking.

**Client mode:** `main.c` only sends directional input to the server via `networkClientSetInput()` and renders whatever `NetGameState` comes back. No local physics. The server handles everything.

### Networking (src/network.c / include/network.h)

Server and client each run in their own SDL thread (`SDL_CreateThread`). Communication is TCP via SDL_net.

- **Server thread** (`startServerThread` → `testServerFun`): accepts up to `NET_MAX_PLAYERS` (2) clients, runs `updateServerGame()` in a tight loop at ~125 Hz (`SDL_Delay(8)`), and broadcasts `NetGameState` to every connected client each tick.
- **Client thread** (`networkClientConnect` → `clientThread`): sends `NetInputPacket` then receives `NetGameState` every iteration. State is exchanged with the main thread under `clientMutex`.
- Packet integrity is verified with a magic number (`NET_MAGIC = 0x504C4E45`).
- `NetGameState` is the single shared struct sent verbatim over the wire (no serialization layer).

### UI (src/UI.c / include/UI.h)

`Button` struct holds geometry, texture, hover state, and a string label. `renderMenu()` renders a centered overlay box with buttons laid out by `layoutButtons()`. Hover detection and click handling live in `main.c:handleButtonEvents()`, which compares `b->label` strings.

### Sound (src/sound.c / include/sound.h)

Thin wrapper around SDL_mixer. `initAudio()` → `loadSound()` → `playSound()`. No cleanup is called at exit currently.

### Key constants

| Constant | Value | Location |
|---|---|---|
| `SCREEN_WIDTH/HEIGHT` | 1200×800 | `main.c`, `network.c` |
| `MAX_BULLETS` | 100 | `main.c` |
| `MAX_ENEMIES` | 20 | `main.c` |
| `PLAYER_LIVES` | 4 | `main.c`, `network.c` |
| `PORT` | 8080 | `network.h` |
| `SERVER_IP` | `"127.0.0.1"` | `network.h` |

`SCREEN_WIDTH`, `SCREEN_HEIGHT`, and `PLAYER_LIVES` are duplicated between `main.c` and `network.c` — keep them in sync when changing.
