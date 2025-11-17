#include "displayDriver.h"

#ifdef M5PAPER_DISPLAY

#include <M5EPD.h>
#include "m5paperDisplayDriver.h"
#include "version.h"
#include "monitor.h"
#include "media/images_m5paper_gray.h"

#define M5PAPER_WIDTH 540
#define M5PAPER_HEIGHT 960

// Portrait mode: image at top (540x240), text labels below
#define IMAGE_HEIGHT 240

/*
 * ===== TOUCH OPTIMIZATION FOR HASH RATE PERFORMANCE =====
 * 
 * The GT911 capacitive touch controller consumes CPU cycles that could be
 * used for Bitcoin mining. This can reduce hash rate by 30-70%.
 * 
 * OPTIMIZATION OPTIONS:
 * 
 * 1. DEFAULT (current): Touch polling optimized to 100ms intervals
 *    - Hash rate: ~280-400 KH/s
 *    - Touch enabled with minimal overhead
 *    - Screen navigation: GPIO buttons + Touch buttons
 * 
 * 2. MAXIMUM PERFORMANCE: Disable touch completely
 *    - Hash rate: ~400-800 KH/s (up to 2-3x improvement!)
 *    - Touch completely disabled, GPIO buttons only
 *    - Screen navigation: GPIO 37 (prev), GPIO 39 (next)
 *    - To enable: Uncomment the line below
 * 
 * #define M5PAPER_DISABLE_TOUCH
 * 
 * Note: Even with touch disabled, GPIO buttons work perfectly for navigation.
 */

// Uncomment the line below to disable touch for maximum hash rate
// #define M5PAPER_DISABLE_TOUCH

// Button pin for previous screen (GPIO 37 - Wheel Up)
#define PREV_SCREEN_BUTTON 37

// E-ink display canvases
M5EPD_Canvas canvas_page(&M5.EPD);   // Full page canvas for static content
M5EPD_Canvas canvas_stats(&M5.EPD);  // Small canvas for dynamic stats updates

// Track display state
static int currentPage = 0;
static int lastDisplayedScreen = -1;  // Track which screen was last displayed
static unsigned long lastFullRefresh = 0;
static unsigned long lastStatsUpdate = 0;
static unsigned long lastButtonCheck = 0;
static unsigned long lastTouchCheck = 0;
static bool prevButtonPressed = false;

const unsigned long FULL_REFRESH_INTERVAL_MS = 60000;  // Full refresh every 60 seconds
const unsigned long STATS_UPDATE_INTERVAL_MS = 1000;    // Stats update every second
const unsigned long BUTTON_CHECK_INTERVAL_MS = 200;     // Check button every 200ms

#ifndef M5PAPER_DISABLE_TOUCH
const unsigned long TOUCH_CHECK_INTERVAL_MS = 100;      // Check touch every 100ms (only if touch enabled)
#endif

// Previous values for change detection
static String prev_hashrate = "";
static String prev_shares = "";
static String prev_btcPrice = "";
static String prev_blockHeight = "";

// Forward declaration
extern DisplayDriver m5paperDisplayDriver;

// Helper: get battery percentage from M5Paper
static int getBatteryPercentage()
{
    // M5Paper uses IP5306 power management IC
    // Battery voltage range: 3.0V (0%) to 4.2V (100%)
    uint32_t vol = M5.getBatteryVoltage();
    
    if (vol < 3000) return 0;
    if (vol > 4200) return 100;
    
    // Linear interpolation between 3.0V and 4.2V
    int percentage = ((vol - 3000) * 100) / 1200;
    return constrain(percentage, 0, 100);
}

// Helper: draw grayscale image from PROGMEM
// Images are now 960x540 fullscreen, displayed at (0,0)
static void draw_grayscale_image(M5EPD_Canvas &canvas, const uint8_t* img_data, 
                                  uint16_t img_width, uint16_t img_height,
                                  int x_offset = 0, int y_offset = 0)
{
    // Draw image pixel by pixel
    // Image format: two 4-bit pixels per byte (high nibble, low nibble)
    uint32_t pixel_index = 0;
    
    for (int y = 0; y < img_height; y++) {
        for (int x = 0; x < img_width; x++) {
            // Get packed byte from PROGMEM
            uint32_t byte_index = pixel_index / 2;
            uint8_t packed_byte = pgm_read_byte(&img_data[byte_index]);
            
            // Extract 4-bit grayscale value
            uint8_t gray4;
            if (pixel_index % 2 == 0) {
                // High nibble (first pixel)
                gray4 = (packed_byte >> 4) & 0x0F;
            } else {
                // Low nibble (second pixel)
                gray4 = packed_byte & 0x0F;
            }
            
            // Draw pixel (M5EPD uses 0=white to 15=black)
            canvas.drawPixel(x_offset + x, y_offset + y, gray4);
            pixel_index++;
        }
    }
}

// Helper: draw an outline button
static void draw_outline_button(M5EPD_Canvas &c, int x, int y, int w, int h, int color, const char *label)
{
    const int stroke = 2;
    for (int s = 0; s < stroke; s++) {
        c.drawRoundRect(x + s, y + s, w - 2 * s, h - 2 * s, 5, color);  // 5px radius
    }

    c.setTextDatum(MC_DATUM);
    c.setTextSize(2);
    c.setTextColor(color);
    int cx = x + w / 2;
    int cy = y + h / 2;
    c.drawString(label, cx, cy);
    c.setTextDatum(TL_DATUM);
}

