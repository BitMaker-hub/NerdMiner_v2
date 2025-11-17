#include "displayDriver.h"

#ifdef M5PAPER_DISPLAY

#include <M5EPD.h>
#include "m5paperDisplayDriver.h"
#include "version.h"
#include "monitor.h"
#include "media/images_m5paper_gray.h"

#define M5PAPER_WIDTH 540
#define M5PAPER_HEIGHT 960

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
const unsigned long TOUCH_CHECK_INTERVAL_MS = 100;      // Check touch every 100ms

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
// Images are 536x240 landscape, displayed at top of screen
static void draw_grayscale_image(M5EPD_Canvas &canvas, const uint8_t* img_data, 
                                  uint16_t img_width, uint16_t img_height)
{
    // Center horizontally on the screen
    int x_offset = (M5PAPER_WIDTH - img_width) / 2;
    int y_offset = 0;
    
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
static void draw_outline_button(M5EPD_Canvas &c, int x, int y, int w, int h, int r, const char *label)
{
    const int stroke = 2;
    for (int s = 0; s < stroke; s++) {
        c.drawRoundRect(x + s, y + s, w - 2 * s, h - 2 * s, r, 15);
    }

    c.setTextDatum(MC_DATUM);
    c.setTextSize(2);
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
    static bool wasTouched = false;
    static int lastButton = 0;
    static unsigned long lastReleaseTime = 0;
    
    TouchState_M5Paper state = {false, false, 0, 0, 0};
    
    // Rate limiting
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
    
    // Define button areas - buttons higher from bottom
    // Touch coordinates in landscape: X = 0-540, Y = 0-960
    if (finger.y > 820 && finger.y < 900) {
        int buttonPressed = 0;
        
        // All screens have navigation buttons at bottom
        const int btnW = 120;
        const int spacing = 30;
        
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
    
    return state;
}

void m5paper_Init(void)
{
    M5.begin();
    M5.EPD.SetRotation(90);  // Landscape orientation (540x960)
    M5.TP.SetRotation(90);   // Touch rotation must match display
    M5.EPD.Clear(true);      // Clear with full refresh
    M5.RTC.begin();
    
    // Initialize previous screen button (GPIO 37)
    pinMode(PREV_SCREEN_BUTTON, INPUT_PULLUP);
    
    // Create canvases
    canvas_page.createCanvas(M5PAPER_WIDTH, M5PAPER_HEIGHT);
    canvas_page.setTextSize(3);
    
    canvas_stats.createCanvas(500, 120);  // Smaller canvas for dynamic updates
    canvas_stats.setTextSize(3);
    
    Serial.println("M5Paper E-ink display initialized (540x960 landscape)");
    Serial.println("Touch enabled: GT911 capacitive touch controller");
    Serial.println("GPIO 39 = Next Screen, GPIO 37 = Previous Screen");
    Serial.println("Touch: [Prev] [Next] [Menu] buttons at screen bottom");
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
    canvas_page.fillCanvas(0);  // White background
    
    // Draw init screen image (536x240 landscape)
    draw_grayscale_image(canvas_page, initScreen_gray, 
                        initScreen_gray_width, initScreen_gray_height);
    
    canvas_page.setTextColor(15);  // Black text
    
    // No additional text needed - image contains all info
    
    canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);  // Full quality refresh
    lastFullRefresh = millis();
}

void m5paper_SetupScreen(void)
{
    canvas_page.fillCanvas(0);
    
    // Draw setup mode image (536x240 landscape)
    draw_grayscale_image(canvas_page, setupModeScreen_gray, 
                        setupModeScreen_gray_width, setupModeScreen_gray_height);
    
    canvas_page.setTextColor(15);
    
    // No additional text needed - image contains all info
    
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
            // Previous button
            switchToPreviousScreen();
            return;  // Exit to refresh on next cycle
        } else if (touch.buttonNumber == 2) {
            // Next button
            switchToNextScreenTouch();
            return;
        }
        // Button 3 (Menu) - reserved for future use
    }
    
    mining_data data = getMiningData(mElapsed);
    
    // Check if we need a full page refresh (every 60 seconds or first time)
    bool needsFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS) || 
                            (prev_shares == "");
    
    if (needsFullRefresh) {
        // Full page refresh with static content
        canvas_page.fillCanvas(0);
        
        // Draw background image at top (536x240 landscape)
        draw_grayscale_image(canvas_page, MinerScreen_gray, 
                           MinerScreen_gray_width, MinerScreen_gray_height);
        
        canvas_page.setTextColor(15);
        
        // Top bar - Time and Battery (on top of image)
        canvas_page.setTextSize(2);
        canvas_page.drawString(data.currentTime.c_str(), 10, 10);
        int battPct = getBatteryPercentage();
        String battStr = "Batt: " + String(battPct) + "%";
        canvas_page.drawString(battStr.c_str(), M5PAPER_WIDTH - 140, 10);
        
        // Draw static labels below the image
        int dataStartY = MinerScreen_gray_height + 20;
        canvas_page.setTextSize(3);
        canvas_page.drawString("Hash Rate:", 50, dataStartY);
        canvas_page.drawString("Total Hashes:", 50, dataStartY + 60);
        canvas_page.drawString("Shares:", 50, dataStartY + 120);
        canvas_page.drawString("Best Diff:", 50, dataStartY + 180);
        canvas_page.drawString("Valid Blocks:", 50, dataStartY + 240);
        canvas_page.drawString("Templates:", 50, dataStartY + 300);
        canvas_page.drawString("Time Mining:", 50, dataStartY + 360);
        canvas_page.drawString("Temp:", 50, dataStartY + 420);
        
        // Draw touch buttons at bottom
        const int btnW = 120;
        const int btnH = 50;
        const int spacing = 30;
        const int totalW = btnW * 3 + spacing * 2;
        const int startX = (M5PAPER_WIDTH - totalW) / 2;
        const int y = 830;
        
        draw_outline_button(canvas_page, startX, y, btnW, btnH, 8, "Prev");
        draw_outline_button(canvas_page, startX + btnW + spacing, y, btnW, btnH, 8, "Next");
        draw_outline_button(canvas_page, startX + 2 * (btnW + spacing), y, btnW, btnH, 8, "Menu");
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
    }
    
    // Update dynamic stats (only if changed and not too frequent)
    bool dataChanged = (data.currentHashRate != prev_hashrate ||
                       data.completedShares != prev_shares);
    
    if (dataChanged && (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS)) {
        // Data starts below the image
        int dataStartY = MinerScreen_gray_height + 20;
        
        // Prepare all values first to avoid flickering
        String values[8];
        int yPositions[8] = {
            dataStartY, 
            dataStartY + 60, 
            dataStartY + 120, 
            dataStartY + 180, 
            dataStartY + 240, 
            dataStartY + 300, 
            dataStartY + 360, 
            dataStartY + 420
        };
        
        values[0] = data.currentHashRate + " KH/s";
        values[1] = data.totalMHashes + " MH";
        values[2] = data.completedShares;
        values[3] = data.bestDiff;
        values[4] = data.valids;
        values[5] = data.templates;
        values[6] = data.timeMining;
        values[7] = data.temp;
        
        // Update all values in a single batch to prevent flickering
        canvas_stats.fillCanvas(0);
        canvas_stats.setTextColor(15);
        canvas_stats.setTextSize(3);
        
        for (int i = 0; i < 8; i++) {
            // Clear and draw each value to canvas
            canvas_stats.fillCanvas(0);
            canvas_stats.drawString(values[i], 0, 0);
            canvas_stats.pushCanvas(300, yPositions[i], UPDATE_MODE_DU4);
        }
        
        // Store current values
        prev_hashrate = data.currentHashRate;
        prev_shares = data.completedShares;
        lastStatsUpdate = millis();
    }
    
    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                  data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
}

