# Design Document: Pop the Lock Game

## Overview

The "Pop the Lock" game is a timing-based puzzle game for Pebble smartwatches where players must stop a rotating indicator at a precise target position. The game features four distinct modes (Classic, Timed Challenge, Endless, Speed) with varying difficulty curves and win conditions.

The application is built using the Pebble SDK in C, targeting color-capable rectangular display platforms (emery, flint, gabbro). The architecture follows Pebble's design guidelines with glance-first design, one-click interactions, and standard UI components (MenuLayer, TextLayer).

### Key Design Principles

1. **Real-time responsiveness**: 30 FPS rendering with <50ms input latency
2. **Battery efficiency**: Timer-based updates only when needed, pause when inactive
3. **Platform consistency**: Use Pebble SDK APIs (MenuLayer, AppTimer, Vibe) throughout
4. **Memory safety**: Explicit lifecycle management for all allocated resources
5. **Glanceable UI**: Clear visual hierarchy with high-contrast colors

## Architecture

### High-Level Component Structure

```
┌─────────────────────────────────────────────────────────┐
│                     Application                          │
│  ┌────────────┐  ┌──────────────┐  ┌─────────────────┐ │
│  │   Menu     │  │  Game Engine │  │  Score Manager  │ │
│  │  System    │  │              │  │                 │ │
│  └────────────┘  └──────────────┘  └─────────────────┘ │
│         │               │                    │          │
│         └───────────────┼────────────────────┘          │
│                         │                               │
│         ┌───────────────┴────────────────┐              │
│         │                                │              │
│  ┌──────▼──────┐              ┌─────────▼────────┐     │
│  │  Renderer   │              │  Input Handler   │     │
│  └─────────────┘              └──────────────────┘     │
│         │                                │              │
└─────────┼────────────────────────────────┼──────────────┘
          │                                │
    ┌─────▼────────┐              ┌────────▼────────┐
    │ Pebble       │              │ Pebble Button   │
    │ Graphics API │              │ Click Handlers  │
    └──────────────┘              └─────────────────┘
```

### Component Responsibilities

**Menu System**
- Manages MenuLayer for mode selection
- Displays high scores and about screens using TextLayer
- Handles navigation between screens
- Transitions to game engine when mode selected

**Game Engine**
- Maintains game state machine (MENU, PLAYING, PAUSED, GAME_OVER, WIN)
- Manages lock generation and validation
- Controls indicator rotation via AppTimer
- Implements mode-specific logic (speed curves, win conditions)
- Coordinates with renderer and input handler

**Renderer**
- Draws circular lock, indicator, and target zone
- Renders lock count and timer displays
- Executes success/failure animations
- Optimizes redraws (only when state changes)

**Input Handler**
- Processes button clicks with <50ms latency
- Routes input based on current state
- Implements pause/resume functionality
- Handles long-press for menu exit

**Score Manager**
- Persists top 5 scores per mode using persist_write_data
- Loads scores on demand
- Compares and updates high score lists
- Stores timestamps with scores

## Components and Interfaces

### Game Engine

#### Data Structures

```c
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER,
    GAME_STATE_WIN
} GameState;

typedef enum {
    MODE_CLASSIC,
    MODE_TIMED,
    MODE_ENDLESS,
    MODE_SPEED
} GameMode;

typedef struct {
    int16_t angle;           // Current angle in degrees (0-360)
    int16_t target_angle;    // Target zone center angle
    int16_t target_width;    // Target zone width in degrees (15-30)
    uint16_t rotation_period_ms; // Time for full rotation
    uint32_t last_update_ms; // Timestamp of last update
} Lock;

typedef struct {
    GameState state;
    GameMode mode;
    Lock current_lock;
    uint16_t lock_count;
    uint16_t time_remaining_sec; // For timed mode
    AppTimer *animation_timer;
    AppTimer *countdown_timer;   // For timed mode
    bool input_blocked;          // During transitions
} GameEngine;
```

#### Key Functions

