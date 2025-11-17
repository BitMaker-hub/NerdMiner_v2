# M5Paper Complete Guide

## Overview

This guide covers M5Paper v1.1 support for NerdMiner V2, including performance optimization, display configuration, and navigation features.

## Features

### Display Implementation
- **3 Cyclic Screens:**
  1. **Mining Screen** - Hash rate, shares, valid blocks, temperature, battery
  2. **Global Hash Screen** - Network stats, BTC price, difficulty, halving progress
  3. **Pool Stats Screen** - Worker count, pool hash rate, difficulty range, session stats

- **E-ink Optimized:**
  - Full page refresh with GC16 mode (best quality, ~450ms)
  - Partial updates with DU mode (fast, ~260ms)
  - Automatic refresh scheduling to maximize hash rate
  - Smart screen change detection

- **Status Bar (All Screens):**
  - Current time
  - ESP32 temperature
  - Battery percentage with visual indicator

### Navigation Methods

#### GPIO Buttons (Always Available)
- **GPIO 37 (Wheel Up)**: Previous screen
- **GPIO 39 (Wheel Down)**: Next screen
- **Long press (5s)**: Reset WiFi configuration (both buttons)

#### Touch Buttons (When Touch Enabled)
- **[Prev]** button at bottom left
- **[Next]** button at bottom center  
- **[Menu]** button at bottom right (reserved for future use)

## Performance Impact & Optimization

### Hash Rate Comparison

| Configuration | Hash Rate | Touch | Stats Update | Full Refresh |
|--------------|-----------|-------|--------------|--------------|
| **Default (legacy)** | ~150-300 KH/s | ✅ Yes | 1s | 60s |
| **Touch Disabled** | ~300-400 KH/s | ❌ No | 1s | 60s |
| **Optimized (current)** | **~400-600 KH/s** | ❌ No | 5s | 120s |
| **Maximum Performance** | **~600-800 KH/s** | ❌ No | 10s | 300s |

### Why Hash Rate Varies

**Initial High Rate (300-600 KH/s):**
- Mining starts before other tasks
- Network and display not fully active

**Stabilized Rate After Few Minutes:**
- Display updates consume CPU cycles
- Network activity (stratum, API calls)
- Partial e-ink refresh (~260ms) every few seconds
- Full e-ink refresh (~450ms) periodically

**Pool Difficulty Changes (0.0015 → 0.08+) are NORMAL:**
- Pool adjusts to your hash rate
- Doesn't affect computational performance
- Just changes how often you find valid shares

## Configuration Options

### 1. Touch Control (platformio.ini)

**Disable touch for maximum hash rate (recommended):**

Already configured in `platformio.ini`:
```ini
[env:M5Paper-v1-1]
build_flags = 
    -D M5PAPER_DISABLE_TOUCH  ; Touch disabled by default
```

**To enable touch:**
Remove or comment out the line:
```ini
; -D M5PAPER_DISABLE_TOUCH  ; Touch enabled
```

### 2. Display Update Intervals (platformio.ini)

**Current optimized settings:**
```ini
-D M5_STATS_UPDATE_INTERVAL_MS=5000     ; Update every 5 seconds
-D M5_FULL_REFRESH_INTERVAL_MS=120000   ; Full refresh every 2 minutes
```

**For maximum hash rate (less responsive display):**
```ini
-D M5_STATS_UPDATE_INTERVAL_MS=10000    ; Update every 10 seconds
-D M5_FULL_REFRESH_INTERVAL_MS=300000   ; Full refresh every 5 minutes
```

**For more responsive display (lower hash rate):**
```ini
-D M5_STATS_UPDATE_INTERVAL_MS=2000     ; Update every 2 seconds
-D M5_FULL_REFRESH_INTERVAL_MS=60000    ; Full refresh every 1 minute
```

**Impact on hash rate:**
- Lower `STATS_UPDATE_INTERVAL_MS` → More frequent partial refreshes → Lower hash rate
- Lower `FULL_REFRESH_INTERVAL_MS` → More frequent blocking full clears → Lower hash rate

### 3. Alternative: Code-based Configuration

You can also modify `src/drivers/displays/m5paperDisplayDriver.cpp`:

**Around line 40, uncomment to disable touch:**
```cpp
#define M5PAPER_DISABLE_TOUCH
```

**Around line 50, adjust default intervals (if not set in platformio.ini):**
```cpp
#ifndef M5_FULL_REFRESH_INTERVAL_MS
    #define M5_FULL_REFRESH_INTERVAL_MS 120000  // 2 minutes
#endif

#ifndef M5_STATS_UPDATE_INTERVAL_MS
    #define M5_STATS_UPDATE_INTERVAL_MS 5000    // 5 seconds
#endif
```

## Pool Stats Screen Features

### Data Displayed
1. **Workers Count** - Number of active workers
2. **Total Hash Rate** - Combined hash rate of all workers
3. **Session Diff Range** - Min/Max difficulty achieved this session across all workers
4. **Best Diff Ever** - Highest difficulty share ever found
5. **Last Seen** - Most recent worker activity (formatted as "X min ago", "X hr ago", etc.)
6. **Net Difficulty** - Current Bitcoin network difficulty
7. **Net Hash Rate** - Global Bitcoin network hash rate
8. **Block Height** - Current blockchain height
9. **Your Shares** - Number of shares submitted
10. **Valid Blocks** - Valid blocks found (if any)

