
#include "displayDriver.h"

#ifdef OLED_096_DISPLAY

#include <U8g2lib.h>
#include "media/images_128_64.h"
#include "monitor.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
bool screenOff = false;
bool rotationToggle = false;

void clearScreen(void)
{
  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void serialPrint(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print hashrate to serial
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print extended data to serial for no display devices
  Serial.printf(">>> Valid blocks: %s\n", data.valids.c_str());
  Serial.printf(">>> Block templates: %s\n", data.templates.c_str());
  Serial.printf(">>> Best difficulty: %s\n", data.bestDiff.c_str());
  Serial.printf(">>> 32Bit shares: %s\n", data.completedShares.c_str());
  Serial.printf(">>> Temperature: %s\n", data.temp.c_str());
  Serial.printf(">>> Total MHashes: %s\n", data.totalMHashes.c_str());
  Serial.printf(">>> Time mining: %s\n", data.timeMining.c_str());
}

void oledDisplay_Init(void)
{
  Serial.println("OLED 0.96 display driver initialized");
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.clear();
  u8g2.setFlipMode(rotationToggle);
  clearScreen();
}

void oledDisplay_AlternateScreenState(void)
{
  screenOff = !screenOff;
  u8g2.setPowerSave(screenOff);
}

void oledDisplay_AlternateRotation(void)
{
  if (screenOff)
    return;

  rotationToggle = !rotationToggle;
  u8g2.setFlipMode(rotationToggle);
}

void oledDisplay_Screen1(unsigned long mElapsed)
{
  if (screenOff)
    return;

  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.drawStr(0, 20, data.currentHashRate.c_str());
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawStr(45, 36, "KH/s");
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_Screen2(unsigned long mElapsed)
{
  if (screenOff)
    return;

  mining_data data = getMiningData(mElapsed);
  char temp[8];
  sprintf(temp, "%s°c", data.temp.c_str());

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.drawUTF8(0, 20, temp);
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_LoadingScreen(void)
{
  Serial.println("Initializing...");
  u8g2.clearBuffer();
  u8g2.drawXBMP(20, 5, btcIconWidth, btcIconHeight, btc_icon);
  u8g2.sendBuffer();
}

void oledDisplay_SetupScreen(void)
{
  Serial.println("Setup...");
  u8g2.clearBuffer();
  u8g2.drawXBMP(20, 0, setupIconWidth, setupIconHeight, setup_icon);
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawUTF8(20, 38, "Setup");
  u8g2.sendBuffer();
}

void oledDisplay_DoLedStuff(unsigned long frame)
{
}

void oledDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction oledDisplayCyclicScreens[] = {oledDisplay_Screen1, oledDisplay_Screen2};

DisplayDriver oled096DisplayDriver = {
    oledDisplay_Init,
    oledDisplay_AlternateScreenState,
    oledDisplay_AlternateRotation,
    oledDisplay_LoadingScreen,
    oledDisplay_SetupScreen,
    oledDisplayCyclicScreens,
    oledDisplay_AnimateCurrentScreen,
    oledDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(oledDisplayCyclicScreens),
    0,
    0,
    0,
};

#endif