```c
// Initialization and lifecycle
void game_engine_init(GameEngine *engine, GameMode mode);
void game_engine_deinit(GameEngine *engine);

// State management
void game_engine_set_state(GameEngine *engine, GameState new_state);
void game_engine_pause(GameEngine *engine);
void game_engine_resume(GameEngine *engine);

// Lock management
void lock_generate(Lock *lock, GameMode mode, uint16_t lock_count);
bool lock_check_hit(const Lock *lock, int16_t stopped_angle);
void lock_advance_difficulty(Lock *lock, GameMode mode, uint16_t lock_count);

// Game loop
void game_engine_update(void *context); // AppTimer callback
void game_engine_handle_select(GameEngine *engine);

// Mode-specific logic
uint16_t get_initial_rotation_period(GameMode mode);
uint16_t get_minimum_rotation_period(GameMode mode);
uint16_t get_speed_decrease_per_lock(GameMode mode);
uint16_t get_win_condition_locks(GameMode mode);
```

#### State Machine

```
MENU ──select──> PLAYING ──game_over──> GAME_OVER ──back──> MENU
                    │                         │
                    │                         └──select──> MENU
                    │
                 pause/back
                    │
                    ▼
                 PAUSED ──resume──> PLAYING
                    │
                  back (long)
                    │
                    ▼
                  MENU

PLAYING ──win_condition──> WIN ──back/select──> MENU
```

### Renderer

#### Data Structures

```c
typedef struct {
    GPoint center;           // Circle center point
    uint16_t radius;         // Lock circle radius
    uint16_t indicator_length; // Indicator line length
    GColor color_background;
    GColor color_lock;
    GColor color_target;
    GColor color_indicator;
    GColor color_text;
} RenderConfig;

typedef struct {
    Layer *game_layer;       // Custom layer for game graphics
    TextLayer *count_layer;  // Lock count display
    TextLayer *timer_layer;  // Timer display (timed mode)
    RenderConfig config;
    const GameEngine *engine; // Read-only reference
} Renderer;
```

#### Key Functions

```c
// Initialization
void renderer_init(Renderer *renderer, Window *window, const GameEngine *engine);
void renderer_deinit(Renderer *renderer);
void renderer_configure_for_platform(Renderer *renderer, GRect bounds);

// Drawing callbacks
void game_layer_update_proc(Layer *layer, GContext *ctx);
void draw_lock_circle(GContext *ctx, const RenderConfig *config);
void draw_target_zone(GContext *ctx, const RenderConfig *config, const Lock *lock);
void draw_indicator(GContext *ctx, const RenderConfig *config, const Lock *lock);
void draw_lock_count(Renderer *renderer, uint16_t count);
void draw_timer(Renderer *renderer, uint16_t seconds);

// Animations
void renderer_animate_success(Renderer *renderer);
void renderer_animate_failure(Renderer *renderer);

// Utility
GPoint calculate_point_on_circle(GPoint center, uint16_t radius, int16_t angle);
```

#### Rendering Pipeline

1. **Layer Update Callback**: `game_layer_update_proc` called when layer marked dirty
2. **Background Clear**: Fill with background color
3. **Lock Circle**: Draw outer circle outline
4. **Target Zone**: Draw colored arc segment at target angle
5. **Indicator**: Draw line from center to perimeter at current angle
6. **Text Overlays**: Update TextLayers for count/timer

### Input Handler

#### Data Structures

```c
typedef struct {
    GameEngine *engine;
    Renderer *renderer;
    Window *game_window;
    uint32_t last_select_time_ms; // For debouncing
} InputHandler;
```

#### Key Functions

```c
void input_handler_init(InputHandler *handler, GameEngine *engine, Renderer *renderer);
void input_handler_register(InputHandler *handler, Window *window);

// Button callbacks
void select_click_handler(ClickRecognizerRef recognizer, void *context);
void back_click_handler(ClickRecognizerRef recognizer, void *context);
void back_long_click_handler(ClickRecognizerRef recognizer, void *context);

// Click config provider
void game_click_config_provider(void *context);
```

### Menu System

#### Data Structures

