
#include "displayDriver.h"

#ifdef OLED_091_DISPLAY

#include <SSD1306.h>
#include <SSD1306Wire.h>
#include "monitor.h"

static uint8_t btc_icon[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xF0, 0x03, 0x00, 0x00, 0xFE, 0x1F, 0x00, 0x00, 0xFF, 0x3F, 0x00, 
  0x80, 0xFF, 0x7F, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 0xE0, 0x3F, 0xFF, 0x01, 
  0xF0, 0x3F, 0xFF, 0x03, 0xF0, 0x0F, 0xFC, 0x03, 0xF0, 0x0F, 0xF8, 0x03, 
  0xF8, 0xCF, 0xF9, 0x07, 0xF8, 0xCF, 0xF9, 0x07, 0xF8, 0x0F, 0xFC, 0x07, 
  0xF8, 0x0F, 0xF8, 0x07, 0xF8, 0xCF, 0xF9, 0x07, 0xF8, 0xCF, 0xF9, 0x07, 
  0xF0, 0x0F, 0xF8, 0x03, 0xF0, 0x0F, 0xFC, 0x03, 0xF0, 0x3F, 0xFF, 0x03, 
  0xE0, 0x3F, 0xFF, 0x01, 0xC0, 0xFF, 0xFF, 0x00, 0x80, 0xFF, 0x7F, 0x00, 
  0x00, 0xFF, 0x3F, 0x00, 0x00, 0xFE, 0x1F, 0x00, 0x00, 0xF0, 0x03, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

static uint8_t setup_icon[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0xE0, 0x01, 0x00, 
  0x00, 0xF0, 0x03, 0x00, 0xC0, 0xF0, 0xC3, 0x00, 0xE0, 0xF9, 0xE3, 0x01, 
  0xF0, 0xFF, 0xFF, 0x03, 0xF0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x01, 
  0xC0, 0xFF, 0xFF, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 0xE0, 0x1F, 0xFE, 0x00, 
  0xF8, 0x0F, 0xFC, 0x07, 0xFE, 0x07, 0xF8, 0x1F, 0xFE, 0x07, 0xF8, 0x1F, 
  0xFE, 0x07, 0xF8, 0x1F, 0xFE, 0x07, 0xF8, 0x1F, 0xF8, 0x0F, 0xFC, 0x07, 
  0xE0, 0x1F, 0xFE, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 0xC0, 0xFF, 0xFF, 0x00, 
  0xE0, 0xFF, 0xFF, 0x01, 0xF0, 0xFF, 0xFF, 0x03, 0xF0, 0xFF, 0xFF, 0x03, 
  0xE0, 0xF9, 0xE3, 0x01, 0xC0, 0xF0, 0xC3, 0x00, 0x00, 0xF0, 0x03, 0x00, 
  0x00, 0xE0, 0x01, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

SSD1306 display(0x3C, SDA_PIN, SCL_PIN, GEOMETRY_128_32);
bool displayStateOn = true;

void clearScreen(void) {
    display.clear();
    display.display();
}

void serialPrint(unsigned long mElapsed) {
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

void oledDisplay_Init(void) {
  Serial.println("OLED 0.42 display driver initialized");
  display.init();
  display.resetDisplay();
  display.flipScreenVertically();
  clearScreen();
}

void oledDisplay_AlternateScreenState(void) {
  Serial.println("Switching display state");

  if (true == displayStateOn) {
    display.displayOff();
    displayStateOn = false;
  } else {
    display.displayOn();
    displayStateOn = true;
  }
}

void oledDisplay_AlternateRotation(void) {
    Serial.println("Switching display rotation");

    display.flipScreenVertically();
    display.display();
}

void oledDisplay_Screen1(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawXbm(0, 1, 30, 30, btc_icon);
  display.drawString(32, 2, data.currentHashRate.c_str());
  display.setFont(ArialMT_Plain_10);
  display.drawString(95, 14, "KH/s");
  display.display();

  serialPrint(mElapsed);
}

void oledDisplay_Screen2(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  char temp[8];
  sprintf(temp, "%s°c", data.temp.c_str());

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 2, temp);
  display.display();

  serialPrint(mElapsed);
}

void oledDisplay_LoadingScreen(void) {
  Serial.println("Initializing...");
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 2, "Initializing...");
  display.display();
}

void oledDisplay_SetupScreen(void) {
  Serial.println("Setup...");
  display.clear();
  display.drawXbm(0, 1, 30, 30, setup_icon);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(32, 2, "Setup");
  display.display();
}

void oledDisplay_DoLedStuff(unsigned long frame) {

}

void oledDisplay_AnimateCurrentScreen(unsigned long frame) {

}

CyclicScreenFunction oledDisplayCyclicScreens[] = {oledDisplay_Screen1, oledDisplay_Screen2};

DisplayDriver oled091DisplayDriver = {
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
