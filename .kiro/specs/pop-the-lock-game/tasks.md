# Tasks: Pop the Lock Game

## Phase 1: Core Infrastructure

### 1.1 Set up project structure and build configuration
- [x] 1.1.1 Verify package.json has "watchface": false for app mode
- [x] 1.1.2 Create header files for game engine, renderer, input handler, menu system, and score manager
- [x] 1.1.3 Set up build configuration for target platforms (emery, flint, gabbro)
- [x] 1.1.4 Verify project builds successfully with `pebble build`

### 1.2 Implement game engine state machine
- [x] 1.2.1 Define GameState enum (MENU, PLAYING, PAUSED, GAME_OVER, WIN)
- [x] 1.2.2 Define GameMode enum (CLASSIC, TIMED, ENDLESS, SPEED)
- [x] 1.2.3 Implement GameEngine struct with state, mode, and lock data
- [x] 1.2.4 Implement game_engine_init() and game_engine_deinit()
- [x] 1.2.5 Implement game_engine_set_state() with transition validation
- [x] 1.2.6 Implement state transition guards and input blocking

### 1.3 Implement lock data structure and generation
- [x] 1.3.1 Define Lock struct (angle, target_angle, target_width, rotation_period_ms, last_update_ms)
- [x] 1.3.2 Implement lock_generate() with randomized target angle (0-360°)
- [x] 1.3.3 Implement lock_generate() with randomized target width (15-30°)
- [x] 1.3.4 Implement mode configuration table with initial periods, min periods, speed decrease, and win conditions
- [x] 1.3.5 Implement lock_advance_difficulty() for mode-specific speed increases

### 1.4 Implement basic renderer with circular lock
- [~] 1.4.1 Define Renderer struct with layers and render config
- [~] 1.4.2 Implement renderer_init() and renderer_deinit()
- [~] 1.4.3 Implement renderer_configure_for_platform() for display scaling
- [~] 1.4.4 Implement draw_lock_circle() to draw outer circle outline
- [~] 1.4.5 Implement game_layer_update_proc() callback
- [~] 1.4.6 Initialize color scheme (black background, white lock, green target, cyan indicator)

## Phase 2: Game Mechanics

### 2.1 Implement indicator rotation with AppTimer
- [~] 2.1.1 Implement update_indicator_angle() with time-based rotation
- [~] 2.1.2 Implement game_engine_update() as AppTimer callback
- [~] 2.1.3 Set up 30 FPS timer (33ms interval) for smooth animation
- [~] 2.1.4 Implement angle normalization (0-360° wraparound)
- [~] 2.1.5 Handle timer callback delays with elapsed time adjustment

### 2.2 Implement hit detection algorithm
- [~] 2.2.1 Implement lock_check_hit() with angular distance calculation
- [~] 2.2.2 Handle angle wraparound in hit detection (e.g., 355° to 5°)
- [~] 2.2.3 Implement tolerance check (within half of target_width)
- [~] 2.2.4 Add unit tests for hit detection edge cases

### 2.3 Implement SELECT button handling
- [~] 2.3.1 Implement select_click_handler() to stop indicator
- [~] 2.3.2 Call lock_check_hit() on SELECT press
- [~] 2.3.3 Increment lock_count on successful hit
- [~] 2.3.4 Generate new lock on successful hit
- [~] 2.3.5 Trigger game over on miss
- [~] 2.3.6 Implement input debouncing (<50ms latency requirement)

### 2.4 Implement game over and win conditions
- [~] 2.4.1 Implement transition to GAME_OVER state on miss
- [~] 2.4.2 Implement win condition check for Classic mode (50 locks)
- [~] 2.4.3 Implement win condition check for Speed mode (30 locks)
- [~] 2.4.4 Implement transition to WIN state when condition met
- [~] 2.4.5 Stop all timers on game over or win

## Phase 3: Visual Polish

### 3.1 Implement target zone rendering
- [~] 3.1.1 Implement draw_target_zone() to draw colored arc segment
- [~] 3.1.2 Implement calculate_point_on_circle() utility function
- [~] 3.1.3 Draw arc from target_angle - (target_width/2) to target_angle + (target_width/2)
- [~] 3.1.4 Use green color for target zone
- [~] 3.1.5 Ensure target zone is visually distinct (3px line width minimum)

