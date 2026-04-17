# UnlockThePlock

A modern, fast-paced "Pop the Lock" clone for Pebble smartwatches. Test your reflexes and timing as you try to unlock the plock!

## Features

- **Three Game Modes**:
  - **Classic**: The standard experience. Increasing speed and decreasing tolerance as you level up.
  - **Hardcore**: For those seeking a real challenge. Faster starts and even tighter windows.
  - **Zen**: Relaxed play. Forgiving misses so you can keep the flow going.
- **Dynamic UI**: Vibrant colors (on supported watches), smooth animations, and particle effects.
- **Multi-platform Support**: Optimized for all Pebble platforms, including Round (Chalk), Time (Basalt), and Time 2 (Emery).
- **Persistent High Scores**: Saves your best scores for Classic and Hardcore modes.

## Controls

- **Title Screen**:
  - **UP / DOWN**: Switch between game modes.
  - **SELECT**: Start the game.
- **In-Game**:
  - **SELECT**: Attempt to hit the target.
  - **BACK**: Pause the game (and see your current level).
- **Paused**:
  - **BACK**: Resume the game.
- **Game Over**:
  - **SELECT**: Return to the title screen.

## Installation & Building

This project uses the Pebble SDK.

### Build the app for all platforms
```bash
pebble build
```

### Install on an emulator
```bash
pebble install --emulator basalt
```
*(Add `--vnc` if running in a headless environment)*

### Take a screenshot
```bash
pebble screenshot screenshot.png
```

## Credits

Developed by h3nry.
