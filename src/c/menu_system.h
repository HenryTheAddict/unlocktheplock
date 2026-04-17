#pragma once

#include <pebble.h>
#include "game_engine.h"

/**
 * Menu System
 * 
 * Manages the main menu using MenuLayer for mode selection, and displays
 * high scores and about screens using TextLayer. Handles navigation between
 * screens and transitions to the game engine when a mode is selected.
 */

// Forward declaration
typedef struct GameEngine GameEngine;

// Menu items
typedef enum {
    MENU_ITEM_CLASSIC,
    MENU_ITEM_TIMED,
    MENU_ITEM_ENDLESS,
    MENU_ITEM_SPEED,
    MENU_ITEM_HIGH_SCORES,
    MENU_ITEM_ABOUT,
    MENU_ITEM_COUNT
} MenuItem;

// Menu system state
typedef struct {
    Window *menu_window;
    MenuLayer *menu_layer;
    Window *scores_window;
    TextLayer *scores_text_layer;
    Window *about_window;
    TextLayer *about_text_layer;
} MenuSystem;

// Initialization and lifecycle
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