```c
typedef struct {
    Window *menu_window;
    MenuLayer *menu_layer;
    Window *scores_window;
    TextLayer *scores_text_layer;
    Window *about_window;
    TextLayer *about_text_layer;
} MenuSystem;

typedef enum {
    MENU_ITEM_CLASSIC,
    MENU_ITEM_TIMED,
    MENU_ITEM_ENDLESS,
    MENU_ITEM_SPEED,
    MENU_ITEM_HIGH_SCORES,
    MENU_ITEM_ABOUT,
    MENU_ITEM_COUNT
} MenuItem;
```

#### Menu Icons

The menu system uses Pebble's built-in system icons to provide visual cues for each menu item:

```c
// Icon mapping for menu items
static const uint32_t MENU_ICONS[] = {
    [MENU_ITEM_CLASSIC] = RESOURCE_ID_GENERIC_CONFIRMATION,    // Classic mode - checkmark
    [MENU_ITEM_TIMED] = RESOURCE_ID_GENERIC_CLOCK,             // Timed Challenge - clock
    [MENU_ITEM_ENDLESS] = RESOURCE_ID_GENERIC_REFRESH,         // Endless mode - refresh/loop
    [MENU_ITEM_SPEED] = RESOURCE_ID_GENERIC_FAST_FORWARD,      // Speed mode - fast forward
    [MENU_ITEM_HIGH_SCORES] = RESOURCE_ID_GENERIC_TROPHY,      // High Scores - trophy
    [MENU_ITEM_ABOUT] = RESOURCE_ID_GENERIC_QUESTION_MARK      // About - question mark
};
```

**Icon Rationale:**
- **Classic Mode** (`RESOURCE_ID_GENERIC_CONFIRMATION`): Checkmark represents completing the standard 50-lock challenge
- **Timed Challenge** (`RESOURCE_ID_GENERIC_CLOCK`): Clock icon clearly indicates time-based gameplay
- **Endless Mode** (`RESOURCE_ID_GENERIC_REFRESH`): Refresh/loop icon suggests continuous, repeating gameplay
- **Speed Mode** (`RESOURCE_ID_GENERIC_FAST_FORWARD`): Fast forward icon communicates rapid gameplay
- **High Scores** (`RESOURCE_ID_GENERIC_TROPHY`): Trophy is the universal symbol for achievements and scores
- **About** (`RESOURCE_ID_GENERIC_QUESTION_MARK`): Question mark is standard for help/information screens

These icons are loaded using `gbitmap_create_with_resource()` and set via `menu_cell_basic_draw()` in the menu draw callback.

#### Key Functions

```c
void menu_system_init(MenuSystem *menu);
void menu_system_deinit(MenuSystem *menu);
void menu_system_show(MenuSystem *menu);

// MenuLayer callbacks
uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context);
void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context);
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context);

// Icon management
GBitmap* get_menu_icon(MenuItem item);
void load_menu_icons(void);
void unload_menu_icons(void);

// Screen transitions
void show_game_screen(GameMode mode);
void show_high_scores_screen(void);
void show_about_screen(void);
```

### Score Manager

#### Data Structures

```c
typedef struct {
    uint16_t lock_count;
    time_t timestamp;
} ScoreEntry;

typedef struct {
    ScoreEntry scores[5]; // Top 5 scores
} ModeScores;

typedef struct {
    ModeScores classic;
    ModeScores timed;
    ModeScores endless;
    ModeScores speed;
} ScoreManager;
```

#### Persistent Storage Keys

```c
#define PERSIST_KEY_CLASSIC_SCORES  1
#define PERSIST_KEY_TIMED_SCORES    2
#define PERSIST_KEY_ENDLESS_SCORES  3
#define PERSIST_KEY_SPEED_SCORES    4
```

#### Key Functions

```c
void score_manager_init(ScoreManager *manager);
void score_manager_load(ScoreManager *manager);
void score_manager_save(ScoreManager *manager);

bool score_manager_is_high_score(const ScoreManager *manager, GameMode mode, uint16_t lock_count);
void score_manager_add_score(ScoreManager *manager, GameMode mode, uint16_t lock_count);
void score_manager_get_scores(const ScoreManager *manager, GameMode mode, ScoreEntry *out_scores);
```

