#ifndef _ESP32CAM_H_
#define _ESP32CAM_H_

#define PIN_BUTTON_1 0
#define LED_PIN 33

#define NO_DISPLAY

// SDMMC interface: 1-bit mode (might cause issues):
#define SDMMC_CLK 14
#define SDMMC_CMD 15
#define SDMMC_D0 2
// additional defines to enable 4-bit mode
#define SDMMC_D1 4
#define SDMMC_D2 12
#define SDMMC_D3 13

#endif // _ESP32_CAM_H_