// Helper: draw a filled button (highlighted)
static void draw_filled_button(M5EPD_Canvas &c, int x, int y, int w, int h, int r, const char *label)
{
    c.fillRoundRect(x, y, w, h, r, 15);
    
    c.setTextDatum(MC_DATUM);
    c.setTextSize(2);
    c.setTextColor(0); // White text on black background
    int cx = x + w / 2;
    int cy = y + h / 2;
    c.drawString(label, cx, cy);
    c.setTextColor(15); // Reset to black
    c.setTextDatum(TL_DATUM);
}

// Helper: check if screen changed externally (e.g., via GPIO button) and force refresh
static void checkForScreenChange() {
    if (m5paperDisplayDriver.current_cyclic_screen != lastDisplayedScreen) {
        Serial.printf("Screen changed externally from %d to %d - forcing full refresh\n", 
                     lastDisplayedScreen, m5paperDisplayDriver.current_cyclic_screen);
        
        // Force complete page refresh on screen change
        M5.EPD.Clear(true);  // Full clear to remove all previous content
        lastFullRefresh = 0;
        prev_shares = "";    // Reset to force redraw
        prev_hashrate = "";
        prev_btcPrice = "";
        prev_blockHeight = "";
        
        lastDisplayedScreen = m5paperDisplayDriver.current_cyclic_screen;
    }
}

// Function to go to previous screen
void switchToPreviousScreen() {
    int numScreens = m5paperDisplayDriver.num_cyclic_screens;
    m5paperDisplayDriver.current_cyclic_screen = 
        (m5paperDisplayDriver.current_cyclic_screen - 1 + numScreens) % numScreens;
    
    // Force complete page refresh on screen change
    M5.EPD.Clear(true);  // Full clear to remove all previous content
    lastFullRefresh = 0;
    prev_shares = "";    // Reset to force redraw
    prev_hashrate = "";
    prev_btcPrice = "";
    prev_blockHeight = "";
    
    lastDisplayedScreen = m5paperDisplayDriver.current_cyclic_screen;  // Update tracker
    
    Serial.printf("Switched to previous screen: %d\n", m5paperDisplayDriver.current_cyclic_screen);
}

// Function to go to next screen (for touch)
void switchToNextScreenTouch() {
    int numScreens = m5paperDisplayDriver.num_cyclic_screens;
    m5paperDisplayDriver.current_cyclic_screen = 
        (m5paperDisplayDriver.current_cyclic_screen + 1) % numScreens;
    
    // Force complete page refresh on screen change
    M5.EPD.Clear(true);  // Full clear to remove all previous content
    lastFullRefresh = 0;
    prev_shares = "";    // Reset to force redraw
    prev_hashrate = "";
    prev_btcPrice = "";
    prev_blockHeight = "";
    
    lastDisplayedScreen = m5paperDisplayDriver.current_cyclic_screen;  // Update tracker
    
    Serial.printf("Switched to next screen: %d\n", m5paperDisplayDriver.current_cyclic_screen);
}

// Check if previous screen button is pressed (GPIO 37)
void checkPrevScreenButton() {
    if (millis() - lastButtonCheck < BUTTON_CHECK_INTERVAL_MS) {
        return;
    }
    lastButtonCheck = millis();
    
    bool currentState = (digitalRead(PREV_SCREEN_BUTTON) == LOW);  // Active LOW
    
    // Detect button press (edge detection)
    if (currentState && !prevButtonPressed) {
        switchToPreviousScreen();
    }
    
    prevButtonPressed = currentState;
}

// Touch handling for M5Paper GT911 - returns touch state with button info
TouchState_M5Paper m5paper_checkTouch(int currentScreen)
{
    TouchState_M5Paper state = {false, false, 0, 0, 0};
    
#ifndef M5PAPER_DISABLE_TOUCH
    static bool wasTouched = false;
    static int lastButton = 0;
    static unsigned long lastReleaseTime = 0;
    
    // Rate limiting - only check touch every TOUCH_CHECK_INTERVAL_MS
    if (millis() - lastTouchCheck < TOUCH_CHECK_INTERVAL_MS) {
        return state;
    }
    lastTouchCheck = millis();
    
    // Debounce: ignore touches for 200ms after release
    if (!wasTouched && (millis() - lastReleaseTime) < 200) {
        return state;
    }
    
    M5.TP.update();
    
    if (!M5.TP.available()) {
        if (wasTouched) {
            state.justReleased = true;
            state.buttonNumber = lastButton;
            wasTouched = false;
            lastButton = 0;
            lastReleaseTime = millis();
            Serial.println("[M5PAPER] Touch released");
        }
        return state;
    }
    
    // Check if finger is up
    if (M5.TP.isFingerUp()) {
        if (wasTouched) {
            state.justReleased = true;
            state.buttonNumber = lastButton;
            wasTouched = false;
            lastButton = 0;
            lastReleaseTime = millis();
            Serial.println("[M5PAPER] Touch released (finger up)");
        }
        return state;
    }
    
    tp_finger_t finger = M5.TP.readFinger(0);
    
    state.x = finger.x;
    state.y = finger.y;
    
    // Filter out invalid touches (0,0 is false positive)
    if (finger.x == 0 && finger.y == 0) {
        return state;
    }
    
    // Debug: print all touches
    if (!wasTouched) {
        Serial.printf("[M5PAPER] Touch detected at (%d, %d)\n", finger.x, finger.y);
    }
    
    // Define button areas - buttons at bottom of screen
    // Touch coordinates in landscape (960x540): X = 0-960, Y = 0-540
    if (finger.y > 460 && finger.y < 540) {
        int buttonPressed = 0;
        
        // All screens have navigation buttons at bottom
        const int btnW = 240;
        const int spacing = 60;
        
        // Three buttons layout: [Prev] [Next] [Menu]
        const int totalW = btnW * 3 + spacing * 2;
        const int startX = (M5PAPER_WIDTH - totalW) / 2;
        
        Serial.printf("[M5PAPER] Touch in button area. StartX=%d, finger.x=%d\n", startX, finger.x);
        
        if (finger.x >= startX && finger.x < (startX + btnW)) {
            buttonPressed = 1;  // Previous screen
            Serial.println("[M5PAPER] Prev button zone");
        } else if (finger.x >= (startX + btnW + spacing) && 
                   finger.x < (startX + 2 * btnW + spacing)) {
            buttonPressed = 2;  // Next screen
            Serial.println("[M5PAPER] Next button zone");
        } else if (finger.x >= (startX + 2 * (btnW + spacing)) && 
                   finger.x < (startX + totalW)) {
            buttonPressed = 3;  // Menu/Options
            Serial.println("[M5PAPER] Menu button zone");
        }
        
        if (buttonPressed > 0) {
            state.isPressed = true;
            state.buttonNumber = buttonPressed;
            
            if (!wasTouched || lastButton != buttonPressed) {
                Serial.printf("[M5PAPER] Button %d pressed at (%d,%d)\n", 
                             buttonPressed, finger.x, finger.y);
                wasTouched = true;
                lastButton = buttonPressed;
            }
        }
    }
    
#endif // M5PAPER_DISABLE_TOUCH
    
    return state;
}

