#pragma once

/***********************config*************************/
#define LCD_USB_QSPI_DREVER   1

#define SPI_FREQUENCY         75000000
#define TFT_SPI_MODE          SPI_MODE0
#define TFT_SPI_HOST          SPI2_HOST

#define EXAMPLE_LCD_H_RES     536
#define EXAMPLE_LCD_V_RES     240
#define LVGL_LCD_BUF_SIZE     (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)

/***********************config*************************/

#define TFT_WIDTH             240
#define TFT_HEIGHT            536
#define SEND_BUF_SIZE         (0x4000) //(LCD_WIDTH * LCD_HEIGHT + 8) / 10

#define TFT_TE                9
#define TFT_SDO               8

#define TFT_DC                7
#define TFT_RES               17
#define TFT_CS                6
#define TFT_MOSI              18
#define TFT_SCK               47

#define TFT_QSPI_CS           6
#define TFT_QSPI_SCK          47
#define TFT_QSPI_D0           18
#define TFT_QSPI_D1           7
#define TFT_QSPI_D2           48
#define TFT_QSPI_D3           5
#define TFT_QSPI_RST          17

#define PIN_LED               38
#define PIN_BAT_VOLT          4

#define PIN_BUTTON_1          0
#define PIN_BUTTON_2          21