#pragma once

#include <pebble.h>
#include "game_engine.h"
#include "renderer.h"

/**
 * Input Handler
 * 
 * Processes button input with <50ms latency and routes events based on
 * current game state. Handles SELECT for stopping the indicator, BACK
 * for pause/resume, and long BACK press for returning to menu.
 */

// Forward declarations
typedef struct GameEngine GameEngine;
typedef struct Renderer Renderer;

// Input handler state
typedef struct {
    GameEngine *engine;
    Renderer *renderer;
    Window *game_window;
    uint32_t last_select_time_ms; // For debouncing
} InputHandler;

// Initialization
void input_handler_init(InputHandler *handler, GameEngine *engine, Renderer *renderer);
void input_handler_register(InputHandler *handler, Window *window);

// Button callbacks
void select_click_handler(ClickRecognizerRef recognizer, void *context);
void back_click_handler(ClickRecognizerRef recognizer, void *context);
void back_long_click_handler(ClickRecognizerRef recognizer, void *context);

// Click config provider
void game_click_config_provider(void *context);
