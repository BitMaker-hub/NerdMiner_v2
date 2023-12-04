#ifndef _ESP32CAM_H_
#define _ESP32CAM_H_

#define PIN_BUTTON_1 0
#define LED_PIN 33

#define NO_DISPLAY

// example how to configure SD card.
// if you would define everything, 
// to select 1 bit mode, make sure SDMMC_D1-3 are undefined
// to use spi mode, make sure all SDMMC_x pins are undefined

/*
// use SDMMC interface:
// 1-bit mode (might cause issues):
#define SDMMC_CLK 14
#define SDMMC_CMD 15
#define SDMMC_D0 2
// additional defines to enable 4-bit mode
#define SDMMC_D1 4
#define SDMMC_D2 12
#define SDMMC_D3 13
*/

// use SPI interface
// (default SPI unit provided by <SPI.h>)
// setup SPI pins.
#define SDSPI_CS    13
// The following pins can be retreived from the TFT_eSPI lib, 
// if a display that is using it is activated.
#define SDSPI_CLK   14
#define SDSPI_MOSI  15
#define SDSPI_MISO  2

#endif // _ESP32_CAM_H_
