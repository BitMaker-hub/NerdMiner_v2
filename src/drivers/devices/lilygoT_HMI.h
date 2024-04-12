#ifndef _NERD_MINER_T_HMI_H
#define _NERD_MINER_T_HMI_H

#define T_HMI_DISPLAY

#define PWR_EN_PIN  (10)
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


// lcd

#define LCD_DATA0_PIN (48)
#define LCD_DATA1_PIN (47)
#define LCD_DATA2_PIN (39)
#define LCD_DATA3_PIN (40)
#define LCD_DATA4_PIN (41)
#define LCD_DATA5_PIN (42)
#define LCD_DATA6_PIN (45)
#define LCD_DATA7_PIN (46)

#define PCLK_PIN      (8)
#define CS_PIN        (6)
#define DC_PIN        (7)
#define RST_PIN       (-1)
#define BK_LIGHT_PIN  (38)
#define BK_LIGHT_LEVEL (1)
#define LED_PIN       (8)

// sd card
// 1-bit SD_MMC
#ifdef  DEFINE_1BIT

#define SDMMC_CLK  (12)
#define SDMMC_CMD  (11)
#define SDMMC_D0   (13)

#else

#define SDSPI_MISO  (13)
#define SDSPI_MOSI  (11)
#define SDSPI_CLK   (12)

#endif

#ifndef TFT_BL
// XXX - defined in User_Setups/Setup207_LilyGo_T_HMI.h:37
#define TFT_BL        (38) // LED back-light
#endif


#endif
