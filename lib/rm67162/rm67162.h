#pragma once

#include "stdint.h"
#include "pins_config.h"

#define TFT_MADCTL 0x36
#define TFT_MAD_MY 0x80
#define TFT_MAD_MX 0x40
#define TFT_MAD_MV 0x20
#define TFT_MAD_ML 0x10
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH 0x04
#define TFT_MAD_RGB 0x00

#define TFT_INVOFF 0x20
#define TFT_INVON 0x21

#define TFT_SCK_H digitalWrite(TFT_SCK, 1);
#define TFT_SCK_L digitalWrite(TFT_SCK, 0);
#define TFT_SDA_H digitalWrite(TFT_MOSI, 1);
#define TFT_SDA_L digitalWrite(TFT_MOSI, 0);

#define TFT_RES_H digitalWrite(TFT_RES, 1);
#define TFT_RES_L digitalWrite(TFT_RES, 0);
#define TFT_DC_H digitalWrite(TFT_DC, 1);
#define TFT_DC_L digitalWrite(TFT_DC, 0);
#define TFT_CS_H digitalWrite(TFT_CS, 1);
#define TFT_CS_L digitalWrite(TFT_CS, 0);

typedef struct
{
  uint8_t cmd;
  uint8_t data[4];
  uint8_t len;
} lcd_cmd_t;

void rm67162_init(void);

// Set the display window size
void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_setRotation(uint8_t r);
void lcd_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void lcd_fill(uint16_t xsta,
              uint16_t ysta,
              uint16_t xend,
              uint16_t yend,
              uint16_t color);
void lcd_PushColors(uint16_t x,
                    uint16_t y,
                    uint16_t width,
                    uint16_t high,
                    uint16_t *data);
void lcd_PushColors(uint16_t *data, uint32_t len);
void lcd_sleep();

void lcd_on();
void lcd_off();