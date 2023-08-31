#define USER_SETUP_ID 300

#define ST7735_DRIVER

#define TFT_WIDTH  80
#define TFT_HEIGHT 160


#define TFT_RST             1
#define TFT_MISO            -1
#define TFT_MOSI            3
#define TFT_SCLK            5
#define TFT_CS              4
#define TFT_DC              2
// #define TFT_BL              38
// #define TFT_BACKLIGHT_ON LOW

#define ST7735_GREENTAB160x80 // For 160 x 80 display (BGR, inverted, 26 offset)

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
#define SMOOTH_FONT

#define SPI_FREQUENCY  50000000 // Actually sets it to 26.67MHz = 80/3

// #define SPI_FREQUENCY  40000000 // Maximum to use SPIFFS
// #define SPI_FREQUENCY  80000000

#define TFT_RGB_ORDER TFT_BGR