### Data Source
Pool statistics are fetched from: `https://web.public-pool.io/#/app/<wallet_address>`

The miner parses worker statistics including per-worker difficulty ranges and activity timestamps.

## Technical Details

### Touch Controller Impact

When touch is **enabled**, the GT911 controller requires:
- I2C communication every 100ms
- Coordinate calculation and transformation  
- Touch debouncing logic
- Button zone detection
- ~100-150 KH/s performance penalty

When touch is **disabled**:
- `M5.TP.SetRotation()` not called
- `m5paper_checkTouch()` returns immediately
- No touch buttons rendered
- Maximum hash rate available

### Display Update Strategy

**Partial Updates (DU mode - ~260ms):**
- Updates stats, time, temperature, battery
- Executed every `STATS_UPDATE_INTERVAL_MS`
- Ghosting may occur after many updates

**Full Refresh (GC16 mode - ~450ms):**
- Complete screen clear and redraw
- Executed every `FULL_REFRESH_INTERVAL_MS`
- Removes ghosting, ensures clean display
- **Blocking operation** - system pauses during refresh

### Button Handler Optimization

**Previous Issue (Fixed):**
- GPIO button callback triggered blocking `M5.EPD.Clear()` directly
- Caused OneButton state machine to hang
- Buttons stopped responding after 10-30 seconds

**Current Implementation:**
- Button press only updates screen number (non-blocking)
- `checkForScreenChange()` detects change on next display update
- `M5.EPD.Clear()` called in display task context (safe)
- Buttons remain responsive throughout runtime

### Screen Change Flow

1. User presses GPIO 39 (or GPIO 37)
2. `switchToNextScreen()` increments `current_cyclic_screen`
3. `m5paper_onScreenChange()` logs change (no blocking operations)
4. Next display update cycle runs
5. `checkForScreenChange()` detects screen number changed
6. Performs `M5.EPD.Clear(true)` safely in display task
7. Resets cache to force full redraw
8. New screen renders

## Monitoring Performance

### Serial Output on Startup
```
M5Paper E-ink display initialized (540x960 portrait)
Layout: Image at top (540x240), text labels below
Display update intervals (configurable in platformio.ini):
  - Stats update: 5000 ms
  - Full refresh: 120000 ms
Touch DISABLED for maximum hash rate performance
GPIO 39 = Next Screen, GPIO 37 = Previous Screen
```

### Watching Hash Rate
Monitor serial output for lines like:
```
[MINER] Current hash rate: 487.3 KH/s
[MINER] Valid shares: 42
[MINER] Best difficulty: 0.0876
```

### Expected Behavior
- **First 30 seconds:** High hash rate (500-800 KH/s) while initializing
- **After 2-3 minutes:** Stabilizes (400-600 KH/s) with all tasks running
- **Screen changes:** Brief pause during `M5.EPD.Clear()` (~450ms)
- **Partial updates:** Minimal impact (~260ms every 5 seconds)

## Recommendations

### For Maximum Mining Performance
```ini
-D M5PAPER_DISABLE_TOUCH
-D M5_STATS_UPDATE_INTERVAL_MS=10000
-D M5_FULL_REFRESH_INTERVAL_MS=300000
```
**Expected:** 600-800 KH/s sustained

### For Balanced Performance
```ini
-D M5PAPER_DISABLE_TOUCH
-D M5_STATS_UPDATE_INTERVAL_MS=5000     ; Current default
-D M5_FULL_REFRESH_INTERVAL_MS=120000   ; Current default
```
**Expected:** 400-600 KH/s sustained (recommended)

### For Responsive Display (Lower Hash Rate)
```ini
; -D M5PAPER_DISABLE_TOUCH              ; Touch enabled
-D M5_STATS_UPDATE_INTERVAL_MS=1000
-D M5_FULL_REFRESH_INTERVAL_MS=60000
```
**Expected:** 200-350 KH/s sustained

## Troubleshooting

### Buttons Stop Working
- **Fixed** in current version
- Ensure you're using latest code with `m5paper_onScreenChange()` non-blocking implementation

### Hash Rate Drops Over Time
- Normal behavior as display updates and network activity stabilize
- Adjust `M5_STATS_UPDATE_INTERVAL_MS` higher for better sustained rate
- Check temperature - throttling occurs if ESP32 overheats

### Display Shows Ghosting
- Increase `M5_FULL_REFRESH_INTERVAL_MS` too high causes ghosting buildup
- Recommended minimum: 60000ms (1 minute)
- Use screen change to force immediate refresh

### Pool Difficulty Increasing
- **This is normal** - pool auto-adjusts to your hash rate
- Higher difficulty = fewer but more valuable shares
- Doesn't affect your actual computational hash rate

## Future Enhancements

- [ ] Menu button functionality (settings, WiFi reconnect)
- [ ] Touch zones for on-screen data refresh
- [ ] Configurable screen rotation
- [ ] Deep sleep mode for battery operation
- [ ] Historical hash rate graph
- [ ] Per-worker detailed stats on pool screen

## Credits

M5Paper support developed for NerdMiner V2
- E-ink optimized display driver
- Multi-screen navigation with GPIO and touch
- Performance-optimized refresh scheduling
- Pool statistics integration
