#ifndef _NERD_MINER_T_HMI_H
#define _NERD_MINER_T_HMI_H

#define T_HMI_DISPLAY

#define PWR_EN_PIN  (10)
#define PIN_ENABLE5V PWR_EN_PIN
#define PWR_ON_PIN  (14)
#define BAT_ADC_PIN (5)
#define BUTTON1_PIN (0)
#define BUTTON2_PIN (21)

#define PIN_BUTTON_1 (0)
//#define PIN_BUTTON_2 (21)

// touch screen
#define TOUCH_IRQ   (9)
#define TOUCH_CLK   (1)
#define TOUCH_MISO  (4)
#define TOUCH_MOSI  (3)
#define ETOUCH_CS   (2)

#define PCLK_PIN      (8)
#define CS_PIN        (6)
#define DC_PIN        (7)
#define RST_PIN       (-1)
#define BK_LIGHT_PIN  (38)
#define BK_LIGHT_LEVEL (1)
#define LED_PIN       (8)

// sd card
// 1-bit SD MMC
#define SDMMC_CLK  (12)
#define SDMMC_CMD  (11)
#define SDMMC_D0   (13)

#define TOUCH_ENABLE (1)
#define SDMMC_1BIT_FIX (1)
#define SD_FREQUENCY (20000)

// calls api to retrieve worker metrics
#define SCREEN_WORKERS_ENABLE (1)
// retrieve current btc fees data
#define SCREEN_FEES_ENABLE (1)

#ifndef TFT_BL
// XXX - defined in User_Setups/Setup207_LilyGo_T_HMI.h:37
#define TFT_BL        (38) // LED back-light
#endif

#endif
