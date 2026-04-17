#pragma once

#include <pebble.h>
#include "game_engine.h"

/**
 * Score Manager
 * 
 * Handles score persistence and high score tracking. Maintains top 5 scores
 * per game mode using Pebble's persistent storage API. Stores lock counts
 * with timestamps for each score entry.
 */

// Forward declaration
typedef struct GameEngine GameEngine;

// Persistent storage keys for each mode
#define PERSIST_KEY_CLASSIC_SCORES  1
#define PERSIST_KEY_TIMED_SCORES    2
#define PERSIST_KEY_ENDLESS_SCORES  3
#define PERSIST_KEY_SPEED_SCORES    4

// Score entry with lock count and timestamp
typedef struct {
    uint16_t lock_count;
    time_t timestamp;
} ScoreEntry;

// Top 5 scores for a single mode
typedef struct {
    ScoreEntry scores[5];
} ModeScores;

// Score manager maintaining scores for all modes
typedef struct {
    ModeScores classic;
    ModeScores timed;
    ModeScores endless;
    ModeScores speed;
} ScoreManager;

// Initialization and lifecycle
void score_manager_init(ScoreManager *manager);
void score_manager_load(ScoreManager *manager);
void score_manager_save(ScoreManager *manager);

// Score management
bool score_manager_is_high_score(const ScoreManager *manager, GameMode mode, uint16_t lock_count);
void score_manager_add_score(ScoreManager *manager, GameMode mode, uint16_t lock_count);
void score_manager_get_scores(const ScoreManager *manager, GameMode mode, ScoreEntry *out_scores);
