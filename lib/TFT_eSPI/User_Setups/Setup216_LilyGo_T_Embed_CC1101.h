// ST7789 170 x 320 display
// LilyGo T-Embed **CC1101** variant - this board's LCD pins are different
// from the plain (non-CC1101) T-Embed, see:
// https://github.com/Xinyuan-LilyGO/T-Embed-CC1101#4-pins
#define USER_SETUP_ID 216

#define ST7789_DRIVER     // Configure all registers

#define TFT_WIDTH  170
#define TFT_HEIGHT 320

#define TFT_INVERSION_ON
#define TFT_BACKLIGHT_ON 1

#define TFT_BL     21   // LED back-light (DISPLAY_BL)
#define TFT_MISO   -1   // Not connected
#define TFT_MOSI    9   // DISPLAY_MOSI
#define TFT_SCLK   11   // DISPLAY_SCLK
#define TFT_CS     41   // DISPLAY_CS
#define TFT_DC     16   // DISPLAY_DC
#define TFT_RST    -1   // DISPLAY_RST - not wired to a GPIO on this board,
                         // display uses power-on reset only

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
