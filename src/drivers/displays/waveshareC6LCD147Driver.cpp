#include "displayDriver.h"

#ifdef C6_LCD147_DISPLAY

#include <TFT_eSPI.h>
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include "rotation.h"

// Landscape: 320 wide x 172 tall
#define WIDTH  320
#define HEIGHT 172

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
  tft.setRotation(ROTATION_270); // Landscape: 320x172
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
  background.setTextFont(1);
  background.setTextDatum(TL_DATUM);
  background.drawString(label, x, y);
}

static void drawValue(int x, int y, const char* value, uint16_t color = C6_VALUE)
{
  background.setTextColor(color, C6_BG);
  background.setTextFont(2);
  background.setTextDatum(TL_DATUM);
  background.drawString(value, x, y);
}

static void drawVLine(int x)
{
  background.drawLine(x, 22, x, HEIGHT - 2, C6_LINE);
}

void c6Display_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  background.fillSprite(C6_BG);

  // Header bar (full width, thin)
  background.fillRect(0, 0, WIDTH, 20, C6_HEADER);
  background.setTextColor(C6_TEXT, C6_HEADER);
  background.setTextFont(1);
  background.setTextDatum(ML_DATUM);
  background.drawString("NerdMiner " CURRENT_VERSION, 4, 10);
  // Time in header right
  background.setTextDatum(MR_DATUM);
  background.drawString(data.currentTime.c_str(), WIDTH - 4, 10);

  // --- Left column: Hashrate ---
  int colW = 160;
  int lx = 16;  // left margin

  render.setFontSize(28);
  render.setFontColor(C6_HASH);
  render.rdrawString(data.currentHashRate.c_str(), lx + colW / 2 + 10, 30, C6_HASH);

  background.setTextColor(C6_LABEL, C6_BG);
  background.setTextFont(1);
  background.setTextDatum(TC_DATUM);
  background.drawString("KH/s", lx + colW / 2, 60);

  // Shares + Best Diff under hashrate
  int y = 76;
  drawLabel(lx, y, "Shares:");
  drawValue(lx + 50, y - 2, data.completedShares.c_str());

  y += 22;
  drawLabel(lx, y, "Best:");
  drawValue(lx + 50, y - 2, data.bestDiff.c_str());

  y += 22;
  drawLabel(lx, y, "Temp:");
  String tempStr = data.temp + "C";
  drawValue(lx + 50, y - 2, tempStr.c_str(), C6_TEXT);

  y += 22;
  drawLabel(lx, y, "Up:");
  drawValue(lx + 50, y - 2, data.timeMining.c_str(), C6_TEXT);

  // Vertical divider
  drawVLine(lx + colW);

  // --- Right column: Pool info ---
  int rx = lx + colW + 10;

  y = 26;
  drawLabel(rx, y, "Templates:");
  drawValue(rx + 68, y - 2, data.templates.c_str());

  y += 22;
  drawLabel(rx, y, "Valids:");
  drawValue(rx + 68, y - 2, data.valids.c_str(), 0x07FF);

  y += 26;
  background.drawLine(rx, y - 4, WIDTH - 8, y - 4, C6_LINE);

  // Temp + Uptime
  drawLabel(rx, y, "Temp:");
  String tempStr2 = data.temp + "C";
  drawValue(rx + 68, y - 2, tempStr2.c_str(), C6_TEXT);

  y += 22;
  drawLabel(rx, y, "Time:");
  drawValue(rx + 68, y - 2, data.currentTime.c_str(), C6_TEXT);

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
  background.fillRect(0, 0, WIDTH, 20, C6_HEADER);
  background.setTextColor(C6_TEXT, C6_HEADER);
  background.setTextFont(1);
  background.setTextDatum(MC_DATUM);
  background.drawString("NerdMiner " CURRENT_VERSION, WIDTH / 2, 10);

  // Time - large centered
  background.setFreeFont(FF23);
  background.setTextColor(C6_TEXT, C6_BG);
  background.setTextDatum(MC_DATUM);
  background.drawString(data.currentTime.c_str(), WIDTH / 2, 55, GFXFF);

  background.drawLine(8, 78, WIDTH - 8, 78, C6_LINE);

  // Bottom row: BTC Price | Block Height | Hashrate | Shares
  int y = 86;
  int col = WIDTH / 4;

  // BTC Price
  drawLabel(8, y, "BTC Price");
  background.setTextColor(C6_VALUE, C6_BG);
  background.setTextFont(2);
  background.setTextDatum(TL_DATUM);
  background.drawString(("$" + data.btcPrice).c_str(), 8, y + 14);

  // Block Height
  drawLabel(col + 8, y, "Block");
  drawValue(col + 8, y + 14, data.blockHeight.c_str(), 0xFD20);

  // Hashrate
  drawLabel(col * 2 + 8, y, "KH/s");
  drawValue(col * 2 + 8, y + 14, data.currentHashRate.c_str(), C6_HASH);

  // Shares
  drawLabel(col * 3 + 8, y, "Shares");
  drawValue(col * 3 + 8, y + 14, data.completedShares.c_str());

  // Date at bottom
  y += 40;
  background.drawLine(8, y - 4, WIDTH - 8, y - 4, C6_LINE);
  drawLabel(8, y, "Date:");
  drawValue(80, y - 2, data.currentDate.c_str(), C6_TEXT);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  background.pushSprite(0, 0);
}

void c6Display_LoadingScreen(void)
{
  tft.fillScreen(C6_BG);
  tft.fillRect(0, 46, WIDTH, 80, C6_HEADER);
  tft.setTextColor(C6_TEXT, C6_HEADER);
  tft.setFreeFont(FSSB9);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("NerdMiner", WIDTH / 2, 72, GFXFF);
  tft.drawString(CURRENT_VERSION, WIDTH / 2, 98, GFXFF);
  tft.setTextColor(C6_LABEL, C6_BG);
  tft.setTextFont(2);
  tft.drawString("ESP32-C6", WIDTH / 2, 140);
  tft.drawString("Initializing...", WIDTH / 2, 158);
}

void c6Display_SetupScreen(void)
{
  tft.fillScreen(C6_BG);
  tft.fillRect(0, 0, WIDTH, 24, C6_HEADER);
  tft.setTextColor(C6_TEXT, C6_HEADER);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WiFi Setup", WIDTH / 2, 12);

  int y = 34;
  tft.setTextColor(C6_TEXT, C6_BG);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connect to:", WIDTH / 2, y);
  tft.setTextColor(C6_VALUE, C6_BG);
  tft.drawString("NerdMinerAP", WIDTH / 2, y + 22);
  tft.setTextColor(C6_LABEL, C6_BG);
  tft.drawString("Pass: MineYourCoins", WIDTH / 2, y + 44);
  tft.setTextColor(C6_TEXT, C6_BG);
  tft.drawString("Open: 192.168.4.1", WIDTH / 2, y + 72);
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
