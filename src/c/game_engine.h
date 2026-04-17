#pragma once

#include <pebble.h>

/**
 * Game Engine
 * 
 * Manages the core game state machine, lock generation and validation,
 * indicator rotation timing, and mode-specific game logic. Coordinates
 * with the renderer and input handler to implement the game loop.
 */

// Game state machine states
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER,
    GAME_STATE_WIN
} GameState;

// Game modes with different difficulty curves and win conditions
typedef enum {
    MODE_CLASSIC,    // 50 locks, increasing speed
    MODE_TIMED,      // 60 seconds, constant speed
    MODE_ENDLESS,    // Infinite locks, increasing speed
    MODE_SPEED       // 30 locks, fast constant speed
} GameMode;

// Lock data structure representing the rotating indicator and target zone
typedef struct {
    int16_t angle;              // Current indicator angle in degrees (0-360)
    int16_t target_angle;       // Target zone center angle
    int16_t target_width;       // Target zone width in degrees (15-30)
    uint16_t rotation_period_ms; // Time for full 360° rotation
    uint32_t last_update_ms;    // Timestamp of last angle update
} Lock;

// Main game engine state
typedef struct {
    GameState state;
    GameMode mode;
    Lock current_lock;
    uint16_t lock_count;
    uint16_t time_remaining_sec; // For timed mode
    AppTimer *animation_timer;
    AppTimer *countdown_timer;   // For timed mode
    bool input_blocked;          // During state transitions
} GameEngine;

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

// Mode-specific configuration
uint16_t get_initial_rotation_period(GameMode mode);
uint16_t get_minimum_rotation_period(GameMode mode);
uint16_t get_speed_decrease_per_lock(GameMode mode);
uint16_t get_win_condition_locks(GameMode mode);