void m5paper_Init(void)
{
    M5.begin();
    M5.EPD.SetRotation(0);  // Try rotation 0 for landscape
    
#ifndef M5PAPER_DISABLE_TOUCH
    M5.TP.SetRotation(0);   // Touch rotation must match display
#endif
    
    M5.EPD.Clear(true);      // Clear with full refresh
    M5.RTC.begin();
    
    // Initialize previous screen button (GPIO 37)
    pinMode(PREV_SCREEN_BUTTON, INPUT_PULLUP);
    
    // Set portrait rotation (90 degrees)
    M5.EPD.SetRotation(90);
    
    // Create canvases for portrait 540x960
    canvas_page.createCanvas(M5PAPER_WIDTH, M5PAPER_HEIGHT);
    canvas_page.setTextSize(2);
    
    // Smaller canvas for text area updates (below image)
    canvas_stats.createCanvas(M5PAPER_WIDTH, M5PAPER_HEIGHT - IMAGE_HEIGHT);
    canvas_stats.setTextSize(2);
    
    Serial.println("M5Paper E-ink display initialized (540x960 portrait)");
    Serial.println("Layout: Image at top (540x240), text labels below");
    
#ifdef M5PAPER_DISABLE_TOUCH
    Serial.println("Touch DISABLED for maximum hash rate performance");
    Serial.println("GPIO 39 = Next Screen, GPIO 37 = Previous Screen");
#else
    Serial.println("Touch enabled: GT911 capacitive touch controller");
    Serial.println("GPIO 39 = Next Screen, GPIO 37 = Previous Screen");
    Serial.println("Touch: [Prev] [Next] [Menu] buttons at screen bottom");
#endif
}

void m5paper_AlternateScreenState(void)
{
    // E-ink doesn't have backlight, but we can do a full refresh
    M5.EPD.Clear(true);
    lastFullRefresh = millis();
}

void m5paper_AlternateRotation(void)
{
    // For now, keep fixed landscape orientation
    // Can be implemented later if needed
}

void m5paper_LoadingScreen(void)
{
    // Splash screen is landscape (960x540) - temporarily switch rotation
    M5.EPD.SetRotation(0);  // Landscape for splash
    
    M5EPD_Canvas temp_canvas(&M5.EPD);
    temp_canvas.createCanvas(960, 540);
    temp_canvas.fillCanvas(0);  // Black background
    
    // Draw landscape splash image (960x540)
    draw_grayscale_image(temp_canvas, initScreen_gray, 
                        initScreen_gray_width, initScreen_gray_height);
    
    temp_canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
    
    // Switch back to portrait for other screens
    M5.EPD.SetRotation(90);
    
    lastFullRefresh = millis();
}

void m5paper_SetupScreen(void)
{
    canvas_page.fillCanvas(0);  // Black background
    
    // Draw setup mode image at top (540x240)
    draw_grayscale_image(canvas_page, setupModeScreen_gray, 
                        setupModeScreen_gray_width, setupModeScreen_gray_height);
    
    canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
    lastFullRefresh = millis();
}

