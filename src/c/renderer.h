#pragma once

#include <pebble.h>
#include "game_engine.h"

/**
 * Renderer
 * 
 * Handles all graphics rendering including the circular lock, rotating
 * indicator, target zone, and text overlays. Optimizes redraws to only
 * update when state changes. Supports platform-specific scaling for
 * different display sizes.
 */

// Rendering configuration for colors and dimensions
typedef struct {
    GPoint center;              // Circle center point
    uint16_t radius;            // Lock circle radius
    uint16_t indicator_length;  // Indicator line length
    GColor color_background;
    GColor color_lock;
    GColor color_target;
    GColor color_indicator;
    GColor color_text;
} RenderConfig;

// Main renderer state
typedef struct {
    Layer *game_layer;          // Custom layer for game graphics
    TextLayer *count_layer;     // Lock count display
    TextLayer *timer_layer;     // Timer display (timed mode)
    RenderConfig config;
    const GameEngine *engine;   // Read-only reference to game state
} Renderer;

// Initialization and lifecycle
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

// Utility functions
GPoint calculate_point_on_circle(GPoint center, uint16_t radius, int16_t angle);