void m5paper_ClockScreen(unsigned long mElapsed)
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
    
    clock_data data = getClockData(mElapsed);
    
    bool needsFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS);
    
    if (needsFullRefresh) {
        canvas_page.fillCanvas(0);
        canvas_page.setTextDatum(TL_DATUM);
        
        // Draw background image at top (536x240 landscape)
        draw_grayscale_image(canvas_page, minerClockScreen_gray, 
                           minerClockScreen_gray_width, minerClockScreen_gray_height);
        
        canvas_page.setTextColor(15);
        
        // Battery on top of image
        canvas_page.setTextSize(2);
        int battPct = getBatteryPercentage();
        String battStr = "Batt: " + String(battPct) + "%";
        canvas_page.drawString(battStr.c_str(), M5PAPER_WIDTH - 140, 10);
        
        // Data labels below image
        int dataStartY = minerClockScreen_gray_height + 20;
        int labelX = 50;
        canvas_page.setTextSize(3);
        canvas_page.drawString("Date:", labelX, dataStartY);
        canvas_page.drawString("Hash:", labelX, dataStartY + 80);
        canvas_page.drawString("BTC Price:", labelX, dataStartY + 160);
        canvas_page.drawString("Block:", labelX, dataStartY + 240);
        canvas_page.drawString("Shares:", labelX, dataStartY + 320);
        
        // Draw touch buttons at bottom
        const int btnW = 120;
        const int btnH = 50;
        const int spacing = 30;
        const int totalW = btnW * 3 + spacing * 2;
        const int startX = (M5PAPER_WIDTH - totalW) / 2;
        const int y = 830;
        
        draw_outline_button(canvas_page, startX, y, btnW, btnH, 8, "Prev");
        draw_outline_button(canvas_page, startX + btnW + spacing, y, btnW, btnH, 8, "Next");
        draw_outline_button(canvas_page, startX + 2 * (btnW + spacing), y, btnW, btnH, 8, "Menu");
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
    }
    
    // Update dynamic data
    if (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS) {
        int dataStartY = minerClockScreen_gray_height + 20;
        int valueX = 300;
        
        canvas_stats.fillCanvas(0);
        canvas_stats.setTextColor(15);
        canvas_stats.setTextSize(3);
        canvas_stats.setTextDatum(TL_DATUM);
        
        // Date
        canvas_stats.drawString(data.currentDate, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY, UPDATE_MODE_DU4);
        
        // Hash rate
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.currentHashRate + " KH/s", 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 80, UPDATE_MODE_DU4);
        
        // BTC Price
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString("$" + data.btcPrice, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 160, UPDATE_MODE_DU4);
        
        // Block height
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.blockHeight, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 240, UPDATE_MODE_DU4);
        
        // Shares
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.completedShares, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 320, UPDATE_MODE_DU4);
        
        lastStatsUpdate = millis();
    }
}