void m5paper_MinerScreen(unsigned long mElapsed)
{
    // Check if screen was changed externally (e.g., GPIO button from main code)
    checkForScreenChange();
    
    // Check for previous screen button press (GPIO 37)
    checkPrevScreenButton();
    
    // Check for touch input
    TouchState_M5Paper touch = m5paper_checkTouch(m5paperDisplayDriver.current_cyclic_screen);
    if (touch.justReleased) {
        if (touch.buttonNumber == 1) {
            switchToPreviousScreen();
            return;
        } else if (touch.buttonNumber == 2) {
            switchToNextScreenTouch();
            return;
        }
    }
    
    mining_data data = getMiningData(mElapsed);
    clock_data clockData = getClockData(mElapsed);
    
    // Get battery percentage
    int batteryPct = getBatteryPercentage();
    
    // Full refresh every 60 seconds or on first load
    bool needFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS) || 
                           (prev_hashrate.isEmpty());
    
    if (needFullRefresh) {
        // Full page refresh with image
        canvas_page.fillCanvas(0);  // Black background
        
        // Draw image at top (540x240)
        draw_grayscale_image(canvas_page, MinerScreen_gray, 
                           MinerScreen_gray_width, MinerScreen_gray_height);
        
        // STATUS BAR - Below image with time, temp, and battery
        canvas_page.setTextColor(15);  // White text
        canvas_page.setTextDatum(TL_DATUM);
        canvas_page.setTextSize(2);
        int statusY = IMAGE_HEIGHT + 5;
        
        // Left side: Time
        canvas_page.drawString(data.currentTime, 10, statusY);
        
        // Center: Temperature with label
        canvas_page.setTextDatum(TC_DATUM);
        String tempLabel = "Temp: " + data.temp;
        canvas_page.drawString(tempLabel, M5PAPER_WIDTH / 2, statusY);
        
        // Right side: Battery with rounded rectangle
        canvas_page.setTextDatum(TR_DATUM);
        int battX = M5PAPER_WIDTH - 10;
        int battY = statusY;
        int battW = 70;
        int battH = 24;
        
        // Draw battery rounded rectangle
        canvas_page.drawRoundRect(battX - battW, battY, battW, battH, 5, 15);
        canvas_page.setTextDatum(MC_DATUM);
        String battText = String(batteryPct) + "%";
        canvas_page.drawString(battText, battX - battW/2, battY + battH/2);
        
        // Draw separator line below status bar
        int separatorY = statusY + 28;
        canvas_page.drawLine(10, separatorY, M5PAPER_WIDTH - 10, separatorY, 15);
        
        // Data area starts below status bar and separator
        canvas_page.setTextDatum(TC_DATUM);  // Top center
        int y = IMAGE_HEIGHT + 45;
        int centerX = M5PAPER_WIDTH / 2;
        
        // VALID BLOCKS - Bigger and prominent
        canvas_page.setTextSize(3);
        canvas_page.drawString("VALID BLOCKS", centerX, y);
        y += 40;
        canvas_page.setTextSize(5);
        canvas_page.drawString(data.valids, centerX, y);
        y += 70;
        
        // HASH RATE - Bigger and prominent
        canvas_page.setTextSize(3);
        canvas_page.drawString("HASH RATE", centerX, y);
        y += 40;
        canvas_page.setTextSize(4);
        canvas_page.drawString(data.currentHashRate + " KH/s", centerX, y);
        y += 60;
        
        // Rest of data in two columns
        canvas_page.setTextSize(2);
        canvas_page.setTextDatum(TL_DATUM);
        int leftCol = 20;
        int rightCol = 280;
        
        canvas_page.drawString("Total Hashes:", leftCol, y);
        canvas_page.drawString(data.totalMHashes + " MH", rightCol, y);
        y += 35;
        
        canvas_page.drawString("Templates:", leftCol, y);
        canvas_page.drawString(data.templates, rightCol, y);
        y += 35;
        
        canvas_page.drawString("Best Diff:", leftCol, y);
        canvas_page.drawString(data.bestDiff, rightCol, y);
        y += 35;
        
        canvas_page.drawString("Shares:", leftCol, y);
        canvas_page.drawString(data.completedShares, rightCol, y);
        y += 35;
        
        canvas_page.drawString("Timer:", leftCol, y);
        canvas_page.drawString(data.timeMining, rightCol, y);
        y += 35;
        
        canvas_page.drawString("BTC Price:", leftCol, y);
        canvas_page.drawString("$" + clockData.btcPrice, rightCol, y);
        y += 35;
        
        canvas_page.drawString("Block Height:", leftCol, y);
        canvas_page.drawString(clockData.blockHeight, rightCol, y);
        y += 45;
        
#ifndef M5PAPER_DISABLE_TOUCH
        // Draw touch buttons at bottom
        int btnY = M5PAPER_HEIGHT - 80;
        draw_outline_button(canvas_page, 20, btnY, 150, 60, 15, "Prev");
        draw_outline_button(canvas_page, 195, btnY, 150, 60, 15, "Next");
        draw_outline_button(canvas_page, 370, btnY, 150, 60, 15, "Menu");
#endif
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
        prev_hashrate = data.currentHashRate;
        prev_shares = data.completedShares;
    }
    else if (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS) {
        // Partial update - update only the dynamic text area (not status bar or image)
        canvas_stats.fillCanvas(0);  // Black background
        canvas_stats.setTextColor(15);  // White text
        
        // STATUS BAR - Below image
        canvas_stats.setTextDatum(TL_DATUM);
        canvas_stats.setTextSize(2);
        int statusY = 5;
        
        // Left side: Time
        canvas_stats.drawString(data.currentTime, 10, statusY);
        
        // Center: Temperature with label
        canvas_stats.setTextDatum(TC_DATUM);
        String tempLabel = "Temp: " + data.temp;
        canvas_stats.drawString(tempLabel, M5PAPER_WIDTH / 2, statusY);
        
        // Right side: Battery with rounded rectangle
        canvas_stats.setTextDatum(TR_DATUM);
        int battX = M5PAPER_WIDTH - 10;
        int battY = statusY;
        int battW = 70;
        int battH = 24;
        
        // Draw battery rounded rectangle
        canvas_stats.drawRoundRect(battX - battW, battY, battW, battH, 5, 15);
        canvas_stats.setTextDatum(MC_DATUM);
        String battText = String(batteryPct) + "%";
        canvas_stats.drawString(battText, battX - battW/2, battY + battH/2);
        
        // Draw separator line below status bar
        int separatorY = statusY + 28;
        canvas_stats.drawLine(10, separatorY, M5PAPER_WIDTH - 10, separatorY, 15);
        
        // Data area
        int y = 45;
        int centerX = M5PAPER_WIDTH / 2;
        
        // VALID BLOCKS
        canvas_stats.setTextDatum(TC_DATUM);
        canvas_stats.setTextSize(3);
        canvas_stats.drawString("VALID BLOCKS", centerX, y);
        y += 40;
        canvas_stats.setTextSize(5);
        canvas_stats.drawString(data.valids, centerX, y);
        y += 70;
        
        // HASH RATE
        canvas_stats.setTextSize(3);
        canvas_stats.drawString("HASH RATE", centerX, y);
        y += 40;
        canvas_stats.setTextSize(4);
        canvas_stats.drawString(data.currentHashRate + " KH/s", centerX, y);
        y += 60;
        
        // Rest of data
        canvas_stats.setTextSize(2);
        canvas_stats.setTextDatum(TL_DATUM);
        int leftCol = 20;
        int rightCol = 280;
        
        canvas_stats.drawString("Total Hashes:", leftCol, y);
        canvas_stats.drawString(data.totalMHashes + " MH", rightCol, y);
        y += 35;
        
        canvas_stats.drawString("Templates:", leftCol, y);
        canvas_stats.drawString(data.templates, rightCol, y);
        y += 35;
        
        canvas_stats.drawString("Best Diff:", leftCol, y);
        canvas_stats.drawString(data.bestDiff, rightCol, y);
        y += 35;
        
        canvas_stats.drawString("Shares:", leftCol, y);
        canvas_stats.drawString(data.completedShares, rightCol, y);
        y += 35;
        
        canvas_stats.drawString("Timer:", leftCol, y);
        canvas_stats.drawString(data.timeMining, rightCol, y);
        y += 35;
        
        canvas_stats.drawString("BTC Price:", leftCol, y);
        canvas_stats.drawString("$" + clockData.btcPrice, rightCol, y);
        y += 35;
        
        canvas_stats.drawString("Block Height:", leftCol, y);
        canvas_stats.drawString(clockData.blockHeight, rightCol, y);
        
        // Push stats canvas below image (includes status bar and all data)
        canvas_stats.pushCanvas(0, IMAGE_HEIGHT, UPDATE_MODE_DU);
        lastStatsUpdate = millis();
        prev_hashrate = data.currentHashRate;
        prev_shares = data.completedShares;
    }
}

