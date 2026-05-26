
#include "displayDriver.h"

#ifdef OLED_SSD1306_128X64_DISPLAY

#include <U8g2lib.h>
#include "monitor.h"

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void ssd1306_clearScreen(void) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void serialPrint_ssd1306(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  Serial.printf(">>> Hashrate: %s KH/s | Shares: %s | Best diff: %s\n",
                data.currentHashRate.c_str(), data.completedShares.c_str(), data.bestDiff.c_str());
}

void ssd1306Display_Init(void) {
  Serial.println("SSD1306 128x64 display driver initialized");
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.clear();
  ssd1306_clearScreen();
}

void ssd1306Display_AlternateScreenState(void) {}
void ssd1306Display_AlternateRotation(void) {}

void ssd1306Display_Screen1(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();

  // Header
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "NERDMINER v2");
  u8g2.drawHLine(0, 12, 128);

  // Hashrate (big)
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.drawStr(0, 38, data.currentHashRate.c_str());
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(90, 38, "KH/s");

  // Bottom line: shares and best diff
  u8g2.drawHLine(0, 50, 128);
  u8g2.setFont(u8g2_font_5x7_tf);
  String shares = "Shares: " + data.completedShares;
  String bestDiff = "Best: " + data.bestDiff;
  u8g2.drawStr(0, 62, shares.c_str());
  u8g2.drawStr(70, 62, bestDiff.c_str());

  u8g2.sendBuffer();
  serialPrint_ssd1306(mElapsed);
}

void ssd1306Display_Screen2(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "MINING STATS");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_5x7_tf);
  String hashrate = "Hashrate: " + data.currentHashRate + " KH/s";
  String hashes   = "Total MH: " + data.totalMHashes;
  String uptime   = "Uptime: " + data.timeMining;
  String temp     = "Temp: " + data.temp + " C";

  u8g2.drawStr(0, 26, hashrate.c_str());
  u8g2.drawStr(0, 36, hashes.c_str());
  u8g2.drawStr(0, 46, uptime.c_str());
  u8g2.drawStr(0, 56, temp.c_str());

  u8g2.sendBuffer();
  serialPrint_ssd1306(mElapsed);
}

void ssd1306Display_LoadingScreen(void) {
  Serial.println("Loading...");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.drawStr(10, 30, "NERD");
  u8g2.drawStr(10, 55, "MINER");
  u8g2.sendBuffer();
}

void ssd1306Display_SetupScreen(void) {
  Serial.println("Setup mode...");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(20, 20, "SETUP MODE");
  u8g2.drawStr(5, 35, "Connect to WiFi:");
  u8g2.drawStr(5, 48, "NerdMinerAP");
  u8g2.drawStr(5, 61, "MineYourCoins");
  u8g2.sendBuffer();
}

void ssd1306Display_DoLedStuff(unsigned long frame) {}
void ssd1306Display_AnimateCurrentScreen(unsigned long frame) {}

CyclicScreenFunction ssd1306DisplayCyclicScreens[] = {
    ssd1306Display_Screen1,
    ssd1306Display_Screen2
};

DisplayDriver ssd1306DisplayDriver = {
    ssd1306Display_Init,
    ssd1306Display_AlternateScreenState,
    ssd1306Display_AlternateRotation,
    ssd1306Display_LoadingScreen,
    ssd1306Display_SetupScreen,
    ssd1306DisplayCyclicScreens,
    ssd1306Display_AnimateCurrentScreen,
    ssd1306Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(ssd1306DisplayCyclicScreens),
    0,
    128,
    64,
};

#endif
