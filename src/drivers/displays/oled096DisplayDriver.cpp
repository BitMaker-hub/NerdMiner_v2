
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

void oledDisplay_MinerScreen(unsigned long mElapsed)
{
  if (screenOff)
    return;

  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();

  // Background
  u8g2.drawXBMP(0, 0, minerWidth, minerHeight, minerScreen);

  // Up Time
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x8_mf);
  u8g2.drawStr(73, 8, data.timeMining.c_str());
  u8g2.setDrawColor(1);

  // BestDiff & Templates
  u8g2.setFont(u8g2_font_5x7_mf);
  u8g2.drawStr(48, 25, "Best Diff");
  u8g2.drawStr(48, 38, "Templates");
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(99, 25, data.bestDiff.c_str());
  u8g2.drawStr(99, 38, data.templates.c_str());

  // Valid Blocks
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(14, 60, data.valids.c_str());
  u8g2.setDrawColor(1);

  // Total Million Hashes
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(50, 62, data.totalMHashes.c_str());

  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_ClockScreen(unsigned long mElapsed)
{
  if (screenOff)
    return;

  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();

  // Background
  u8g2.drawXBMP(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);

  // Clock
  u8g2.setFont(u8g2_font_logisoso26_tf);
  u8g2.drawStr(50, 40, data.currentTime.c_str());

  // Hash rate
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(5, 62, data.currentHashRate.c_str());
  u8g2.setDrawColor(1);

  // Temperature
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(94, 62, data.temp.c_str());

  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_GlobalHashScreen(unsigned long mElapsed)
{
  if (screenOff)
    return;

  coin_data data = getCoinData(mElapsed);

  u8g2.clearBuffer();

  // Background
  u8g2.drawXBMP(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);

  // bitcoin price
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_5x8_mr);
  u8g2.drawStr(84, 9, data.btcPrice.c_str());
  u8g2.setDrawColor(1);

  // difficulty
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(73, 34, data.netwrokDifficulty.c_str());

  // block height
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(2, 54, data.blockHeight.c_str());
  u8g2.setDrawColor(1);

  // global hash rate
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(78, 60, data.globalHashRate.c_str());
  u8g2.sendBuffer();

  serialPrint(mElapsed);
}

void oledDisplay_LoadingScreen(void)
{
  Serial.println("Initializing...");
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, initWidth, initHeight, initScreen);
  u8g2.sendBuffer();
}

void oledDisplay_SetupScreen(void)
{
  Serial.println("Setup...");
  u8g2.clearBuffer();
  u8g2.drawXBMP(15, 10, setupIconWidth, setupIconHeight, setup_icon);
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawUTF8(15, 48, "Setup");
  u8g2.drawXBMP(64, 2, qrCodeWidth, qrCodeHeight, server_qr_code);
  u8g2.sendBuffer();
}

void oledDisplay_DoLedStuff(unsigned long frame)
{
}

void oledDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction oledDisplayCyclicScreens[] = {oledDisplay_MinerScreen, oledDisplay_ClockScreen, oledDisplay_GlobalHashScreen};

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