### 3.2 Implement lock count display
- [~] 3.2.1 Create TextLayer for lock count
- [~] 3.2.2 Implement draw_lock_count() to update text
- [~] 3.2.3 Position count display prominently (top of screen)
- [~] 3.2.4 Use 18pt+ font size for glance-first readability
- [~] 3.2.5 Update count display on each successful unlock

### 3.3 Implement success/failure animations
- [~] 3.3.1 Implement renderer_animate_success() with 200-500ms duration
- [~] 3.3.2 Implement renderer_animate_failure() with 200-500ms duration
- [~] 3.3.3 Block input during animations
- [~] 3.3.4 Trigger animations on lock unlock/miss

### 3.4 Implement vibration feedback
- [~] 3.4.1 Implement vibrate_success() with short single pulse
- [~] 3.4.2 Implement vibrate_failure() with double pulse pattern (100ms, 100ms pause, 100ms)
- [~] 3.4.3 Implement vibrate_victory() with triple pulse pattern
- [~] 3.4.4 Call vibration functions on appropriate game events
- [~] 3.4.5 Ensure vibration durations are 50-200ms for battery conservation

## Phase 4: Menu System

### 4.1 Implement MenuLayer-based main menu
- [~] 4.1.1 Define MenuSystem struct with window and MenuLayer
- [~] 4.1.2 Implement menu_system_init() and menu_system_deinit()
- [~] 4.1.3 Create menu window and MenuLayer
- [~] 4.1.4 Implement menu_get_num_rows_callback() returning 6 items
- [~] 4.1.5 Set up MenuLayer callbacks

### 4.2 Implement mode selection
- [~] 4.2.1 Define MenuItem enum (CLASSIC, TIMED, ENDLESS, SPEED, HIGH_SCORES, ABOUT)
- [~] 4.2.2 Implement menu_draw_row_callback() with item titles and icons
- [~] 4.2.3 Load menu icons using gbitmap_create_with_resource()
- [~] 4.2.4 Map icons: Classic=CONFIRMATION, Timed=CLOCK, Endless=REFRESH, Speed=FAST_FORWARD, Scores=TROPHY, About=QUESTION_MARK
- [~] 4.2.5 Implement menu_select_callback() to handle item selection
- [~] 4.2.6 Implement show_game_screen() to transition to game
- [~] 4.2.7 Implement load_menu_icons() and unload_menu_icons() for lifecycle management

### 4.3 Implement high scores screen
- [~] 4.3.1 Create scores window and TextLayer
- [~] 4.3.2 Implement show_high_scores_screen()
- [~] 4.3.3 Format and display top 5 scores for each mode
- [~] 4.3.4 Display score timestamps
- [~] 4.3.5 Handle BACK button to return to menu

### 4.4 Implement about screen
- [~] 4.4.1 Create about window and TextLayer
- [~] 4.4.2 Implement show_about_screen()
- [~] 4.4.3 Display game information and controls
- [~] 4.4.4 Use 14pt+ font for readability
- [~] 4.4.5 Handle BACK button to return to menu

## Phase 5: Game Modes

### 5.1 Implement Classic mode logic
- [~] 5.1.1 Set initial rotation period to 2000ms
- [~] 5.1.2 Decrease period by 50ms per lock
- [~] 5.1.3 Enforce minimum period of 500ms
- [~] 5.1.4 Check for win condition at 50 locks
- [~] 5.1.5 Display completion time on win

### 5.2 Implement Timed Challenge mode with countdown
- [~] 5.2.1 Set constant rotation period to 1500ms
- [~] 5.2.2 Initialize countdown timer at 60 seconds
- [~] 5.2.3 Create TextLayer for timer display
- [~] 5.2.4 Implement draw_timer() to update display
- [~] 5.2.5 Implement countdown_timer_callback() to decrement timer
- [~] 5.2.6 End game when timer reaches zero
- [~] 5.2.7 Display final lock count

### 5.3 Implement Endless mode
- [~] 5.3.1 Set initial rotation period to 2000ms
- [~] 5.3.2 Decrease period by 30ms per lock
- [~] 5.3.3 Enforce minimum period of 400ms
- [~] 5.3.4 Continue indefinitely until game over
- [~] 5.3.5 Display final lock count on game over

### 5.4 Implement Speed mode
- [~] 5.4.1 Set constant rotation period to 800ms
- [~] 5.4.2 Maintain constant speed throughout
- [~] 5.4.3 Check for win condition at 30 locks
- [~] 5.4.4 Display final lock count on game over or win

