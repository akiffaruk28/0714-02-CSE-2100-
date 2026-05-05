# WORDS_COLLIDE — MVC Architecture Branch

> **Branch purpose:** This branch refactors the active gameplay screen of WORDS_COLLIDE to follow the **Model-View-Controller (MVC)** design pattern. All other game states (Splash, Multiplayer Menu, etc.) remain on the existing `IGameState` state-machine architecture; the MVC pattern is applied exclusively and intentionally to the `PlayState`, which is the most data-heavy and logic-heavy screen in the game.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Why MVC — and Why Here?](#why-mvc--and-why-here)
3. [Architecture at a Glance](#architecture-at-a-glance)
4. [Folder Structure](#folder-structure)
5. [The MVC Triad in Detail](#the-mvc-triad-in-detail)
   - [Model — `PlayModel`](#model--playmodel)
   - [View — `PlayView`](#view--playview)
   - [Controller — `PlayController`](#controller--playcontroller)
6. [How the Pieces Connect — `PlayState` as the Orchestrator](#how-the-pieces-connect--playstate-as-the-orchestrator)
7. [The Full Game Loop (Step by Step)](#the-full-game-loop-step-by-step)
8. [Data Flow Diagram](#data-flow-diagram)
9. [Supporting Systems](#supporting-systems)
   - [GameEngine & IGameState](#gameengine--igamestate)
   - [GameState (the shared data struct)](#gamestate-the-shared-data-struct)
   - [IBoardScorer / BoardScorer](#iboardscorer--boardscorer)
   - [Network](#network)
   - [Audio & Renderer](#audio--renderer)
   - [UI — Button, Label, IDrawable, IInteractable](#ui--button-label-idrawable-iinteractable)
10. [Class Responsibility Table](#class-responsibility-table)
11. [Key Design Decisions](#key-design-decisions)
12. [Building the Project](#building-the-project)

---

## Project Overview

**WORDS_COLLIDE** is a two-player word game built in C++ with SDL2. Players take turns clicking tiles on a 20×20 grid and typing letters to form valid English words. Words are validated against a 200,000-word dictionary; bonus words award extra points. The game supports both local hotseat play and Wi-Fi multiplayer over a raw TCP connection.

**Tech stack:** C++17 · SDL2 · SDL2_ttf · SDL2_mixer · SDL2_net

---

## Why MVC — and Why Here?

Before this branch, all game screens were thin `IGameState` subclasses: they received three callbacks (`handleInput`, `update`, `render`) and did everything inline. That was fine for simple screens like the splash screen or the name-input prompt, which have almost no state of their own.

`PlayState` is different. It owns:

- A mutable 20×20 character grid
- Two player scores, names, and the current active player
- A running game clock and a per-turn countdown
- Real-time keyboard and mouse input
- Incoming TCP packets from the remote player
- All rendering logic for the grid, letters, highlights, timer, and HUD

Cramming all of that into one class creates an unmaintainable monolith. MVC solves this by giving each concern its own home:

| Concern | MVC Role | Class |
|---|---|---|
| What the game data looks like | **Model** | `PlayModel` |
| How the data is drawn on screen | **View** | `PlayView` |
| How input and network events mutate the data | **Controller** | `PlayController` |

The rest of the game screens are simple enough that MVC would add overhead with no benefit, so they stay as plain `IGameState` implementations.

---

## Architecture at a Glance

```
main()
  └── GameEngine          ← owns the SDL window, subsystems, and the state machine
        └── IGameState    ← abstract interface for every screen
              ├── SplashState
              ├── MultiplayerMenuState
              ├── IPInputState
              ├── NameInputState
              ├── GameOverState
              └── PlayState               ← IGameState that hosts the MVC triad
                    ├── PlayModel         ← M: game data + rules
                    ├── PlayView          ← V: rendering
                    └── PlayController    ← C: input + network → model mutations
```

---

## Folder Structure

```
WORDS_COLLIDE/
├── assets/                     # Fonts, dictionary, sound effects, music
├── include/
│   ├── Core/
│   │   ├── Config.hpp          # Compile-time constants (grid size, timers, etc.)
│   │   ├── GameEngine.hpp      # Main loop + state machine
│   │   ├── IGameState.hpp      # Abstract state interface
│   │   └── Types.hpp           # GameState struct, AppContext struct, enums
│   ├── Game/
│   │   ├── Dictionary.hpp      # Word list loader + lookup
│   │   ├── IBoardScorer.hpp    # Abstract scoring interface
│   │   └── BoardScorer.hpp     # Concrete horizontal/vertical word scorer
│   ├── MVC/                    ← This branch's primary addition
│   │   ├── PlayModel.hpp
│   │   ├── PlayView.hpp
│   │   └── PlayController.hpp
│   ├── States/
│   │   ├── PlayState.hpp       ← Orchestrates the MVC triad
│   │   ├── SplashState.hpp
│   │   ├── MultiplayerMenuState.hpp
│   │   ├── IPInputState.hpp
│   │   ├── NameInputState.hpp
│   │   └── GameOverState.hpp
│   ├── Systems/
│   │   ├── Audio.hpp
│   │   ├── InputHandler.hpp
│   │   ├── Network.hpp
│   │   └── Renderer.hpp
│   └── UI/
│       ├── IDrawable.hpp
│       ├── IInteractable.hpp
│       ├── Button.hpp
│       └── Label.hpp
├── src/                        # Mirror of include/, with .cpp implementations
├── Makefile
└── main.cpp
```

---

## The MVC Triad in Detail

### Model — `PlayModel`

**Files:** `include/MVC/PlayModel.hpp` · `src/MVC/PlayModel.cpp`

The Model is the single source of truth for the gameplay screen. It holds no SDL handles, draws nothing, and knows nothing about how the data will be displayed.

```cpp
class PlayModel {
private:
    GameState& data;       // Reference to the engine-owned game state struct
    IBoardScorer& scorer;  // Injected scoring strategy (open/closed principle)

public:
    PlayModel(GameState& state, IBoardScorer& boardScorer);

    void updateTimers(Uint32 currentTime);       // Enforce per-turn time limit
    bool checkTimeUp(Uint32 currentTime) const;  // Has the overall game ended?

    GameState& getData() const;        // Expose data to View (read) and Controller (write)
    IBoardScorer& getScorer() const;   // Expose scorer to Controller
};
```

**What it owns conceptually (via `GameState&`):**

- `grid[20][20]` — the board of placed characters
- `scores[2]` — both players' running totals
- `playerNames[2]` — set during the name-input screen
- `currentPlayer` — whose turn it is (0 or 1)
- `currentLetter`, `selectedX/Y`, `isTileSelected` — the in-progress tile placement
- `gameStartTime`, `turnStartTime`, `lastWarningTime` — SDL tick timestamps

**Timer logic (`updateTimers`):**

Every frame the Controller calls `model->updateTimers(currentTime)`. If the current turn has exceeded `TURN_DURATION_MS` (20 seconds, defined in `Config.hpp`), the Model automatically advances the turn:

```cpp
void PlayModel::updateTimers(Uint32 currentTime) {
    if (currentTime - data.turnStartTime >= TURN_DURATION_MS) {
        data.currentPlayer = (data.currentPlayer + 1) % 2;
        data.isTileSelected = false;
        data.currentLetter = '\0';
        data.turnStartTime = SDL_GetTicks();
        SDL_StopTextInput();
    }
}
```

The Model does **not** trigger any rendering or sound effect here — it only mutates data. The View will pick up the changed `currentPlayer` the next time it reads `getData()`.

---

### View — `PlayView`

**Files:** `include/MVC/PlayView.hpp` · `src/MVC/PlayView.cpp`

The View is read-only with respect to game logic. It reads the Model's data and renders it to screen using the `Renderer` system. It never changes game state.

```cpp
class PlayView {
private:
    PlayModel* model;
    Renderer& renderer;
    Audio& audio;
    AppContext& ctx;
    std::shared_ptr<Button> giveUpBtn;  // UI element owned by the View

public:
    PlayView(PlayModel* m, Renderer& r, Audio& a, AppContext& c);

    void initUI();                               // Create the Give Up button
    void draw();                                 // Called every frame from PlayState::render()
    std::shared_ptr<Button> getGiveUpButton() const;
};
```

**What `draw()` renders each frame:**

1. **Countdown timer** — reads `data.gameStartTime` and `GAME_DURATION_MS`, formats `mm:ss`, draws top-right.
2. **Grid lines** — 21 horizontal and 21 vertical lines forming the 20×20 cell grid.
3. **Placed letters** — iterates `data.grid[i][j]`; for every non-null character draws it at the correct pixel position.
4. **Active tile highlight** — if `data.isTileSelected`, fills that cell with a translucent grey overlay and draws `data.currentLetter` as a preview.
5. **Score HUD** — reads `data.scores[0]` and `data.scores[1]`, draws `"P1: X | P2: Y"` at the bottom.
6. **Give Up button** — delegates to `giveUpBtn->render(renderer)`.

The View gets a **pointer** to the Model, not a copy, so every frame it automatically sees the latest game state without any explicit notification or observer pattern.

---

### Controller — `PlayController`

**Files:** `include/MVC/PlayController.hpp` · `src/MVC/PlayController.cpp`

The Controller sits between SDL events / the network layer and the Model. It is the only component that writes to `GameState`.

```cpp
class PlayController {
private:
    PlayModel* model;
    PlayView* view;
    Network& network;
    Audio& audio;
    AppContext& ctx;

public:
    PlayController(PlayModel* m, PlayView* v, Network& n, Audio& a, AppContext& c);

    int processInput(const SDL_Event& event);  // Returns 1 if "Give Up" was clicked
    void processNetwork();                     // Applies incoming remote moves
};
```

**`processInput` — local player's turn:**

```
SDL_Event arrives
    │
    ├─ Give Up button clicked?
    │      → play win SFX, return 1  (PlayState transitions to GameOverState)
    │
    ├─ Mouse click on empty grid cell?
    │      → set isTileSelected = true, record selectedX/Y, SDL_StartTextInput()
    │
    ├─ SDL_TEXTINPUT event while tile selected?
    │      → if alpha, store uppercase in data.currentLetter
    │
    └─ ENTER key while tile selected and letter chosen?
           → write letter into data.grid[selectedX][selectedY]
           → play place-tile SFX
           → ask scorer.checkAndScore() for points earned
           → if score > 0: play word-found SFX, add to data.scores[currentPlayer]
           → network.sendMove() — broadcast move to remote player
           → advance currentPlayer, reset selection, SDL_StopTextInput()
```

**`processNetwork` — remote player's turn:**

```
network.receiveMove(enemyMove)  ← non-blocking TCP poll
    │
    └─ packet received?
           → play place-tile SFX
           → write letter into data.grid[enemyMove.x][enemyMove.y]
           → ask scorer.checkAndScore() for points
           → if score > 0: play word-found SFX, add to data.scores[currentPlayer]
           → advance currentPlayer, reset selection state
```

The key design point: the Controller does not call any `view->draw()` or `view->update()`. It only mutates data. Rendering happens separately in the same frame through `PlayState::render()`.

---

## How the Pieces Connect — `PlayState` as the Orchestrator

`PlayState` is the glue layer. It is an `IGameState` (so it plugs into the `GameEngine`'s state machine) and it owns the three MVC objects as raw pointers (allocated in `onEnter`, deleted in the destructor).

```cpp
// PlayState.hpp
class PlayState : public IGameState {
private:
    PlayModel*      model;
    PlayView*       view;
    PlayController* controller;
public:
    void onEnter(GameEngine& engine) override;
    void handleInput(GameEngine& engine, const SDL_Event& event) override;
    void update(GameEngine& engine, Uint32 deltaMs) override;
    void render(GameEngine& engine) override;
    ~PlayState();
};
```

**`onEnter`** — runs once when the state becomes active:

```cpp
void PlayState::onEnter(GameEngine& engine) {
    GameState& data = engine.getGameData();
    data.gameStartTime = SDL_GetTicks();
    data.turnStartTime = SDL_GetTicks();
    engine.getAudio().stopMusic();

    model      = new PlayModel(data, engine.getScorer());
    view       = new PlayView(model, engine.getRenderer(), engine.getAudio(), engine.getContext());
    view->initUI();
    controller = new PlayController(model, view, engine.getNetwork(), engine.getAudio(), engine.getContext());
}
```

**`handleInput`** — routes SDL events to the Controller:

```cpp
void PlayState::handleInput(GameEngine& engine, const SDL_Event& event) {
    if (controller->processInput(event) == 1) {
        engine.changeState(std::make_unique<GameOverState>());
    }
}
```

**`update`** — runs game logic each frame:

```cpp
void PlayState::update(GameEngine& engine, Uint32 deltaMs) {
    Uint32 now = SDL_GetTicks();
    if (model->checkTimeUp(now)) {
        engine.getAudio().playSound(engine.getContext().sfxWin);
        engine.changeState(std::make_unique<GameOverState>());
        return;
    }
    model->updateTimers(now);      // Enforce per-turn limit
    controller->processNetwork();  // Consume incoming TCP packets
    // Warning sound logic (reads model data directly via engine.getGameData())
}
```

**`render`** — completely delegates to the View:

```cpp
void PlayState::render(GameEngine& engine) {
    view->draw();
}
```

**`~PlayState`** — cleans up in reverse construction order:

```cpp
PlayState::~PlayState() {
    delete controller;
    delete view;
    delete model;
}
```

---

## The Full Game Loop (Step by Step)

`GameEngine::run()` executes this sequence every frame (~16 ms at 60 fps):

```
① State Transition Check
    └─ If nextState is set, call currentState->onExit(), swap, call onExit()->onEnter()

② Delta Time Calculation
    └─ deltaMs = SDL_GetTicks() - lastTime

③ SDL Event Loop  →  currentState->handleInput(engine, event)
    └─ PlayState::handleInput()
         └─ controller->processInput(event)
              ├─ Give Up?  → return 1 → engine.changeState(GameOverState)
              ├─ Grid click → update model selection fields
              ├─ Text input → update model currentLetter
              └─ Enter key  → commit tile, score, network.sendMove(), advance turn

④ Update  →  currentState->update(engine, deltaMs)
    └─ PlayState::update()
         ├─ model->checkTimeUp()     → transition to GameOverState if expired
         ├─ model->updateTimers()    → auto-advance turn if TURN_DURATION_MS exceeded
         └─ controller->processNetwork()  → apply remote player's move

⑤ Clear Renderer
    └─ renderer.clear({230, 210, 255, 255})   // light purple background

⑥ Render  →  currentState->render(engine)
    └─ PlayState::render()
         └─ view->draw()
              ├─ Timer text
              ├─ Grid lines
              ├─ Placed letters from data.grid
              ├─ Active tile highlight + preview letter
              ├─ Score HUD
              └─ Give Up button

⑦ renderer->present()     // flip to screen
⑧ SDL_Delay(16)           // ~60 fps cap
```

---

## Data Flow Diagram

```
┌────────────────────────────────────────────────────────────┐
│                        GameEngine                          │
│  AppContext  ·  GameState  ·  Audio  ·  Renderer  ·  Net  │
└───────────────────────────┬────────────────────────────────┘
                            │ owns & passes refs
                            ▼
                    ┌───────────────┐
                    │   PlayState   │  (IGameState)
                    │  orchestrates │
                    └──┬────┬────┬──┘
                       │    │    │
            ┌──────────┘    │    └──────────┐
            ▼               ▼               ▼
     ┌────────────┐  ┌────────────┐  ┌──────────────────┐
     │ PlayModel  │  │  PlayView  │  │  PlayController  │
     │            │  │            │  │                  │
     │ holds ref  │◄─┤ reads data │  │ writes to data   │
     │ to         │  │ from model │  │ via model ref    │
     │ GameState  │  │            │  │                  │
     │ &          │  │ renders:   │  │ responds to:     │
     │ IBoardScorer│ │ · grid     │  │ · SDL_Events     │
     │            │  │ · letters  │  │ · Network TCP    │
     │ provides:  │  │ · timer    │  │                  │
     │ · timers   │  │ · scores   │  │ calls:           │
     │ · time-up  │  │ · buttons  │  │ · scorer         │
     └────────────┘  └────────────┘  │ · audio          │
                                     │ · network.send   │
                                     └──────────────────┘

   SDL Events ──────────────────────────────► PlayController
   Network (TCP) ───────────────────────────► PlayController
   GameState mutations ─────────────────────► PlayModel.data
   Frame render reads ──────────────────────► PlayView ◄── PlayModel
```

---

## Supporting Systems

### GameEngine & IGameState

`GameEngine` is the application shell. It owns the SDL window, all subsystems, and a single `currentState` pointer. Every frame it calls `handleInput`, `update`, and `render` on the current state.

`IGameState` is a pure virtual base class with four virtual methods: `onEnter`, `onExit`, `handleInput`, `update`, `render`. All game screens — including `PlayState` — implement this interface.

State transitions happen via `engine.changeState(newState)`. The engine defers the actual swap to the top of the next frame to avoid mutating state mid-loop.

### GameState (the shared data struct)

Defined in `include/Core/Types.hpp`. This is a plain-old-data struct — no methods, no encapsulation — that acts as the game's single shared memory. It is owned by `GameEngine` and passed by reference into `PlayModel`. This means the MVC triad and the `GameEngine` see the same live data at all times.

```cpp
struct GameState {
    std::array<std::array<char, GRID_SIZE>, GRID_SIZE> grid{};
    int currentPlayer = 0;
    char currentLetter = '\0';
    int selectedX = -1, selectedY = -1;
    bool isTileSelected = false;
    std::array<int, 2> scores{0, 0};
    std::array<std::string, 2> playerNames{};
    Uint32 gameStartTime = 0;
    Uint32 turnStartTime = 0;
    Uint32 lastWarningTime = 0;
    // ... and more
};
```

### IBoardScorer / BoardScorer

`IBoardScorer` is an abstract interface with a single method:

```cpp
virtual int checkAndScore(GameState& game, int col, int row, char letter) const = 0;
```

`BoardScorer` is the concrete implementation. After every tile placement it scans horizontally and vertically outward from the placed tile, extracts the full word in each direction, validates it against the `Dictionary`, checks it hasn't already been scored, and returns the total points earned (word length + bonus for special words).

Injecting `IBoardScorer` into `PlayModel` (and wiring in `BoardScorer` from `main.cpp`) means the scoring algorithm is swappable without touching any MVC code — a direct application of the Dependency Inversion Principle.

### Network

`Network` wraps SDL_net's TCP socket API. It exposes methods used by the Controller:

- `hostGame(port)` — opens a listening socket, blocks until a client connects
- `joinGame(ip, port)` — connects as client
- `sendMove(x, y, letter)` — packs a `NetworkMove` struct and sends it over TCP
- `receiveMove(outMove)` — non-blocking poll; returns `true` and populates `outMove` if a packet arrived

The Controller calls `processNetwork()` every frame. Since `receiveMove` is non-blocking it returns immediately if nothing is waiting, adding negligible overhead.

### Audio & Renderer

`Audio` wraps SDL2_mixer. It provides `playSound(Mix_Chunk*)` and `playMusic(Mix_Music*)`. Sound effects are triggered by the Controller (because they are responses to game events); music is managed by `SplashState` and stopped by `PlayState::onEnter`.

`Renderer` wraps `SDL_Renderer*`. It provides stateless drawing primitives: `drawText`, `drawTextCentered`, `drawRect`, `drawLine`. `PlayView` holds a reference to `Renderer` and calls these directly.

### UI — Button, Label, IDrawable, IInteractable

`IDrawable` defines `virtual void render(Renderer&) const`. `IInteractable` defines `virtual bool handleInput(const SDL_Event&)`. `Button` inherits both; `Label` inherits only `IDrawable`.

`Button::handleInput` returns `true` when clicked (it consumes the event). The Controller checks this for the Give Up button; other states check it inline in their own `handleInput` methods.

`PlayView` owns the `giveUpBtn` shared pointer. The Controller accesses it via `view->getGiveUpButton()` and checks its click result inside `processInput`. This keeps button ownership in the View while allowing the Controller to react to interaction outcomes.

---

## Class Responsibility Table

| Class | Layer | Creates UI | Reads GameState | Writes GameState | Sends Network | Plays Sound | Renders |
|---|---|---|---|---|---|---|---|
| `PlayModel` | Model | ✗ | ✓ | ✓ (timers only) | ✗ | ✗ | ✗ |
| `PlayView` | View | ✓ | ✓ (via model) | ✗ | ✗ | ✗ | ✓ |
| `PlayController` | Controller | ✗ | ✓ (via model) | ✓ | ✓ | ✓ | ✗ |
| `PlayState` | Orchestrator | ✗ | ✗ (delegates) | ✗ (delegates) | ✗ | ✓ (win sound) | ✗ (delegates) |
| `BoardScorer` | Domain | ✗ | ✓ | ✓ (scores/words) | ✗ | ✗ | ✗ |
| `GameEngine` | Shell | ✗ | ✓ | ✗ | ✗ | ✗ | ✓ (clear/present) |

---

## Key Design Decisions

**1. MVC only for `PlayState`, not all states.**
Simple screens (Splash, GameOver) have almost no logic. Applying MVC to them would add three files and zero clarity. MVC was introduced only where the complexity justified it.

**2. `GameState` is a shared struct, not encapsulated inside `PlayModel`.**
`GameState` is owned by `GameEngine` and pre-populated by earlier states (player names, network connection). `PlayModel` holds a reference to it rather than a copy so the handover is zero-cost and the game-over screen can still read scores without any serialization step.

**3. The View gets a Model pointer, not a copy of the data.**
This means the View is always reading live data. There is no observer/notification system needed. The trade-off is that the View and Model are tightly coupled — acceptable here because they are always created together by the same `PlayState`.

**4. `IBoardScorer` is injected into `PlayModel`.**
Scoring is domain logic, not presentation. Injecting the interface means the scoring algorithm can be tested, swapped, or mocked independently of the MVC shell. The concrete `BoardScorer` is wired in from `main.cpp` via `GameEngine`.

**5. `PlayController` triggers audio.**
In strict MVC, sound would be a side-effect notified through the model. Here, the Controller directly calls `audio.playSound()` at the moment of game events. This is a pragmatic simplification: SDL audio is stateless and the Controller already has the reference; the alternative would be a notification queue for minimal gain.

**6. No observer/event bus.**
SDL's event loop already provides a frame-synchronous event queue. Adding a separate observer pattern on top would replicate SDL's own infrastructure. The current approach — Controller polls SDL events, Controller polls network, View reads model every frame — is idiomatic for SDL game architecture.

---

## Building the Project

**Prerequisites:** MinGW-w64 (Windows) or GCC/Clang (Linux/macOS), SDL2, SDL2_ttf, SDL2_mixer, SDL2_net.

```bash
# Clone this branch
git clone --branch mvc-implementation https://github.com/your-repo/words-collide.git
cd words-collide

# Build (Makefile uses g++ with SDL2 flags)
make

# Run
./WORDS_COLLIDE.exe        # Windows
./WORDS_COLLIDE            # Linux / macOS
```

The `Makefile` compiles every `.cpp` under `src/` (including `src/MVC/`) and links against the SDL2 family. Include paths are set to `include/` so headers are always referenced from the project root, e.g. `#include "MVC/PlayModel.hpp"`.

Assets (dictionary, fonts, sounds) must remain in `assets/` relative to the executable.