## Data Models

### Lock Generation Algorithm

```c
void lock_generate(Lock *lock, GameMode mode, uint16_t lock_count) {
    // Randomize target angle (0-360 degrees)
    lock->target_angle = rand() % 360;
    
    // Randomize target width (15-30 degrees)
    lock->target_width = 15 + (rand() % 16);
    
    // Set rotation period based on mode and difficulty
    lock->rotation_period_ms = get_initial_rotation_period(mode);
    lock->rotation_period_ms -= (lock_count * get_speed_decrease_per_lock(mode));
    
    // Enforce minimum period
    uint16_t min_period = get_minimum_rotation_period(mode);
    if (lock->rotation_period_ms < min_period) {
        lock->rotation_period_ms = min_period;
    }
    
    // Initialize angle and timestamp
    lock->angle = 0;
    lock->last_update_ms = 0;
}
```

### Hit Detection Algorithm

```c
bool lock_check_hit(const Lock *lock, int16_t stopped_angle) {
    // Calculate angular distance to target center
    int16_t distance = abs(stopped_angle - lock->target_angle);
    
    // Handle wraparound (e.g., 355° to 5° is 10° apart, not 350°)
    if (distance > 180) {
        distance = 360 - distance;
    }
    
    // Check if within half the target width
    int16_t tolerance = lock->target_width / 2;
    return distance <= tolerance;
}
```

### Indicator Rotation Update

```c
void update_indicator_angle(Lock *lock) {
    uint32_t current_time_ms = time_ms(NULL, NULL);
    
    if (lock->last_update_ms == 0) {
        lock->last_update_ms = current_time_ms;
        return;
    }
    
    // Calculate elapsed time
    uint32_t elapsed_ms = current_time_ms - lock->last_update_ms;
    
    // Calculate angular velocity (degrees per millisecond)
    float angular_velocity = 360.0f / lock->rotation_period_ms;
    
    // Update angle
    float angle_delta = angular_velocity * elapsed_ms;
    lock->angle = (lock->angle + (int16_t)angle_delta) % 360;
    
    lock->last_update_ms = current_time_ms;
}
```

### Mode Configuration Table

```c
typedef struct {
    uint16_t initial_period_ms;
    uint16_t min_period_ms;
    uint16_t speed_decrease_ms;
    uint16_t win_locks;
    bool has_timer;
    uint16_t timer_seconds;
} ModeConfig;

static const ModeConfig MODE_CONFIGS[] = {
    [MODE_CLASSIC] = {
        .initial_period_ms = 2000,
        .min_period_ms = 500,
        .speed_decrease_ms = 50,
        .win_locks = 50,
        .has_timer = false,
        .timer_seconds = 0
    },
    [MODE_TIMED] = {
        .initial_period_ms = 1500,
        .min_period_ms = 1500,
        .speed_decrease_ms = 0,
        .win_locks = 0,
        .has_timer = true,
        .timer_seconds = 60
    },
    [MODE_ENDLESS] = {
        .initial_period_ms = 2000,
        .min_period_ms = 400,
        .speed_decrease_ms = 30,
        .win_locks = 0,
        .has_timer = false,
        .timer_seconds = 0
    },
    [MODE_SPEED] = {
        .initial_period_ms = 800,
        .min_period_ms = 800,
        .speed_decrease_ms = 0,
        .win_locks = 30,
        .has_timer = false,
        .timer_seconds = 0
    }
};
```

## Testing Strategy

This game involves real-time graphics, timing mechanics, and platform-specific APIs. The testing strategy focuses on:

### Unit Tests

**Lock Generation and Validation**
- Test `lock_generate` produces angles in 0-360 range
- Test `lock_generate` produces target widths in 15-30 range
- Test `lock_check_hit` correctly identifies hits within tolerance
- Test `lock_check_hit` correctly identifies misses outside tolerance
- Test `lock_check_hit` handles wraparound cases (e.g., 355° target, 5° indicator)

