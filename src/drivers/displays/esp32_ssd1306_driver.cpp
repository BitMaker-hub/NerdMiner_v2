#include "displayDriver.h"

#ifdef ESP32_SSD1306
#include "version.h"
#include "monitor.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "media/128x64_SSD1306.h"

#define WIDTH 128
#define HEIGHT 64

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, OLED_RESET);


void ssd1306_Display_Init(void)
{

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    // Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  display.clearDisplay();

  // Draw the splash screen
  display.drawBitmap(
      (display.width() - WIDTH) / 2,
      (display.height() - HEIGHT) / 2,
      logo_bmp, WIDTH, HEIGHT, 1);
  display.display();

  delay(2000);

  // Invert and restore display, pausing in-between (short animation)
  for (int i = 0; i <= 10; i++)
  {
    display.invertDisplay(true);
    delay(25);
    display.invertDisplay(false);
    delay(25);
  }

  // Clear the buffer
  display.clearDisplay();
}

void ssd1306_Display_AlternateScreenState(void)
{
  // tbd
}

void ssd1306_Display_AlternateRotation(void)
{
  display.setRotation((display.getRotation() + 2) % 4);
}

void ssd1306_Display_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  display.clearDisplay(); // paint it black :-)

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);
  // render.setFontColor(TFT_BLACK);

  display.print(data.currentHashRate.c_str());
  display.println(F(" kH/s"));
  display.setTextSize(1);
  display.println();

  // Valid Blocks
  display.setTextSize(2);
  display.print(data.valids.c_str());
  // display.setTextSize(1);
  display.println(F(" Blocks"));

  // Mining Time
  char timeMining[15];
  unsigned long secElapsed = millis() / 1000;
  int days = secElapsed / 86400;
  int hours = (secElapsed - (days * 86400)) / 3600;               // Number of seconds in an hour
  int mins = (secElapsed - (days * 86400) - (hours * 3600)) / 60; // Remove the number of hours and calculate the minutes.
  int secs = secElapsed - (days * 86400) - (hours * 3600) - (mins * 60);
  sprintf(timeMining, "%01d d %02d:%02d:%02d h", days, hours, mins, secs);
  display.setTextSize(1);
  display.println();
  display.println(String(timeMining).c_str());
  display.display();
}

// uint16_t osx=64, osy=64, omx=64, omy=64, ohx=64, ohy=64;  // Saved H, M, S x & y coords
void ssd1306_Display_ClockScreen(unsigned long mElapsed)
{
  clock_data_t data = getClockData_t(mElapsed);

  char clocktimeNow[8];
  uint8_t secs = data.currentSeconds;
  uint8_t mins = data.currentMinutes;
  uint8_t hours = data.currentHours;

  sprintf(clocktimeNow, "%02d:%02d:%02d", hours, mins, secs);

  display.clearDisplay();

  display.setTextSize(2);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);             // Start at top-left corner

  display.println(F(String(clocktimeNow).c_str()));

  display.display();
}

void ssd1306_Display_GlobalHashScreen(unsigned long mElapsed)
{
}

void ssd1306_Display_LoadingScreen(void)
{
}

void ssd1306_Display_SetupScreen(void)
{
}

void ssd1306_Display_AnimateCurrentScreen(unsigned long frame)
{
}

void ssd1306_Display_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction ssd1306_DisplayCyclicScreens[] = {ssd1306_Display_MinerScreen, ssd1306_Display_ClockScreen};

DisplayDriver esp32_ssd1306_driver = {
    ssd1306_Display_Init,
    ssd1306_Display_AlternateScreenState,
    ssd1306_Display_AlternateRotation,
    ssd1306_Display_LoadingScreen,
    ssd1306_Display_SetupScreen,
    ssd1306_DisplayCyclicScreens,
    ssd1306_Display_AnimateCurrentScreen,
    ssd1306_Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(ssd1306_DisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif