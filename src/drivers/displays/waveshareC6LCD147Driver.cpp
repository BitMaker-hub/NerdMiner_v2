#include "displayDriver.h"

#ifdef C6_LCD147_DISPLAY

#include <TFT_eSPI.h>
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include "rotation.h"

#define WIDTH  172
#define HEIGHT 320

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);

// Colors
#define C6_BG       0x0000  // Black
#define C6_HEADER   0x1A73  // Dark blue
#define C6_TEXT     0xFFFF  // White
#define C6_LABEL    0x9CF3  // Light gray
#define C6_VALUE    0x07E0  // Green
#define C6_HASH     0xFD20  // Orange
#define C6_LINE     0x2945  // Dark gray line

void c6Display_Init(void)
{
  tft.init();
  tft.setRotation(ROTATION_0); // Portrait: 172x320
  tft.setSwapBytes(true);
  tft.fillScreen(C6_BG);

  background.createSprite(WIDTH, HEIGHT);
  background.setSwapBytes(true);
  render.setDrawer(background);
  render.setLineSpaceRatio(0.9);

  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Font load error");
    return;
  }

  // Turn on backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
}

void c6Display_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

void c6Display_AlternateRotation(void)
{
  tft.setRotation(flipRotation(tft.getRotation()));
}

static void drawLabel(int x, int y, const char* label)
{
  background.setTextColor(C6_LABEL, C6_BG);
  background.setTextFont(2);
  background.setTextDatum(TL_DATUM);
  background.drawString(label, x, y);
}

static void drawValue(int x, int y, const char* value, uint16_t color = C6_VALUE)
{
  background.setTextColor(color, C6_BG);
  background.setFreeFont(FSSB9);
  background.setTextDatum(TL_DATUM);
  background.drawString(value, x, y, GFXFF);
}

static void drawHLine(int y)
{
  background.drawLine(8, y, WIDTH - 8, y, C6_LINE);
}

void c6Display_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  background.fillSprite(C6_BG);

  // Header bar
  background.fillRect(0, 0, WIDTH, 28, C6_HEADER);
  background.setTextColor(C6_TEXT, C6_HEADER);
  background.setTextFont(2);
  background.setTextDatum(MC_DATUM);
  background.drawString("NerdMiner " CURRENT_VERSION, WIDTH / 2, 14);

  // Hashrate - large centered
  render.setFontSize(32);
  render.setFontColor(C6_HASH);
  render.rdrawString(data.currentHashRate.c_str(), WIDTH / 2 + 30, 42, C6_HASH);

  background.setTextColor(C6_LABEL, C6_BG);
  background.setTextFont(2);
  background.setTextDatum(TC_DATUM);
  background.drawString("KH/s", WIDTH / 2, 72);

  drawHLine(90);

  // Stats grid
  int y = 96;
  int spacing = 28;

  drawLabel(8, y, "Shares:");
  drawValue(100, y, data.completedShares.c_str());

  y += spacing;
  drawLabel(8, y, "Best Diff:");
  drawValue(100, y, data.bestDiff.c_str());

  y += spacing;
  drawLabel(8, y, "Templates:");
  drawValue(100, y, data.templates.c_str());

  y += spacing;
  drawLabel(8, y, "Valids:");
  drawValue(100, y, data.valids.c_str(), 0x07FF); // Cyan

  drawHLine(y + spacing - 4);
  y += spacing;

  drawLabel(8, y, "Uptime:");
  drawValue(100, y, data.timeMining.c_str(), C6_TEXT);

  y += spacing;
  drawLabel(8, y, "Temp:");
  String tempStr = data.temp + " C";
  drawValue(100, y, tempStr.c_str(), C6_TEXT);

  y += spacing;
  drawLabel(8, y, "Time:");
  drawValue(100, y, data.currentTime.c_str(), C6_TEXT);

  // Serial output
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  background.pushSprite(0, 0);
}

