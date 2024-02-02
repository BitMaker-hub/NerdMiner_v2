#ifndef _ESP32_2432S028R
#define _ESP32_2432S028R

#define PIN_BUTTON_1 0
//#define PIN_BUTTON_2 22  // Not used
#define PIN_ENABLE5V 21  // Not used
#define LED_PIN      4   // Red pin
#define LED_PIN_G    17  // Green pin

// Pin defines for the SD card interface
// This is working 
// --------------------------------------
// use SPI interface
// (default SPI unit provided by <SPI.h>)
// setup SPI pins.

#define SDSPI_CS    5
#define SDSPI_CLK   18
#define SDSPI_MOSI  23
#define SDSPI_MISO  19

#endif