**Angle Calculations**
- Test `calculate_point_on_circle` for cardinal angles (0°, 90°, 180°, 270°)
- Test angle wraparound in `update_indicator_angle`
- Test angular distance calculation handles all quadrants

**Mode Configuration**
- Test each mode returns correct initial rotation period
- Test each mode returns correct minimum rotation period
- Test each mode returns correct speed decrease value
- Test each mode returns correct win condition

**State Transitions**
- Test valid state transitions (MENU→PLAYING, PLAYING→PAUSED, etc.)
- Test invalid transitions are prevented
- Test input blocking during transitions

**Score Management**
- Test score insertion maintains sorted order
- Test score list caps at 5 entries
- Test scores below top 5 are rejected
- Test persistence key mapping for each mode

### Integration Tests

**Game Flow**
- Test complete game session: menu → play → game over → menu
- Test pause/resume preserves game state
- Test win condition triggers correctly for each mode
- Test timer countdown in timed mode

**Input Handling**
- Test SELECT button stops indicator
- Test BACK button pauses game
- Test long BACK press returns to menu
- Test input debouncing prevents double-clicks

**Rendering**
- Test layer initialization and cleanup
- Test text layer updates reflect game state
- Test platform-specific scaling for different display sizes

### Manual Testing

**Visual Verification**
- Verify smooth 30 FPS animation
- Verify target zone is visually distinct
- Verify high contrast on color displays
- Verify text is readable (glance-first principle)

**Timing Verification**
- Verify rotation periods match specifications
- Verify input latency <50ms
- Verify animations last 200-500ms

**Platform Testing**
- Test on emery, flint, and gabbro emulators
- Verify color scheme on all platforms
- Verify proper scaling on different display sizes

**Battery Testing**
- Verify timers stop when paused
- Verify memory is released on exit
- Monitor battery drain during extended play

## Error Handling

### Memory Allocation Failures

```c
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Memory allocation failed: %zu bytes", size);
        // Show error dialog
        show_error_dialog("Memory Error", "Unable to allocate memory");
        // Return to menu
        window_stack_pop_all(false);
        menu_system_show(&g_menu_system);
    }
    return ptr;
}
```

### Persistent Storage Failures

```c
void score_manager_load(ScoreManager *manager) {
    int result = persist_read_data(PERSIST_KEY_CLASSIC_SCORES, 
                                    &manager->classic, 
                                    sizeof(ModeScores));
    if (result < 0) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Failed to load classic scores, using defaults");
        memset(&manager->classic, 0, sizeof(ModeScores));
    }
    // Repeat for other modes...
}

void score_manager_save(ScoreManager *manager) {
    int result = persist_write_data(PERSIST_KEY_CLASSIC_SCORES, 
                                     &manager->classic, 
                                     sizeof(ModeScores));
    if (result < 0) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to save classic scores: %d", result);
        // Continue execution - don't crash on save failure
    }
    // Repeat for other modes...
}
```

### Timer Callback Delays

```c
void game_engine_update(void *context) {
    GameEngine *engine = (GameEngine *)context;
    
    uint32_t current_time = time_ms(NULL, NULL);
    uint32_t expected_time = engine->current_lock.last_update_ms + FRAME_INTERVAL_MS;
    
    // If callback was delayed, adjust angle based on actual elapsed time
    if (current_time > expected_time + 10) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer delayed by %lu ms", 
                current_time - expected_time);
        // update_indicator_angle uses actual elapsed time, so it self-corrects
    }
    
    update_indicator_angle(&engine->current_lock);
    layer_mark_dirty(g_renderer.game_layer);
    
    // Schedule next update
    engine->animation_timer = app_timer_register(FRAME_INTERVAL_MS, 
                                                  game_engine_update, 
                                                  engine);
}
```

### Invalid Angle Clamping

```c
int16_t normalize_angle(int16_t angle) {
    // Handle negative angles
    while (angle < 0) {
        angle += 360;
    }
    // Handle angles >= 360
    angle = angle % 360;
    return angle;
}
```

