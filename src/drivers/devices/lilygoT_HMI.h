#ifndef _NERD_MINER_T_HMI_H
#define _NERD_MINER_T_HMI_H

#define T_HMI_DISPLAY

#define PWR_EN_PIN  (10)
#define PWR_ON_PIN  (14)
#define BAT_ADC_PIN (5)
#define BUTTON1_PIN (0)
#define BUTTON2_PIN (21)

#define PIN_BUTTON_1 BUTTON1_PIN
// #define PIN_BUTTON_2 BUTTON2_PIN


// touch screen
#define TOUCHSCREEN_SCLK_PIN (1)
#define TOUCHSCREEN_MISO_PIN (4)
#define TOUCHSCREEN_MOSI_PIN (3)
#define TOUCHSCREEN_CS_PIN   (2)
#define TOUCHSCREEN_IRQ_PIN  (9)
#define TOUCH_CLK   TOUCHSCREEN_SCLK_PIN
#define TOUCH_MISO  TOUCHSCREEN_MISO_PIN
#define TOUCH_MOSI  TOUCHSCREEN_MOSI_PIN
#define ETOUCH_CS   TOUCHSCREEN_CS_PIN


// lcd
#define PCLK_PIN      (8)
#define CS_PIN        (6)
#define DC_PIN        (7)
#define RST_PIN       (-1)
#define BK_LIGHT_PIN  (38)
#define LED_PIN       (8)

#ifndef TFT_BL
// XXX - defined in User_Setups/Setup207_LilyGo_T_HMI.h:37
#define TFT_BL        (38) // LED back-light
#endif


#endif