void c6Display_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  background.fillSprite(C6_BG);

  // Header
  background.fillRect(0, 0, WIDTH, 28, C6_HEADER);
  background.setTextColor(C6_TEXT, C6_HEADER);
  background.setTextFont(2);
  background.setTextDatum(MC_DATUM);
  background.drawString("NerdMiner " CURRENT_VERSION, WIDTH / 2, 14);

  // Time - large
  background.setFreeFont(FF23);
  background.setTextColor(C6_TEXT, C6_BG);
  background.setTextDatum(MC_DATUM);
  background.drawString(data.currentTime.c_str(), WIDTH / 2, 65, GFXFF);

  drawHLine(90);

  // BTC Price
  int y = 100;
  drawLabel(8, y, "BTC Price:");
  background.setFreeFont(FSSB9);
  background.setTextColor(C6_VALUE, C6_BG);
  background.setTextDatum(TL_DATUM);
  background.drawString(("$" + data.btcPrice).c_str(), 8, y + 20, GFXFF);

  y += 56;
  drawHLine(y - 4);

  // Block Height
  drawLabel(8, y, "Block Height:");
  drawValue(8, y + 20, data.blockHeight.c_str(), 0xFD20);

  y += 56;
  drawHLine(y - 4);

  // Hashrate
  drawLabel(8, y, "Hashrate:");
  drawValue(8, y + 20, (data.currentHashRate + " KH/s").c_str(), C6_HASH);

  y += 50;
  drawLabel(8, y, "Shares:");
  drawValue(100, y, data.completedShares.c_str());

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  background.pushSprite(0, 0);
}

void c6Display_LoadingScreen(void)
{
  tft.fillScreen(C6_BG);
  tft.fillRect(0, 120, WIDTH, 80, C6_HEADER);
  tft.setTextColor(C6_TEXT, C6_HEADER);
  tft.setFreeFont(FSSB9);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("NerdMiner", WIDTH / 2, 145, GFXFF);
  tft.drawString(CURRENT_VERSION, WIDTH / 2, 170, GFXFF);
  tft.setTextColor(C6_LABEL, C6_BG);
  tft.setTextFont(2);
  tft.drawString("ESP32-C6", WIDTH / 2, 200);
  tft.drawString("Initializing...", WIDTH / 2, 225);
}

void c6Display_SetupScreen(void)
{
  tft.fillScreen(C6_BG);
  tft.fillRect(0, 40, WIDTH, 40, C6_HEADER);
  tft.setTextColor(C6_TEXT, C6_HEADER);
  tft.setFreeFont(FSSB9);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WiFi Setup", WIDTH / 2, 60, GFXFF);

  tft.setTextColor(C6_TEXT, C6_BG);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connect to WiFi:", WIDTH / 2, 110);
  tft.setTextColor(C6_VALUE, C6_BG);
  tft.drawString("NerdMinerAP", WIDTH / 2, 140);
  tft.setTextColor(C6_LABEL, C6_BG);
  tft.drawString("Password:", WIDTH / 2, 175);
  tft.setTextColor(C6_VALUE, C6_BG);
  tft.drawString("MineYourCoins", WIDTH / 2, 200);
  tft.setTextColor(C6_LABEL, C6_BG);
  tft.drawString("Open browser:", WIDTH / 2, 240);
  tft.setTextColor(C6_TEXT, C6_BG);
  tft.drawString("192.168.4.1", WIDTH / 2, 265);
}

void c6Display_AnimateCurrentScreen(unsigned long frame)
{
}

void c6Display_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction c6DisplayCyclicScreens[] = {c6Display_MinerScreen, c6Display_ClockScreen};

DisplayDriver c6LCD147DisplayDriver = {
    c6Display_Init,
    c6Display_AlternateScreenState,
    c6Display_AlternateRotation,
    c6Display_LoadingScreen,
    c6Display_SetupScreen,
    c6DisplayCyclicScreens,
    c6Display_AnimateCurrentScreen,
    c6Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(c6DisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