### State Transition Guards

```c
void game_engine_set_state(GameEngine *engine, GameState new_state) {
    // Validate transition
    if (!is_valid_transition(engine->state, new_state)) {
        APP_LOG(APP_LOG_LEVEL_WARNING, 
                "Invalid state transition: %d -> %d", 
                engine->state, new_state);
        return;
    }
    
    // Block input during transition
    engine->input_blocked = true;
    
    // Perform transition
    GameState old_state = engine->state;
    engine->state = new_state;
    
    // State-specific cleanup/initialization
    on_state_exit(engine, old_state);
    on_state_enter(engine, new_state);
    
    // Unblock input after 100ms
    app_timer_register(100, unblock_input_callback, engine);
}
```

## Performance and Memory Optimization

### Frame Rate Management

```c
#define TARGET_FPS 30
#define FRAME_INTERVAL_MS (1000 / TARGET_FPS)

void game_engine_update(void *context) {
    uint32_t frame_start = time_ms(NULL, NULL);
    
    // Update game logic
    update_indicator_angle(&engine->current_lock);
    
    // Mark layer dirty (actual rendering happens in update_proc)
    layer_mark_dirty(g_renderer.game_layer);
    
    uint32_t frame_time = time_ms(NULL, NULL) - frame_start;
    if (frame_time > FRAME_INTERVAL_MS) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Frame took %lu ms (target: %d ms)", 
                frame_time, FRAME_INTERVAL_MS);
    }
    
    // Schedule next frame
    engine->animation_timer = app_timer_register(FRAME_INTERVAL_MS, 
                                                  game_engine_update, 
                                                  context);
}
```

### Selective Redrawing

```c
void game_layer_update_proc(Layer *layer, GContext *ctx) {
    // Only redraw if game is playing
    if (g_engine.state != GAME_STATE_PLAYING) {
        return;
    }
    
    // Draw static elements (could be cached in future optimization)
    draw_lock_circle(ctx, &g_renderer.config);
    draw_target_zone(ctx, &g_renderer.config, &g_engine.current_lock);
    
    // Draw dynamic element
    draw_indicator(ctx, &g_renderer.config, &g_engine.current_lock);
}
```

### Memory Lifecycle

```c
// Global state (allocated once at app start)
static GameEngine g_engine;
static Renderer g_renderer;
static MenuSystem g_menu_system;
static ScoreManager g_score_manager;
static InputHandler g_input_handler;

void init(void) {
    // Initialize managers
    score_manager_init(&g_score_manager);
    score_manager_load(&g_score_manager);
    
    // Initialize menu system
    menu_system_init(&g_menu_system);
    menu_system_show(&g_menu_system);
}

void deinit(void) {
    // Save scores before exit
    score_manager_save(&g_score_manager);
    
    // Clean up game engine if active
    if (g_engine.state != GAME_STATE_MENU) {
        game_engine_deinit(&g_engine);
    }
    
    // Clean up renderer
    renderer_deinit(&g_renderer);
    
    // Clean up menu system
    menu_system_deinit(&g_menu_system);
}
```

### Timer Management

```c
void game_engine_pause(GameEngine *engine) {
    // Cancel animation timer
    if (engine->animation_timer != NULL) {
        app_timer_cancel(engine->animation_timer);
        engine->animation_timer = NULL;
    }
    
    // Cancel countdown timer (timed mode)
    if (engine->countdown_timer != NULL) {
        app_timer_cancel(engine->countdown_timer);
        engine->countdown_timer = NULL;
    }
    
    engine->state = GAME_STATE_PAUSED;
}

void game_engine_resume(GameEngine *engine) {
    // Reset last update time to prevent jump
    engine->current_lock.last_update_ms = time_ms(NULL, NULL);
    
    // Restart animation timer
    engine->animation_timer = app_timer_register(FRAME_INTERVAL_MS, 
                                                  game_engine_update, 
                                                  engine);
    
    // Restart countdown timer if timed mode
    if (engine->mode == MODE_TIMED) {
        engine->countdown_timer = app_timer_register(1000, 
                                                      countdown_timer_callback, 
                                                      engine);
    }
    
    engine->state = GAME_STATE_PLAYING;
}
```

