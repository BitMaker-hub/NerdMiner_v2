// ST7789P3 135 x 240 display - M5Stack StickS3 (ESP32-S3-PICO)
#define USER_SETUP_ID 216

#define ST7789_DRIVER

#define TFT_WIDTH  135
#define TFT_HEIGHT 240

#define TFT_INVERSION_ON
#define TFT_BACKLIGHT_ON 1

#define TFT_MISO   -1
#define TFT_MOSI   39
#define TFT_SCLK   40
#define TFT_CS     41
#define TFT_DC     45
#define TFT_RST    21
#define TFT_BL     38

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY   20000000
