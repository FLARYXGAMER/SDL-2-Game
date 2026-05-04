# Architecture

## 1. Module Dependencies

```mermaid
graph TB
    subgraph "Public API"
        NH[network.h]
    end

    subgraph "Game (main.c)"
        ML[Main Loop]
        LP[Local Play Logic]
        CP[Client-Side Prediction]
    end

    subgraph "Server (server.c)"
        SL[Server UDP Loop]
        GS[updateGame]
    end

    subgraph "Client (client.c)"
        CL[Client UDP Loop]
        SB[Snapshot Ring Buffer]
        IN[interpolate]
    end

    subgraph "Shared Utils (network.c)"
        PU[packState / unpackState]
        NI[networkInit / addressEqual]
    end

    subgraph "Wire Format (net_private.h)"
        WS["WireState — 1008 bytes"]
        NIP["NetInputPacket — 24 bytes"]
    end

    subgraph "UI (UI.c)"
        BTN[Button / renderMenu]
    end

    subgraph "Audio (sound.c)"
        SND[loadSound / playSound]
    end

    ML --> LP & CP & BTN & SND
    ML -- "networkClient*\nstartServerThread" --> NH
    NH --> CL & SL
    CP -- "networkClientGetState\nnetworkClientGetInterpolated" --> NH

    CL --> PU & SB & IN
    SL --> PU & GS

    PU --> WS & NIP
    NI --> WS
```

---

## 2. Game State Machine

```mermaid
stateDiagram-v2
    [*] --> MAIN_MENU

    MAIN_MENU --> PLAYING_LOCAL   : "Start" button
    MAIN_MENU --> PLAYING_CLIENT  : "Client" button (networkClientConnect succeeds)
    MAIN_MENU --> MAIN_MENU       : "Server" button (spawns thread, no state change)
    MAIN_MENU --> QUIT            : "Quit" button

    PLAYING_LOCAL  --> PAUSE_MENU : ESC
    PLAYING_CLIENT --> PAUSE_MENU : ESC
    PAUSE_MENU --> PLAYING_LOCAL  : ESC / Resume (if was local)
    PAUSE_MENU --> PLAYING_CLIENT : ESC / Resume (if was client)
    PAUSE_MENU --> QUIT           : "Quit"

    PLAYING_LOCAL  --> MAIN_MENU  : lives ≤ 0 → resetLocalGame
    PLAYING_CLIENT --> MAIN_MENU  : gameOver or disconnect
    QUIT --> [*]
```

---

## 3. Main Loop — Per-Frame Flow

```mermaid
flowchart TD
    A([Frame Start]) --> B[SDL_PollEvent]
    B --> C[handleKeyDown\nESC toggles pause]
    C --> D[handleButtonEvents\nmouse click routing]
    D --> E[SDL_RenderClear]
    E --> F{GameState?}

    F -->|MAIN_MENU| G[renderMenu — 4 buttons\nStart · Quit · Server · Client]
    F -->|PAUSE_MENU| H[renderMenu — 2 buttons\nResume · Quit]
    F -->|QUIT| I[running = false]
    F -->|PLAYING| J{PlayMode?}

    J -->|MODE_LOCAL| K[Arrow keys → plane ±5 px\nclamped to screen]
    K --> L[Auto-fire every 10 frames\nbullet at each gun position]
    L --> M[Bullets move up 8 px/frame\ndeactivate at y < 0]
    M --> N["Enemies: 0.5% spawn chance/frame\nsinusoidal x-drift, move down 2 px"]
    N --> O[Bullet–enemy AABB\n+10 score on hit]
    O --> P[Plane–enemy AABB\n−1 life; reset → MAIN_MENU if 0]
    P --> Q[Render plane · bullets · enemies\nhearts · score text]

    J -->|MODE_CLIENT| R[networkClientSetInput\ncurrent key bitmask]
    R --> S[networkClientGetState\nrawState for prediction]
    S --> T[predX/predY += keys × 5\nclamped to screen]
    T --> U{"err = dist(pred, server)\n> 80 px?"}
    U -->|Yes| V[Snap: predX/Y = server pos]
    U -->|No| W[Blend: pred += err × 0.15]
    V --> X[networkClientGetInterpolated]
    W --> X
    X -->|snapCount = 0| Y[Render 'Connecting...']
    X -->|gameOver| Z[Disconnect → MAIN_MENU\npredInit = 0]
    X -->|playing| AA[Render own player at predX/predY\nothers at interpolated pos]

    G & H & Q & Y & AA --> BB[SDL_RenderPresent\nSDL_Delay 16 ms]
    BB --> A
```