## Platform-Specific Considerations

### Display Scaling

```c
void renderer_configure_for_platform(Renderer *renderer, GRect bounds) {
    // Calculate center point
    renderer->config.center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    
    // Calculate radius (80% of smaller dimension)
    uint16_t min_dimension = bounds.size.w < bounds.size.h ? 
                             bounds.size.w : bounds.size.h;
    renderer->config.radius = (min_dimension * 80) / 100 / 2;
    
    // Indicator length is 90% of radius
    renderer->config.indicator_length = (renderer->config.radius * 90) / 100;
    
    APP_LOG(APP_LOG_LEVEL_INFO, 
            "Display: %dx%d, Center: (%d,%d), Radius: %d",
            bounds.size.w, bounds.size.h,
            renderer->config.center.x, renderer->config.center.y,
            renderer->config.radius);
}
```

### Color Configuration

```c
void renderer_init_colors(Renderer *renderer) {
    // Original game color scheme optimized for color displays
    renderer->config.color_background = GColorBlack;
    renderer->config.color_lock = GColorWhite;
    renderer->config.color_target = GColorGreen;
    renderer->config.color_indicator = GColorCyan;
    renderer->config.color_text = GColorWhite;
}
```

### Vibration Patterns

```c
void vibrate_success(void) {
    // Short single pulse
    vibes_short_pulse();
}

void vibrate_failure(void) {
    // Double pulse pattern
    static const uint32_t segments[] = {100, 100, 100};
    VibePattern pattern = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments)
    };
    vibes_enqueue_custom_pattern(pattern);
}

void vibrate_victory(void) {
    // Triple pulse pattern
    static const uint32_t segments[] = {50, 100, 50, 100, 50};
    VibePattern pattern = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments)
    };
    vibes_enqueue_custom_pattern(pattern);
}
```

## Implementation Roadmap

### Phase 1: Core Infrastructure
1. Set up project structure and build configuration
2. Implement game engine state machine
3. Implement lock data structure and generation
4. Implement basic renderer with circular lock

### Phase 2: Game Mechanics
1. Implement indicator rotation with AppTimer
2. Implement hit detection algorithm
3. Implement SELECT button handling
4. Implement game over and win conditions

### Phase 3: Visual Polish
1. Implement target zone rendering
2. Implement lock count display
3. Implement success/failure animations
4. Implement vibration feedback

### Phase 4: Menu System
1. Implement MenuLayer-based main menu
2. Implement mode selection
3. Implement high scores screen
4. Implement about screen

### Phase 5: Game Modes
1. Implement Classic mode logic
2. Implement Timed Challenge mode with countdown
3. Implement Endless mode
4. Implement Speed mode

### Phase 6: Score Persistence
1. Implement score manager
2. Implement persistent storage
3. Implement high score comparison
4. Implement score display formatting

### Phase 7: Testing and Optimization
1. Test on all target platforms (emery, flint, gabbro)
2. Optimize frame rate and battery usage
3. Test edge cases and error conditions
4. Final polish and bug fixes

## Open Questions and Future Enhancements

### Open Questions
1. Should the target zone width decrease as difficulty increases?
2. Should there be a practice mode with no game over?
3. Should scores include completion time in addition to lock count?

### Future Enhancements
1. **Achievements**: Unlock badges for milestones (100 locks, sub-30s Classic, etc.)
2. **Daily Challenge**: Seeded random mode with global leaderboard
3. **Customization**: Allow players to choose color schemes
4. **Sound Effects**: Add optional sound effects (if platform supports)
5. **Multiplayer**: Pass-and-play mode with alternating turns
6. **Statistics**: Track total locks unlocked, games played, success rate
7. **Difficulty Settings**: Easy/Normal/Hard presets for each mode
8. **Tutorial**: Interactive first-time user experience