/*
// Clock screen removed - BTC price and block height now shown on mining screen
void m5paper_ClockScreen(unsigned long mElapsed)
{
    checkForScreenChange();
    checkPrevScreenButton();
    
    TouchState_M5Paper touch = m5paper_checkTouch(m5paperDisplayDriver.current_cyclic_screen);
    if (touch.justReleased) {
        if (touch.buttonNumber == 1) {
            switchToPreviousScreen();
            return;
        } else if (touch.buttonNumber == 2) {
            switchToNextScreenTouch();
            return;
        }
    }
    
    clock_data data = getClockData(mElapsed);
    
    // Full refresh every 60 seconds or on first load
    bool needFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS) || 
                           (prev_btcPrice.isEmpty());
    
    if (needFullRefresh) {
        // Full page refresh
        canvas_page.fillCanvas(0);  // Black background
        
        // Draw image at top (540x240)
        draw_grayscale_image(canvas_page, minerClockScreen_gray, 
                           minerClockScreen_gray_width, minerClockScreen_gray_height);
        
        // Draw text labels below image
        canvas_page.setTextColor(15);  // White text
        canvas_page.setTextDatum(MC_DATUM);  // Center alignment
        int y = IMAGE_HEIGHT + 80;
        
        // Large time display
        canvas_page.setTextSize(6);
        canvas_page.drawString(data.currentTime, M5PAPER_WIDTH / 2, y);
        
        y += 120;
        canvas_page.setTextSize(3);
        canvas_page.drawString(data.currentHashRate + " KH/s", M5PAPER_WIDTH / 2, y);
        
        y += 60;
        canvas_page.setTextSize(2);
        canvas_page.drawString("BTC: " + data.btcPrice, M5PAPER_WIDTH / 2, y);
        
        y += 50;
        canvas_page.drawString("Block: " + data.blockHeight, M5PAPER_WIDTH / 2, y);
        
        y += 50;
        canvas_page.drawString(data.completedShares + " shares", M5PAPER_WIDTH / 2, y);
        
#ifndef M5PAPER_DISABLE_TOUCH
        int btnY = M5PAPER_HEIGHT - 80;
        draw_outline_button(canvas_page, 20, btnY, 150, 60, 15, "Prev");
        draw_outline_button(canvas_page, 195, btnY, 150, 60, 15, "Next");
        draw_outline_button(canvas_page, 370, btnY, 150, 60, 15, "Menu");
#endif
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
        prev_btcPrice = data.btcPrice;
        prev_blockHeight = data.blockHeight;
    }
    else if (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS) {
        // Partial update of text area only
        canvas_stats.fillCanvas(0);  // Black background
        canvas_stats.setTextColor(15);  // White text
        canvas_stats.setTextDatum(MC_DATUM);
        
        int y = 80;
        
        // Large time display
        canvas_stats.setTextSize(6);
        canvas_stats.drawString(data.currentTime, M5PAPER_WIDTH / 2, y);
        
        y += 120;
        canvas_stats.setTextSize(3);
        canvas_stats.drawString(data.currentHashRate + " KH/s", M5PAPER_WIDTH / 2, y);
        
        y += 60;
        canvas_stats.setTextSize(2);
        canvas_stats.drawString("BTC: " + data.btcPrice, M5PAPER_WIDTH / 2, y);
        
        y += 50;
        canvas_stats.drawString("Block: " + data.blockHeight, M5PAPER_WIDTH / 2, y);
        
        y += 50;
        canvas_stats.drawString(data.completedShares + " shares", M5PAPER_WIDTH / 2, y);
        
        // Push stats canvas below image
        canvas_stats.pushCanvas(0, IMAGE_HEIGHT, UPDATE_MODE_DU);
        lastStatsUpdate = millis();
        prev_btcPrice = data.btcPrice;
        prev_blockHeight = data.blockHeight;
    }
}
*/