---

## 4. Three-Thread Network Flow

```mermaid
sequenceDiagram
    participant MT as Main Thread
    participant CT as Client Thread
    participant ST as Server Thread

    Note over MT: User clicks "Server"
    MT->>ST: startServerThread() — detached SDL_Thread
    ST->>ST: SDLNet_UDP_Open(8080)

    Note over MT: User clicks "Client"
    MT->>CT: networkClientConnect(host) — detached SDL_Thread
    CT->>CT: SDLNet_UDP_Open(0) — ephemeral port

    loop Every ~16 ms — Client Thread
        CT-->>MT: clientMutex locked
        CT->>ST: UDP NetInputPacket {magic, seq++, L/R/U/D}
        ST->>ST: Drain recv — match sender to slot by IP:port
        ST->>ST: updateGame() — move, shoot, collide
        ST->>CT: UDP WireState {seq++, serverTime, all entities}
        CT->>CT: Drop if wire.sequence ≤ lastRecvSeq
        CT->>CT: unpackState() → NetGameState
        CT-->>MT: Lock mutex, push Snap{state, SDL_GetTicks()}, update clientLatest
    end

    loop Every ~16 ms — Main Thread
        MT->>CT: networkClientGetState() → rawState
        MT->>MT: predX/predY ± keys — reconcile with rawState
        MT->>CT: networkClientGetInterpolated()
        CT->>CT: interpolate(): find two Snaps bracketing now−80ms, lerp
        CT-->>MT: Interpolated NetGameState
        MT->>MT: Render
    end
```

---

## 5. Snapshot Interpolation

```mermaid
flowchart LR
    subgraph "snapBuffer ring  SNAP_BUF = 8"
        direction LR
        S0["snap[oldest]\nt = 84 ms"]
        S1["snap[1]\nt = 100 ms"]
        S2["snap[before]\nt = 116 ms"]
        S3["snap[after]\nt = 132 ms"]
        S4["snap[newest]\nt = 148 ms"]
    end

    RT["renderTime\n= now − 80 ms\n= 124 ms"]

    S3 -- "132 > 124 → after" --> RT
    S2 -- "116 ≤ 124 → before" --> RT

    RT --> LF["t = (124−116) / (132−116) = 0.5\n\nx = before.x·0.5 + after.x·0.5\ny = before.y·0.5 + after.y·0.5\nactive = after.active"]
```

---

## 6. UDP Wire Format

```mermaid
packet-beta
  0-31: "magic (0x504C4E45)"
  32-63: "sequence"
  64-95: "serverTime"
  96-127: "playerId"
  128-159: "connectedPlayers"
  160-191: "lives"
  192-223: "score"
  224-255: "gameOver"
  256-351: "players[2]  (2 × WireObj = 16 B)"
  352-6751: "bullets[100]  (100 × WireObj = 800 B)"
  6752-7167: "enemies[20]  (20 × WireObj = 160 B)"
```

> `WireObj` = `Sint16 x` + `Sint16 y` + `Sint32 active` = **8 bytes**
> Total `WireState` = **1008 bytes** (fits in one UDP datagram; old TCP format was ~1492 bytes)