void m5paper_GlobalHashScreen(unsigned long mElapsed)
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
    
    coin_data data = getCoinData(mElapsed);
    
    bool needsFullRefresh = (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS);
    
    if (needsFullRefresh) {
        canvas_page.fillCanvas(0);
        canvas_page.setTextColor(15);
        canvas_page.setTextDatum(TL_DATUM);
        
        // Draw background image at top (536x240 landscape)
        draw_grayscale_image(canvas_page, globalHashScreen_gray, 
                           globalHashScreen_gray_width, globalHashScreen_gray_height);
        
        // Battery on top of image
        canvas_page.setTextSize(2);
        int battPct = getBatteryPercentage();
        String battStr = "Batt: " + String(battPct) + "%";
        canvas_page.drawString(battStr.c_str(), M5PAPER_WIDTH - 140, 10);
        
        // Data labels below image
        int dataStartY = globalHashScreen_gray_height + 20;
        int labelX = 20;
        canvas_page.setTextSize(2);
        canvas_page.drawString("BTC Price:", labelX, dataStartY);
        canvas_page.drawString("Block:", labelX, dataStartY + 50);
        canvas_page.drawString("Global Hash:", labelX, dataStartY + 100);
        canvas_page.drawString("Difficulty:", labelX, dataStartY + 150);
        canvas_page.drawString("Halving:", labelX, dataStartY + 200);
        canvas_page.drawString("Blocks:", labelX, dataStartY + 250);
        canvas_page.drawString("Fee sat/vB:", labelX, dataStartY + 300);
        canvas_page.drawString("Your Hash:", labelX, dataStartY + 350);
        canvas_page.drawString("Shares:", labelX, dataStartY + 400);
        
        // Draw touch buttons at bottom
        const int btnW = 120;
        const int btnH = 50;
        const int spacing = 30;
        const int totalW = btnW * 3 + spacing * 2;
        const int startX = (M5PAPER_WIDTH - totalW) / 2;
        const int y = 830;
        
        draw_outline_button(canvas_page, startX, y, btnW, btnH, 8, "Prev");
        draw_outline_button(canvas_page, startX + btnW + spacing, y, btnW, btnH, 8, "Next");
        draw_outline_button(canvas_page, startX + 2 * (btnW + spacing), y, btnW, btnH, 8, "Menu");
        
        canvas_page.pushCanvas(0, 0, UPDATE_MODE_GC16);
        lastFullRefresh = millis();
    }
    
    // Update dynamic data
    if (millis() - lastStatsUpdate > STATS_UPDATE_INTERVAL_MS) {
        int dataStartY = globalHashScreen_gray_height + 20;
        int valueX = 300;
        
        canvas_stats.fillCanvas(0);
        canvas_stats.setTextColor(15);
        canvas_stats.setTextSize(2);
        canvas_stats.setTextDatum(TL_DATUM);
        
        // BTC Price
        canvas_stats.drawString("$" + data.btcPrice, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY, UPDATE_MODE_DU4);
        
        // Block height
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.blockHeight, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 50, UPDATE_MODE_DU4);
        
        // Global hash rate
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.globalHashRate, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 100, UPDATE_MODE_DU4);
        
        // Network difficulty
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.netwrokDifficulty, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 150, UPDATE_MODE_DU4);
        
        // Halving progress
        canvas_stats.fillCanvas(0);
        char progress[32];
        snprintf(progress, sizeof(progress), "%.2f%%", data.progressPercent);
        canvas_stats.drawString(progress, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 200, UPDATE_MODE_DU4);
        
        // Remaining blocks
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.remainingBlocks, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 250, UPDATE_MODE_DU4);
        
        // Half hour fee
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.halfHourFee, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 300, UPDATE_MODE_DU4);
        
        // Your hash rate
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.currentHashRate + " KH/s", 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 350, UPDATE_MODE_DU4);
        
        // Your shares
        canvas_stats.fillCanvas(0);
        canvas_stats.drawString(data.completedShares, 0, 0);
        canvas_stats.pushCanvas(valueX, dataStartY + 400, UPDATE_MODE_DU4);
        
        lastStatsUpdate = millis();
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
    m5paper_ClockScreen,
    m5paper_GlobalHashScreen
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