void m5paper_GlobalHashScreen(unsigned long mElapsed)
{
    checkForScreenChange();
    checkPrevScreenButton();
    
    TouchState_M5Paper touch = m5paper_checkTouch(m5paperDisplayDriver.current_cyclic_screen);
    if (touch.justReleased) {
        if (touch.buttonNumber == 1) {
            switchToPreviousScreen();
            return;
        } else if (touch.buttonNumber == 2) {
            switchToNextScreenTouch();
            return;
        }
    }
    
    coin_data data = getCoinData(mElapsed);
    clock_data clockData = getClockData(mElapsed);
    
    // Get battery percentage
    int batteryPct = getBatteryPercentage();
    
    // Full refresh every 60 seconds or on first load
    bool needFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS) || 
                           (prev_blockHeight.isEmpty());
    
    if (needFullRefresh) {
        // Full page refresh
        canvas_page.fillCanvas(0);  // Black background
        
        // Draw image at top (540x240)
        draw_grayscale_image(canvas_page, globalHashScreen_gray, 
                           globalHashScreen_gray_width, globalHashScreen_gray_height);
        
        // STATUS BAR - Below image with time, temp, and battery
        canvas_page.setTextColor(15);  // White text
        canvas_page.setTextDatum(TL_DATUM);
        canvas_page.setTextSize(2);
        int statusY = IMAGE_HEIGHT + 5;
        
        // Left side: Time
        canvas_page.drawString(clockData.currentTime, 10, statusY);
        
        // Center: Temperature with label
        canvas_page.setTextDatum(TC_DATUM);
        mining_data miningData = getMiningData(mElapsed);
        String tempLabel = "Temp: " + miningData.temp;
        canvas_page.drawString(tempLabel, M5PAPER_WIDTH / 2, statusY);
        
        // Right side: Battery with rounded rectangle
        canvas_page.setTextDatum(TR_DATUM);
        int battX = M5PAPER_WIDTH - 10;
        int battY = statusY;
        int battW = 70;
        int battH = 24;
        
        // Draw battery rounded rectangle
        canvas_page.drawRoundRect(battX - battW, battY, battW, battH, 5, 15);
        canvas_page.setTextDatum(MC_DATUM);
        String battText = String(batteryPct) + "%";
        canvas_page.drawString(battText, battX - battW/2, battY + battH/2);
        
        // Draw separator line below status bar
        int separatorY = statusY + 28;
        canvas_page.drawLine(10, separatorY, M5PAPER_WIDTH - 10, separatorY, 15);
        
        // Draw text labels below status bar
        canvas_page.setTextColor(15);  // White text
        canvas_page.setTextDatum(TL_DATUM);
        int y = IMAGE_HEIGHT + 45;
        int leftCol = 20;
        int rightCol = 260;
        
        canvas_page.setTextSize(3);
        canvas_page.drawString("Block Height:", leftCol, y);
        canvas_page.drawString(data.blockHeight, rightCol, y);
        
        y += 60;
        canvas_page.setTextSize(2);
        canvas_page.drawString("BTC Price:", leftCol, y);
        canvas_page.drawString(data.btcPrice, rightCol, y);
        
        y += 45;
        canvas_page.drawString("Global Hashrate:", leftCol, y);
        canvas_page.drawString(data.globalHashRate, rightCol, y);
        
        y += 45;
        canvas_page.drawString("Difficulty:", leftCol, y);
        canvas_page.drawString(data.netwrokDifficulty, rightCol, y);
        
        y += 45;
        canvas_page.drawString("Half Hour Fee:", leftCol, y);
        canvas_page.drawString(data.halfHourFee, rightCol, y);
        
        y += 50;
        canvas_page.drawString("Remaining Blocks:", leftCol, y);
        canvas_page.drawString(data.remainingBlocks, rightCol, y);
        
        // Draw halving progress bar
        y += 50;
        int barWidth = M5PAPER_WIDTH - 40;
        int barX = 20;
        int filledWidth = (barWidth * data.progressPercent) / 100;
        canvas_page.drawRect(barX, y, barWidth, 30, 15);  // White outline
        canvas_page.fillRect(barX + 2, y + 2, filledWidth - 4, 26, 15);  // White fill
        
        canvas_page.setTextDatum(MC_DATUM);
        canvas_page.setTextSize(2);
        canvas_page.setTextColor(0);  // Black text on white bar
        canvas_page.drawString(String(data.progressPercent) + "%", barX + barWidth / 2, y + 15);
        
#ifndef M5PAPER_DISABLE_TOUCH
        int btnY = M5PAPER_HEIGHT - 80;
        draw_outline_button(canvas_page, 20, btnY, 150, 60, 15, "Prev");
        draw_outline_button(canvas_page, 195, btnY, 150, 60, 15, "Next");
        draw_outline_button(canvas_page, 370, btnY, 150, 60, 15, "Menu");
#endif
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
        prev_blockHeight = data.blockHeight;
        prev_btcPrice = data.btcPrice;
    }
    else if (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS) {
        // Partial update - update only the dynamic text area
        canvas_stats.fillCanvas(0);  // Black background
        canvas_stats.setTextColor(15);  // White text
        
        // STATUS BAR
        canvas_stats.setTextDatum(TL_DATUM);
        canvas_stats.setTextSize(2);
        int statusY = 5;
        
        // Left side: Time
        canvas_stats.drawString(clockData.currentTime, 10, statusY);
        
        // Center: Temperature with label
        canvas_stats.setTextDatum(TC_DATUM);
        mining_data miningData = getMiningData(mElapsed);
        String tempLabel = "Temp: " + miningData.temp;
        canvas_stats.drawString(tempLabel, M5PAPER_WIDTH / 2, statusY);
        
        // Right side: Battery with rounded rectangle
        canvas_stats.setTextDatum(TR_DATUM);
        int battX = M5PAPER_WIDTH - 10;
        int battY = statusY;
        int battW = 70;
        int battH = 24;
        
        // Draw battery rounded rectangle
        canvas_stats.drawRoundRect(battX - battW, battY, battW, battH, 5, 15);
        canvas_stats.setTextDatum(MC_DATUM);
        String battText = String(batteryPct) + "%";
        canvas_stats.drawString(battText, battX - battW/2, battY + battH/2);
        
        // Draw separator line below status bar
        int separatorY = statusY + 28;
        canvas_stats.drawLine(10, separatorY, M5PAPER_WIDTH - 10, separatorY, 15);
        
        // Data area
        canvas_stats.setTextDatum(TL_DATUM);
        int y = 45;
        int leftCol = 20;
        int rightCol = 260;
        
        canvas_stats.setTextSize(3);
        canvas_stats.drawString("Block Height:", leftCol, y);
        canvas_stats.drawString(data.blockHeight, rightCol, y);
        
        y += 60;
        canvas_stats.setTextSize(2);
        canvas_stats.drawString("BTC Price:", leftCol, y);
        canvas_stats.drawString(data.btcPrice, rightCol, y);
        
        y += 45;
        canvas_stats.drawString("Global Hashrate:", leftCol, y);
        canvas_stats.drawString(data.globalHashRate, rightCol, y);
        
        y += 45;
        canvas_stats.drawString("Difficulty:", leftCol, y);
        canvas_stats.drawString(data.netwrokDifficulty, rightCol, y);
        
        y += 45;
        canvas_stats.drawString("Half Hour Fee:", leftCol, y);
        canvas_stats.drawString(data.halfHourFee, rightCol, y);
        
        y += 50;
        canvas_stats.drawString("Remaining Blocks:", leftCol, y);
        canvas_stats.drawString(data.remainingBlocks, rightCol, y);
        
        // Draw halving progress bar
        y += 50;
        int barWidth = M5PAPER_WIDTH - 40;
        int barX = 20;
        int filledWidth = (barWidth * data.progressPercent) / 100;
        canvas_stats.drawRect(barX, y, barWidth, 30, 15);  // White outline
        canvas_stats.fillRect(barX + 2, y + 2, filledWidth - 4, 26, 15);  // White fill
        
        canvas_stats.setTextDatum(MC_DATUM);
        canvas_stats.setTextSize(2);
        canvas_stats.setTextColor(0);  // Black text on white bar
        canvas_stats.drawString(String(data.progressPercent) + "%", barX + barWidth / 2, y + 15);
        canvas_stats.setTextColor(15);  // Reset to white
        
        // Push stats canvas below image
        canvas_stats.pushCanvas(0, IMAGE_HEIGHT, UPDATE_MODE_DU);
        lastStatsUpdate = millis();
        prev_blockHeight = data.blockHeight;
        prev_btcPrice = data.btcPrice;
    }
}

