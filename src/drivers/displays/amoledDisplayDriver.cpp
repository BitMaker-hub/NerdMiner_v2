#include "displayDriver.h"

#ifdef AMOLED_DISPLAY

#include <rm67162.h>
#include <TFT_eSPI.h>
#include "media/images_536_240.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"

#define WIDTH 536
#define HEIGHT 240

#define SOURCE_HEIGHT 170

#define MARGIN_LEFT 42
#define SCALE HEIGHT / SOURCE_HEIGHT

#define X(x) (MARGIN_LEFT + (x * SCALE))
#define Y(y) (y * SCALE)
#define FS(S) (S * SCALE)

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);

void amoledDisplay_Init(void)
{
  rm67162_init();
  lcd_setRotation(1);

  background.createSprite(WIDTH, HEIGHT);
  background.setSwapBytes(true);
  render.setDrawer(background);
  render.setLineSpaceRatio(0.9);

  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }
}

int screen_state = 1;
void amoledDisplay_AlternateScreenState(void)
{
  screen_state == 1 ? lcd_off() : lcd_on();
  screen_state ^= 1;
}

int screen_rotation = 1;
void amoledDisplay_AlternateRotation(void)
{
  screen_rotation == 1 ? lcd_setRotation(3) : lcd_setRotation(1);
  screen_rotation ^= 1;
}

void amoledDisplay_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(FS(35));
  render.setFontColor(TFT_BLACK);

  render.rdrawString(data.currentHashRate.c_str(), X(118), Y(114), TFT_BLACK);
  // Total hashes
  render.setFontSize(FS(18));
  render.rdrawString(data.totalMHashes.c_str(), X(268), Y(138), TFT_BLACK);
  // Block templates
  render.setFontSize(FS(18));
  render.drawString(data.templates.c_str(), X(186), Y(20), 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), X(186), Y(48), 0xDEDB);
  // 32Bit shares
  render.setFontSize(FS(18));
  render.drawString(data.completedShares.c_str(), X(186), Y(76), 0xDEDB);
  // Hores
  render.setFontSize(FS(14));
  render.rdrawString(data.timeMining.c_str(), X(315), Y(104), 0xDEDB);

  // Valid Blocks
  render.setFontSize(FS(24));
  render.drawString(data.valids.c_str(), X(285), Y(56), 0xDEDB);

  // Print Temp
  render.setFontSize(FS(10));
  render.rdrawString(data.temp.c_str(), X(239), Y(1), TFT_BLACK);

  render.setFontSize(FS(4));
  render.rdrawString(String(0).c_str(), X(244), Y(3), TFT_BLACK);

  // Print Hour
  render.setFontSize(FS(10));
  render.rdrawString(data.currentTime.c_str(), X(286), Y(1), TFT_BLACK);

  // Push prepared background to screen
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate
  render.setFontSize(FS(25));
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), X(94), Y(129), TFT_BLACK);

  // Print BTC Price
  background.setFreeFont(FSSB12);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), X(202), Y(3), GFXFF);

  // Print BlockHeight
  render.setFontSize(FS(18));
  render.rdrawString(data.blockHeight.c_str(), X(254), Y(140), TFT_BLACK);

  // Print Hour
  background.setFreeFont(FF24);
  background.setTextSize(2);
  background.setTextColor(0xDEDB, TFT_BLACK);

  background.drawString(data.currentTime.c_str(), X(130), Y(50), GFXFF);

  // Push prepared background to screen
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_GlobalHashScreen(unsigned long mElapsed)
{
  coin_data data = getCoinData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print BTC Price
  background.setFreeFont(FSSB12);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), X(198), Y(3), GFXFF);

  // Print Hour
  background.setFreeFont(FSSB12);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), X(268), Y(3), GFXFF);

  // Print Last Pool Block
  background.setFreeFont(FSS12);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), X(302), Y(52), GFXFF);

  // Print Difficulty
  background.setFreeFont(FSS12);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), X(302), Y(88), GFXFF);

  // Print Global Hashrate
  render.setFontSize(FS(17));
  render.rdrawString(data.globalHashRate.c_str(), X(274), Y(145), TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(FS(28));
  render.rdrawString(data.blockHeight.c_str(), X(140), Y(104), 0xDEDB);

  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, Y(149), X(x2), Y(238), 0xDEDB);

  // Print Remaining BLocks
  background.setTextFont(FONT4);
  background.setTextSize(1);
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), X(72), Y(159), FONT2);

  // Push prepared background to screen
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_LoadingScreen(void)
{
  background.fillScreen(TFT_BLACK);
  background.pushImage(0, 0, initWidth, initHeight, initScreen);
  background.setTextColor(TFT_BLACK);
  background.drawString(CURRENT_VERSION, X(24), Y(147), FONT2);

  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_SetupScreen(void)
{
  background.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);

  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

void amoledDisplay_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction amoledDisplayCyclicScreens[] = {amoledDisplay_MinerScreen, amoledDisplay_ClockScreen, amoledDisplay_GlobalHashScreen};

DisplayDriver amoledDisplayDriver = {
    amoledDisplay_Init,
    amoledDisplay_AlternateScreenState,
    amoledDisplay_AlternateRotation,
    amoledDisplay_LoadingScreen,
    amoledDisplay_SetupScreen,
    amoledDisplayCyclicScreens,
    amoledDisplay_AnimateCurrentScreen,
    amoledDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(amoledDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
