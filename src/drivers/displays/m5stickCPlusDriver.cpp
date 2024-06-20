#include "displayDriver.h"

#ifdef M5STICKCPLUS_DISPLAY

#include <M5StickCPlus.h>

#include "media/images_240_135.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "OpenFontRender.h"
#include "version.h"
#include "monitor.h"
#include "rotation.h"

#define WIDTH 240
#define HEIGHT 135

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();     // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite

int screen_state = 1;

void m5stickCPlusDriver_Init(void)
{
  M5.begin();
  M5.Axp.ScreenBreath(100);  //screen brightness 0 - 100

  tft.init();
  tft.setRotation(ROTATION_90);
  tft.setSwapBytes(true);                 // Swap the colour byte order when rendering
  background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto

  // Load the font and check it can be read OK
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    while(1);
    return;
  }
}

void m5stickCPlusDriver_AlternateScreenState(void)
{
  if (screen_state==1) {
    M5.Lcd.writecommand(ST7789_DISPOFF);
    M5.Axp.ScreenBreath(0);
    screen_state=0;
  } else {
    M5.Lcd.writecommand(ST7789_DISPON);
    M5.Axp.ScreenBreath(100);
    screen_state=1;
  }
}

void m5stickCPlusDriver_AlternateRotation(void)
{
    tft.setRotation( flipRotation(tft.getRotation()) );
}

void m5stickCPlusDriver_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(30);
  render.setCursor(19, 118);
  render.setFontColor(TFT_BLACK);

  render.rdrawString(data.currentHashRate.c_str(), 100, 82, TFT_BLACK);
  // Total hashes
  render.setFontSize(13);
  render.rdrawString(data.totalMHashes.c_str(), 200, 106, TFT_BLACK);
  // Block templates
  render.drawString(data.templates.c_str(), 140, 15, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 140, 36, 0xDEDB);
  // 32Bit shares
  render.drawString(data.completedShares.c_str(), 140, 56, 0xDEDB);
  // Hores
  render.setFontSize(9);
  render.rdrawString(data.timeMining.c_str(), 226, 80, 0xDEDB);

  // Valid Blocks
  render.setFontSize(19);
  render.drawString(data.valids.c_str(), 212, 42, 0xDEDB);

  // Print Temp
  render.setFontSize(8);
  render.rdrawString(data.temp.c_str(), 180, 1, TFT_BLACK);

  render.setFontSize(3);
  render.rdrawString(String(0).c_str(), 184, 2, TFT_BLACK);

  // Print Hour
  render.setFontSize(8);
  render.rdrawString(data.currentTime.c_str(), 215, 1, TFT_BLACK);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void m5stickCPlusDriver_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(20);
  render.setCursor(19, 122);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 70, 103, TFT_BLACK);

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 148, 1, GFXFF);

  // Print BlockHeight
  render.setFontSize(14);
  render.rdrawString(data.blockHeight.c_str(), 190, 110, TFT_BLACK);

  // Print Hour
  background.setFreeFont(FF22);
  background.setTextSize(2);
  background.setTextColor(TFT_WHITE, TFT_BLACK);

  background.drawString(data.currentTime.c_str(), 100, 40, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void m5stickCPlusDriver_GlobalHashScreen(unsigned long mElapsed)
{
  coin_data data = getCoinData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 148, 1, GFXFF);

  // Print Last Pool Block
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.halfHourFee.c_str(), 230, 40, GFXFF);

  // Print Difficulty
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.netwrokDifficulty.c_str(), 230, 68, GFXFF);

  // Print Global Hashrate
  render.setFontSize(12);
  render.rdrawString(data.globalHashRate.c_str(), 205, 115, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(23);
  render.rdrawString(data.blockHeight.c_str(), 105, 80, TFT_WHITE);

  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149, x2, 168, 0xDEDB);

  // Print Remaining BLocks
  background.setTextFont(FONT2);
  background.setTextSize(1);
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 55, 125, FONT2);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_BTCprice(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  
  // Print background screen
  background.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(22);
  render.setCursor(19, 122);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 75, 90, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(16);
  render.rdrawString(data.blockHeight.c_str(), 190, 100, TFT_WHITE);

  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 148, 1, GFXFF);

  // Print BTC Price 
  background.setFreeFont(FF18);
  background.setTextDatum(TR_DATUM);
  background.setTextSize(2);
  background.setTextColor(TFT_WHITE);
  background.drawString(data.btcPrice.c_str(), 230, 40, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void m5stickCPlusDriver_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, initScreen);
  tft.setTextFont(2);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(20, 110);
  tft.println(CURRENT_VERSION);
}

void m5stickCPlusDriver_SetupScreen(void)
{
  tft.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
}

void m5stickCPlusDriver_AnimateCurrentScreen(unsigned long frame)
{
}

void m5stickCPlusDriver_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction m5stickCPlusDriverCyclicScreens[] = { m5stickCPlusDriver_MinerScreen, m5stickCPlusDriver_ClockScreen, m5stickCPlusDriver_GlobalHashScreen, tDisplay_BTCprice};

DisplayDriver m5stickCPlusDriver = {
    m5stickCPlusDriver_Init,
    m5stickCPlusDriver_AlternateScreenState,
    m5stickCPlusDriver_AlternateRotation,
    m5stickCPlusDriver_LoadingScreen,
    m5stickCPlusDriver_SetupScreen,
    m5stickCPlusDriverCyclicScreens,
    m5stickCPlusDriver_AnimateCurrentScreen,
    m5stickCPlusDriver_DoLedStuff,
    SCREENS_ARRAY_SIZE(m5stickCPlusDriverCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