void m5paper_PoolStatsScreen(unsigned long mElapsed)
{
    checkForScreenChange();
    checkPrevScreenButton();
    
    TouchState_M5Paper touch = m5paper_checkTouch(m5paperDisplayDriver.current_cyclic_screen);
    if (touch.justReleased) {
        if (touch.buttonNumber == 1) {
            switchToPreviousScreen();
            return;
        } else if (touch.buttonNumber == 2) {
            switchToNextScreenTouch();
            return;
        }
    }
    
    pool_data poolData = getPoolData();
    clock_data clockData = getClockData(mElapsed);
    mining_data miningData = getMiningData(mElapsed);
    
    // Get battery percentage
    int batteryPct = getBatteryPercentage();
    
    // Full refresh every 60 seconds or on first load
    static String prev_workersHash = "";
    bool needFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS) || 
                           (prev_workersHash.isEmpty());
    
    if (needFullRefresh) {
        // Full page refresh
        canvas_page.fillCanvas(0);  // Black background
        
        // Draw image at top (540x240)
        draw_grayscale_image(canvas_page, setupModeScreen_gray, 
                           setupModeScreen_gray_width, setupModeScreen_gray_height);
        
        // STATUS BAR - Below image with time, temp, and battery
        canvas_page.setTextColor(15);  // White text
        canvas_page.setTextDatum(TL_DATUM);
        canvas_page.setTextSize(2);
        int statusY = IMAGE_HEIGHT + 5;
        
        // Left side: Time
        canvas_page.drawString(clockData.currentTime, 10, statusY);
        
        // Center: Temperature with label
        canvas_page.setTextDatum(TC_DATUM);
        String tempLabel = "Temp: " + miningData.temp;
        canvas_page.drawString(tempLabel, M5PAPER_WIDTH / 2, statusY);
        
        // Right side: Battery with rounded rectangle
        canvas_page.setTextDatum(TR_DATUM);
        int battX = M5PAPER_WIDTH - 10;
        int battY = statusY;
        int battW = 70;
        int battH = 24;
        
        // Draw battery rounded rectangle
        canvas_page.drawRoundRect(battX - battW, battY, battW, battH, 5, 15);
        canvas_page.setTextDatum(MC_DATUM);
        String battText = String(batteryPct) + "%";
        canvas_page.drawString(battText, battX - battW/2, battY + battH/2);
        
        // Draw separator line below status bar
        int separatorY = statusY + 28;
        canvas_page.drawLine(10, separatorY, M5PAPER_WIDTH - 10, separatorY, 15);
        
        // Pool Stats Title
        canvas_page.setTextDatum(TC_DATUM);
        int y = IMAGE_HEIGHT + 60;
        canvas_page.setTextSize(4);
        canvas_page.drawString("POOL STATS", M5PAPER_WIDTH / 2, y);
        
        y += 70;
        
        // Pool data in two columns
        canvas_page.setTextSize(2);
        canvas_page.setTextDatum(TL_DATUM);
        int leftCol = 20;
        int rightCol = 280;
        
        canvas_page.drawString("Workers Count:", leftCol, y);
        canvas_page.drawString(String(poolData.workersCount), rightCol, y);
        
        y += 50;
        canvas_page.drawString("Total Hash Rate:", leftCol, y);
        canvas_page.drawString(poolData.workersHash + "H/s", rightCol, y);
        
        y += 50;
        canvas_page.drawString("Best Difficulty:", leftCol, y);
        canvas_page.drawString(poolData.bestDifficulty, rightCol, y);
        
        y += 50;
        canvas_page.drawString("Your Shares:", leftCol, y);
        canvas_page.drawString(miningData.completedShares, rightCol, y);
        
        y += 50;
        canvas_page.drawString("Valid Blocks:", leftCol, y);
        canvas_page.drawString(miningData.valids, rightCol, y);
        
#ifndef M5PAPER_DISABLE_TOUCH
        int btnY = M5PAPER_HEIGHT - 80;
        draw_outline_button(canvas_page, 20, btnY, 150, 60, 15, "Prev");
        draw_outline_button(canvas_page, 195, btnY, 150, 60, 15, "Next");
        draw_outline_button(canvas_page, 370, btnY, 150, 60, 15, "Menu");
#endif
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
        prev_workersHash = poolData.workersHash;
    }
    else if (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS) {
        // Partial update - update only the dynamic text area
        canvas_stats.fillCanvas(0);  // Black background
        canvas_stats.setTextColor(15);  // White text
        
        // STATUS BAR
        canvas_stats.setTextDatum(TL_DATUM);
        canvas_stats.setTextSize(2);
        int statusY = 5;
        
        // Left side: Time
        canvas_stats.drawString(clockData.currentTime, 10, statusY);
        
        // Center: Temperature with label
        canvas_stats.setTextDatum(TC_DATUM);
        String tempLabel = "Temp: " + miningData.temp;
        canvas_stats.drawString(tempLabel, M5PAPER_WIDTH / 2, statusY);
        
        // Right side: Battery with rounded rectangle
        canvas_stats.setTextDatum(TR_DATUM);
        int battX = M5PAPER_WIDTH - 10;
        int battY = statusY;
        int battW = 70;
        int battH = 24;
        
        // Draw battery rounded rectangle
        canvas_stats.drawRoundRect(battX - battW, battY, battW, battH, 5, 15);
        canvas_stats.setTextDatum(MC_DATUM);
        String battText = String(batteryPct) + "%";
        canvas_stats.drawString(battText, battX - battW/2, battY + battH/2);
        
        // Draw separator line below status bar
        int separatorY = statusY + 28;
        canvas_stats.drawLine(10, separatorY, M5PAPER_WIDTH - 10, separatorY, 15);
        
        // Pool Stats Title
        canvas_stats.setTextDatum(TC_DATUM);
        int y = 60;
        canvas_stats.setTextSize(4);
        canvas_stats.drawString("POOL STATS", M5PAPER_WIDTH / 2, y);
        
        y += 70;
        
        // Pool data
        canvas_stats.setTextSize(2);
        canvas_stats.setTextDatum(TL_DATUM);
        int leftCol = 20;
        int rightCol = 280;
        
        canvas_stats.drawString("Workers Count:", leftCol, y);
        canvas_stats.drawString(String(poolData.workersCount), rightCol, y);
        
        y += 50;
        canvas_stats.drawString("Total Hash Rate:", leftCol, y);
        canvas_stats.drawString(poolData.workersHash + "H/s", rightCol, y);
        
        y += 50;
        canvas_stats.drawString("Best Difficulty:", leftCol, y);
        canvas_stats.drawString(poolData.bestDifficulty, rightCol, y);
        
        y += 50;
        canvas_stats.drawString("Your Shares:", leftCol, y);
        canvas_stats.drawString(miningData.completedShares, rightCol, y);
        
        y += 50;
        canvas_stats.drawString("Valid Blocks:", leftCol, y);
        canvas_stats.drawString(miningData.valids, rightCol, y);
        
        // Push stats canvas below image
        canvas_stats.pushCanvas(0, IMAGE_HEIGHT, UPDATE_MODE_DU);
        lastStatsUpdate = millis();
        prev_workersHash = poolData.workersHash;
    }
}

void m5paper_AnimateCurrentScreen(unsigned long frame)
{
    // E-ink doesn't support animation
}

void m5paper_DoLedStuff(unsigned long frame)
{
    // M5Paper has no LED
}

// Create the cyclic screens array
CyclicScreenFunction m5paper_screens[] = {
    m5paper_MinerScreen,
    m5paper_GlobalHashScreen,
    m5paper_PoolStatsScreen
};

// Create the DisplayDriver structure
DisplayDriver m5paperDisplayDriver = {
    m5paper_Init,
    m5paper_AlternateScreenState,
    m5paper_AlternateRotation,
    m5paper_LoadingScreen,
    m5paper_SetupScreen,
    m5paper_screens,
    m5paper_AnimateCurrentScreen,
    m5paper_DoLedStuff,
    SCREENS_ARRAY_SIZE(m5paper_screens),
    0,
    M5PAPER_WIDTH,
    M5PAPER_HEIGHT
};

#endif // M5PAPER_DISPLAY
