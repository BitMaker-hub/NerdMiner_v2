#ifndef _ESP32_2432S024R
#define _ESP32_2432S024R

#define PIN_BUTTON_1 0
//#define PIN_BUTTON_2 22  // Not used
// #define PIN_ENABLE5V 21  // Not used
#define LED_PIN      4   // Red pin
#define LED_PIN_G    17  // Green pin (swapped with Blue for 2.4" variant)
#define LED_PIN_B    16  // Blue pin (swapped with Green for 2.4" variant)

// Pin defines for the SD card interface
// This is working for both, ESP32 2432S028R and ESP 2432S024R boards 
// --------------------------------------
// use SPI interface
// (default SPI unit provided by <SPI.h>)
// setup SPI pins.

#define SDSPI_CS    5
#define SDSPI_CLK   18
#define SDSPI_MOSI  23
#define SDSPI_MISO  19

#define PIN_POWER_ON -1 // Not used
#define PIN_SD_EN    -1 // Not used

// Touch screen pins for ESP32-2432S024R (Resistive touch variant)
// Share SPI bus with display, only CS pin is different
#define TOUCH_CS    33  // Touch CS pin (different from TFT_CS=15)
#define TOUCH_CLK   14  // Touch clock pin (same as TFT_SCLK)
#define TOUCH_MISO  12  // Touch MISO pin (same as TFT_MISO)
#define TOUCH_MOSI  13  // Touch MOSI pin (same as TFT_MOSI)
#define TOUCH_IRQ   36  // Touch IRQ pin

// calls api to retrieve worker metrics
#define SCREEN_WORKERS_ENABLE (1)

#endif