## Phase 6: Score Persistence

### 6.1 Implement score manager
- [~] 6.1.1 Define ScoreEntry struct (lock_count, timestamp)
- [~] 6.1.2 Define ModeScores struct with array of 5 ScoreEntry
- [~] 6.1.3 Define ScoreManager struct with scores for all modes
- [~] 6.1.4 Implement score_manager_init()
- [~] 6.1.5 Define persistent storage keys (1-4 for each mode)

### 6.2 Implement persistent storage
- [~] 6.2.1 Implement score_manager_load() using persist_read_data()
- [~] 6.2.2 Implement score_manager_save() using persist_write_data()
- [~] 6.2.3 Handle storage read failures (initialize with defaults)
- [~] 6.2.4 Handle storage write failures (log error, continue execution)
- [~] 6.2.5 Call load on app init, save on app deinit

### 6.3 Implement high score comparison
- [~] 6.3.1 Implement score_manager_is_high_score() to check if score qualifies
- [~] 6.3.2 Implement score_manager_add_score() to insert new score
- [~] 6.3.3 Maintain sorted order (highest to lowest)
- [~] 6.3.4 Cap list at 5 entries
- [~] 6.3.5 Store current timestamp with each score

### 6.4 Implement score display formatting
- [~] 6.4.1 Implement score_manager_get_scores() to retrieve scores for mode
- [~] 6.4.2 Format scores as "Rank. Count locks - Date"
- [~] 6.4.3 Handle empty score lists (display "No scores yet")
- [~] 6.4.4 Format timestamps as readable dates

## Phase 7: Input Handling

### 7.1 Implement pause/resume functionality
- [~] 7.1.1 Implement game_engine_pause() to stop timers
- [~] 7.1.2 Implement game_engine_resume() to restart timers
- [~] 7.1.3 Implement back_click_handler() to pause game
- [~] 7.1.4 Reset last_update_ms on resume to prevent jump
- [~] 7.1.5 Display pause indicator on screen

### 7.2 Implement menu navigation
- [~] 7.2.1 Register UP button handler for menu navigation
- [~] 7.2.2 Register DOWN button handler for menu navigation
- [~] 7.2.3 Register SELECT button handler for menu selection
- [~] 7.2.4 Ensure <50ms input latency

### 7.3 Implement long press for menu exit
- [~] 7.3.1 Implement back_long_click_handler() for 2-second press
- [~] 7.3.2 Return to main menu on long press
- [~] 7.3.3 Clean up game state on exit
- [~] 7.3.4 Register long click handler in game_click_config_provider()

## Phase 8: Testing and Optimization

### 8.1 Test on all target platforms
- [~] 8.1.1 Build for emery platform and test in emulator
- [~] 8.1.2 Build for flint platform and test in emulator
- [~] 8.1.3 Build for gabbro platform and test in emulator
- [~] 8.1.4 Verify color rendering on all platforms
- [~] 8.1.5 Verify display scaling on different screen sizes

### 8.2 Optimize frame rate and battery usage
- [~] 8.2.1 Verify 30 FPS rendering (33ms frame time)
- [~] 8.2.2 Profile frame update times
- [~] 8.2.3 Implement selective redrawing (only when state changes)
- [~] 8.2.4 Verify timers stop when paused
- [~] 8.2.5 Test battery drain during extended play sessions

### 8.3 Test edge cases and error conditions
- [~] 8.3.1 Test angle wraparound (355° to 5°)
- [~] 8.3.2 Test hit detection at all quadrants
- [~] 8.3.3 Test memory allocation failure handling
- [~] 8.3.4 Test persistent storage failure handling
- [~] 8.3.5 Test timer callback delays
- [~] 8.3.6 Test rapid button presses (debouncing)
- [~] 8.3.7 Test state transition edge cases

### 8.4 Final polish and bug fixes
- [~] 8.4.1 Verify all visual elements meet contrast requirements (7:1 ratio)
- [~] 8.4.2 Verify all text meets font size requirements (18pt+ for count, 14pt+ for menu)
- [~] 8.4.3 Verify all animations are 200-500ms duration
- [~] 8.4.4 Verify all vibrations are 50-200ms duration
- [~] 8.4.5 Test complete game flow: menu → play → game over → menu
- [~] 8.4.6 Test pause/resume preserves state correctly
- [~] 8.4.7 Verify memory is released on exit
- [~] 8.4.8 Fix any remaining bugs identified during testing
