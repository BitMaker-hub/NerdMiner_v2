// =============================================================================
//  Guition JC3248W535 — 3.5" 320x480 QSPI capacitive touch
// =============================================================================
//  ESP32-S3 N16R8  (16MB flash, 8MB OPI PSRAM)
//  Display:  AXS15231B over QSPI (CS=45, SCK=47, D0=21, D1=48, D2=40, D3=39)
//  Touch:    AXS-touch over I2C @ 0x3B (SDA=4, SCL=8)
//  Backlight: GPIO 1 (active HIGH)
//
//  Pins confirmed against the F1ATB working sample:
//  https://github.com/AudunKodehode/JC3248W535EN-Touch-LCD
// =============================================================================
#ifndef _JC3248W535_H
#define _JC3248W535_H

#define JC3248W535_DISPLAY

// ---------------- LCD (AXS15231B QSPI) ---------------------------------------
#define LCD_CS     45
#define LCD_SCK    47
#define LCD_D0     21
#define LCD_D1     48
#define LCD_D2     40
#define LCD_D3     39
#define LCD_BL      1     // backlight, active HIGH

// ---------------- Touch (AXS controller, I2C) --------------------------------
#define TOUCH_SDA       4
#define TOUCH_SCL       8
#define TOUCH_INT      11       // ⚠ shared with SDMMC_CMD — see SD card note
#define TOUCH_RST      12       // ⚠ shared with SDMMC_CLK — see SD card note
#define TOUCH_I2C_ADDR  0x3B
#define TOUCH_I2C_HZ    400000

// ---------------- SD card (SDMMC 1-bit mode) ---------------------------------
// Board wiring per JC3248W535 silkscreen, matches hd2_macropad reference.
// 1-bit SDMMC needs only CLK + CMD + D0 — D1/D2/D3 unused.
//
// IMPORTANT: SDMMC_CLK shares GPIO 12 with TOUCH_RST, and SDMMC_CMD
// shares GPIO 11 with TOUCH_INT. The display driver does NOT call
// pinMode(TOUCH_RST,OUTPUT) or pinMode(TOUCH_INT,INPUT_PULLUP) so the
// SDMMC peripheral can drive these pins. The AXS-touch chip self-resets
// at power-on (per hd2_macropad reference) and we poll touch via I2C
// (not interrupt), so neither shared signal is needed for touch to work.
#define SDMMC_CLK      12
#define SDMMC_CMD      11
#define SDMMC_D0       13
#define SD_FREQUENCY   20000    // 20 MHz (matches T-HMI's reliability fix)
#define SDMMC_1BIT_FIX (1)      // tells NerdMinerV2.ino.cpp to defer SD
                                // init until after main subsystems are up

// ---------------- NerdMiner expected pins ------------------------------------
// LED — board has no dedicated user LED; reuse backlight so doLedStuff() does
// something visible (briefly dims the screen during "Connecting" blinks).
// If you wire an external LED, redefine LED_PIN to its GPIO.
//  #define LED_PIN        LCD_BL
#define ACTIVE_LED     HIGH
#define INACTIVE_LED   LOW

// Buttons — JC3248W535 has no physical user buttons exposed to the firmware.
// Touch is used instead (see jc3248w535DisplayDriver.cpp).
// Define PIN_BUTTON_1 only if you wire one to a free GPIO.
//#define PIN_BUTTON_1   0

// Screen size in landscape orientation (matches setRotation(1))
#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  320

// ---------------- Optional NerdMiner subsystem enables -----------------------
// SCREEN_WORKERS_ENABLE — adds the pool worker API call so the bottom POOL
// panel can show real workers count / total worker hashrate / best difficulty.
// Without this, those fields stay zeros.
#define SCREEN_WORKERS_ENABLE (1)

// SCREEN_FEES_ENABLE — adds the extra mempool fee fields (fastestFee,
// hourFee, economyFee, minimumFee) to coin_data so the FEES bottom panel
// can show the full T-HMI-style 3-column fee view instead of just one value.
//
// NOTE: This also requires the matching #if guard in src/monitor.h to allow
// JC3248W535 to access those fields (added in v0.7.2).
#define SCREEN_FEES_ENABLE (1)

#endif // _JC3248W535_H
