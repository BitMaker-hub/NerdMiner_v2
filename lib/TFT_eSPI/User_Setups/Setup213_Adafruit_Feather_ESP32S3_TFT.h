// Setup for the ADAFRUIT FEATHER ESP32S3 TFT
#define USER_SETUP_ID 213

// See SetupX_Template.h for all options available

#define ST7789_DRIVER
// #define TFT_SDA_READ   // Display has a bidirectional SDA pin

#define TFT_WIDTH  135
#define TFT_HEIGHT 240

#define CGRAM_OFFSET      // Library will add offsets required

/* TFT Display
On the front of the board is a color 1.14" IPS TFT with 240x135 pixels.
It's a bright and colorful display with ST7789 chipset that can be viewed at any angle.

There is a power pin that must be pulled high for the display to work.
This is done automatically by CircuitPython and Arduino.
The pin is available in CircuitPython and in Arduino as TFT_I2C_POWER or 21.

If you run into I2C or TFT power issues on Arduino, ensure you are using the latest Espressif board support package.
If you are still having issues, you may need to manually pull the pin HIGH in your code.
*/
// #define TFT_MISO 37
#define TFT_MISO            -1
#define TFT_MOSI            35
#define TFT_SCLK            36
#define TFT_CS              7
#define TFT_DC              39
#define TFT_RST             40

#define TFT_BL              45 // Display backlight control pin

#define TFT_BACKLIGHT_ON HIGH  // HIGH or LOW are options

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

//#define SPI_FREQUENCY  27000000
  #define SPI_FREQUENCY  40000000


#define SPI_READ_FREQUENCY  6000000 // 6 MHz is the maximum SPI read speed for the ST7789V
