#include "displayDriver.h"

#ifdef T_DISPLAY

#include <TFT_eSPI.h>
#include "media/images_320_170.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"

#define WIDTH 340
#define HEIGHT 170

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite

void tDisplay_Init(void)
{
  tft.init();
  #ifdef LILYGO_S3_T_EMBED
  tft.setRotation(3);
  #else
  tft.setRotation(1);
  #endif
  tft.setSwapBytes(true);                 // Swap the colour byte order when rendering
  background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }
}

void tDisplay_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

int tDisplay_AlternateRotation(void)
{
  int screen_rotation = tft.getRotation() == 1 ? 3 : 1;
  tft.setRotation(screen_rotation);
  return screen_rotation; 
}

void tDisplay_SetRotation(int rotation)
{
  tft.setRotation(rotation);
}

void tDisplay_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(35);
  render.setCursor(19, 118);
  render.setFontColor(TFT_BLACK);

  render.rdrawString(data.currentHashRate.c_str(), 118, 114, TFT_BLACK);
  // Total hashes
  render.setFontSize(18);
  render.rdrawString(data.totalMHashes.c_str(), 268, 138, TFT_BLACK);
  // Block templates
  render.setFontSize(18);
  render.drawString(data.templates.c_str(), 186, 20, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 186, 48, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 186, 76, 0xDEDB);
  // Hores
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 315, 104, 0xDEDB);

  // Valid Blocks
  render.setFontSize(24);
  render.drawString(data.valids.c_str(), 285, 56, 0xDEDB);

  // Print Temp
  render.setFontSize(10);
  render.rdrawString(data.temp.c_str(), 239, 1, TFT_BLACK);

  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 244, 3, TFT_BLACK);

  // Print Hour
  render.setFontSize(10);
  render.rdrawString(data.currentTime.c_str(), 286, 1, TFT_BLACK);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(25);
  render.setCursor(19, 122);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 94, 129, TFT_BLACK);

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 202, 3, GFXFF);

  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 140, TFT_BLACK);

  // Print Hour
  background.setFreeFont(FF23);
  background.setTextSize(2);
  background.setTextColor(0xDEDB, TFT_BLACK);

  background.drawString(data.currentTime.c_str(), 130, 50, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_GlobalHashScreen(unsigned long mElapsed)
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
  background.drawString(data.btcPrice.c_str(), 198, 3, GFXFF);

  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 268, 3, GFXFF);

  // Print Last Pool Block
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), 302, 52, GFXFF);

  // Print Difficulty
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), 302, 88, GFXFF);

  // Print Global Hashrate
  render.setFontSize(17);
  render.rdrawString(data.globalHashRate.c_str(), 274, 145, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(28);
  render.rdrawString(data.blockHeight.c_str(), 140, 104, 0xDEDB);

  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149, x2, 168, 0xDEDB);

  // Print Remaining BLocks
  background.setTextFont(FONT2);
  background.setTextSize(1);
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 72, 159, FONT2);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}


void tDisplay_BTCprice(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);
  data.currentDate ="01/12/2023";
  
  //if(data.currentDate.indexOf("12/2023")>) { tDisplay_ChristmasContent(data); return; }

  // Print background screen
  background.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(25);
  render.setCursor(19, 122);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 94, 129, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 138, TFT_WHITE);

  // Print Hour
  
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 222, 3, GFXFF);

  // Print BTC Price 
  background.setFreeFont(FF24);
  background.setTextDatum(TR_DATUM);
  background.setTextSize(1);
  background.setTextColor(0xDEDB, TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 300, 58, GFXFF);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(CURRENT_VERSION, 24, 147, FONT2);
}

void tDisplay_SetupScreen(void)
{
  tft.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
}

void tDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

void tDisplay_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction tDisplayCyclicScreens[] = {tDisplay_MinerScreen, tDisplay_ClockScreen, tDisplay_GlobalHashScreen, tDisplay_BTCprice};

DisplayDriver tDisplayDriver = {
    tDisplay_Init,
    tDisplay_AlternateScreenState,
    tDisplay_AlternateRotation,
    tDisplay_SetRotation,
    tDisplay_LoadingScreen,
    tDisplay_SetupScreen,
    tDisplayCyclicScreens,
    tDisplay_AnimateCurrentScreen,
    tDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(tDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
