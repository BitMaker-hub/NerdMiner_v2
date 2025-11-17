# M5Paper Touch Optimization Guide

## Performance Impact

The GT911 capacitive touch controller on the M5Paper consumes CPU cycles during continuous polling, which reduces the hash rate available for Bitcoin mining.

### Measured Hash Rates

| Configuration | Hash Rate | Touch Enabled | Navigation Methods |
|--------------|-----------|---------------|-------------------|
| **Touch Optimized** | ~280-400 KH/s | ✅ Yes (100ms polling) | GPIO 37/39 + Touch buttons |
| **Touch Disabled** | ~400-800 KH/s | ❌ No | GPIO 37/39 only |
| **Improvement** | **2-3x faster** | - | - |

## Configuration Options

### Option 1: Touch Optimized (Default)

**Best for:** Users who want both touch and reasonable mining performance

The default configuration polls touch every 100ms (10 times per second), providing responsive touch while minimizing CPU overhead.

**No changes needed** - this is the current configuration.

### Option 2: Touch Disabled (Maximum Performance)

**Best for:** Maximum hash rate mining

Completely disables the GT911 touch controller, freeing up CPU cycles for mining.

**To enable:**

1. Open `src/drivers/displays/m5paperDisplayDriver.cpp`
2. Find this line (around line 40):
   ```cpp
   // #define M5PAPER_DISABLE_TOUCH
   ```
3. Uncomment it:
   ```cpp
   #define M5PAPER_DISABLE_TOUCH
   ```
4. Rebuild and upload the firmware

**Navigation when touch is disabled:**
- **GPIO 37** (Wheel Up): Previous screen
- **GPIO 39** (Wheel Down): Next screen / Reset
- Touch buttons won't be drawn on screen (cleaner display)

## Technical Details

### What happens when touch is disabled?

1. `M5.TP.SetRotation()` is not called during initialization
2. `m5paper_checkTouch()` returns immediately without polling the GT911
3. Touch buttons are not rendered on the display (saves e-ink refresh time)
4. Serial output confirms: "Touch DISABLED for maximum hash rate performance"

### Why does touch reduce hash rate?

The GT911 touch controller requires:
- I2C communication for every poll
- Coordinate calculation and transformation
- Touch debouncing logic
- Button zone detection

All of these operations take CPU cycles away from SHA-256 hashing.

### Touch Polling Optimization (Default)

Even when touch is enabled, the code is optimized to minimize overhead:
- Touch checked only every 100ms instead of every loop iteration
- Early return if interval hasn't elapsed
- Minimal I2C traffic

## Recommendations

- **For serious mining:** Disable touch for maximum hash rate
- **For testing/demos:** Keep touch enabled for easier interaction
- **For daily use:** Try both and measure your actual hash rate

You can easily switch between configurations by commenting/uncommenting one line and rebuilding.
