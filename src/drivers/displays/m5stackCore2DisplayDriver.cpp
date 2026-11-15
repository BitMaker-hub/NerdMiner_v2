#include "displayDriver.h"

#ifdef M5STACKCORE2_DISPLAY

#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>

#include "Free_Fonts.h"
#include "monitor.h"
#include "drivers/storage/storage.h"
#include "wManager.h"

extern monitor_data mMonitor;
extern TSettings Settings;

#define WIDTH 320
#define HEIGHT 240

void m5stackDisplay_Init(void)
{
  Serial.println("M5Stack Core2 display driver initialized");
  M5.begin(); // Init M5Stack Core2
  M5.Lcd.setRotation(1); // Landscape mode
  M5.Lcd.fillScreen(BLACK);
}

int screen_state = 1;
void m5stackDisplay_AlternateScreenState(void)
{
  screen_state == 1 ? M5.Lcd.sleep() : M5.Lcd.wakeup();
  screen_state ^= 1;
}

int screen_rotation = 1;
void m5stackDisplay_AlternateRotation(void)
{
  screen_rotation = (screen_rotation == 1) ? 3 : 1;
  M5.Lcd.setRotation(screen_rotation);
}

void m5stackDisplay_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextDatum(TL_DATUM);

  // Header
  M5.Lcd.setTextFont(4);
  M5.Lcd.drawString("NerdMiner", 10, 5);
  M5.Lcd.drawString(data.currentTime, 240, 5);
  M5.Lcd.drawLine(0, 30, 320, 30, TFT_DARKGREY);

  // Main hashrate display
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.drawString(data.currentHashRate, 20, 45);
  M5.Lcd.setTextFont(4);
  M5.Lcd.drawString("KH/s", 200, 70);

  // Stats column
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Templates:", 20, 120);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.templates, 130, 120);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Best Diff:", 20, 140);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.bestDiff, 130, 140);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Shares:", 20, 160);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.completedShares, 130, 160);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Valids:", 20, 180);
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.drawString(data.valids, 130, 180);

  // Right column
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Total:", 180, 120);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.totalKHashes + " KH", 230, 120);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Time:", 180, 140);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.timeMining, 180, 160);

  // Footer
  M5.Lcd.drawLine(0, 205, 320, 205, TFT_DARKGREY);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(TFT_GREENYELLOW);
  M5.Lcd.drawString("Temp: " + data.temp + "C", 10, 215);
}

void m5stackDisplay_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);

  // Large time display
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString(data.currentTime, 160, 50);

  // Date
  M5.Lcd.setTextFont(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.currentDate, 160, 110);

  // BTC Price
  M5.Lcd.setTextFont(4);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.drawString("BTC: " + data.btcPrice, 160, 145);

  // Block Height
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_GREENYELLOW);
  M5.Lcd.drawString("Block: " + data.blockHeight, 160, 180);

  // Hashrate
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_ORANGE);
  M5.Lcd.drawString(data.currentHashRate + " KH/s", 160, 200);

  // Footer info
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.drawString("Shares: " + data.completedShares, 160, 220);
}

void m5stackDisplay_GlobalHashScreen(unsigned long mElapsed)
{
  coin_data data = getCoinData(mElapsed);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextDatum(TL_DATUM);

  // Header
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString("NETWORK STATS", 10, 5);
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.drawString("BTC: " + data.btcPrice, 200, 5);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString(data.currentTime, 10, 20);
  M5.Lcd.drawLine(0, 35, 320, 35, TFT_DARKGREY);

  // Block Height - Large
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextColor(TFT_ORANGE);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString(data.blockHeight, 160, 45);

  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString("Block Height", 110, 95);

  // Network stats
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextFont(2);
  
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Global Hash:", 10, 120);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.globalHashRate + " EH/s", 120, 120);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Difficulty:", 10, 140);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.netwrokDifficulty, 120, 140);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("Fee (30min):", 10, 160);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString(data.halfHourFee, 120, 160);

  // Halving progress bar
  M5.Lcd.drawLine(0, 185, 320, 185, TFT_DARKGREY);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(TFT_GREENYELLOW);
  M5.Lcd.drawString("Next Halving Progress:", 10, 195);
  
  // Progress bar
  int barWidth = 300;
  int barHeight = 20;
  int barX = 10;
  int barY = 210;
  
  M5.Lcd.drawRect(barX, barY, barWidth, barHeight, TFT_WHITE);
  int fillWidth = (barWidth - 4) * data.progressPercent / 100;
  M5.Lcd.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, TFT_ORANGE);
  
  // Progress text
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.drawString(data.remainingBlocks, 160, 213);
}

void m5stackDisplay_LoadingScreen(void)
{
  Serial.println("M5Stack Core2 Initializing...");
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextFont(4);
  M5.Lcd.drawString("NerdMiner", 160, 80);
  
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString("M5Stack Core2", 160, 120);
  M5.Lcd.drawString("Initializing...", 160, 140);
  
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(TFT_DARKGREY);
}

void m5stackDisplay_SetupScreen(void)
{
  Serial.println("M5Stack Core2 Setup Mode");
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_ORANGE);
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextFont(4);
  M5.Lcd.drawString("SETUP MODE", 160, 60);
  
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString("Connect to WiFi:", 160, 100);
  
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("HanSoloAP", 160, 120);
  
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.drawString("Password:", 160, 145);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("MineYourCoins", 160, 165);
  
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(TFT_GREENYELLOW);
  M5.Lcd.drawString("Pool: " + String(Settings.PoolAddress), 160, 195);
  M5.Lcd.drawString("IP: " + WiFi.localIP().toString(), 160, 210);
}

void m5stackDisplay_DoLedStuff(unsigned long frame)
{
  // M5Stack Core2 doesn't have external LEDs like the original
  // Could add touch feedback or vibration motor effects here if desired
}

void m5stackDisplay_AnimateCurrentScreen(unsigned long frame)
{
  // Add any screen animations here if desired
}

CyclicScreenFunction m5stackDisplayCyclicScreens[] = {
    m5stackDisplay_MinerScreen,
    m5stackDisplay_ClockScreen,
    m5stackDisplay_GlobalHashScreen
};

DisplayDriver m5stackCore2DisplayDriver = {
    m5stackDisplay_Init,
    m5stackDisplay_AlternateScreenState,
    m5stackDisplay_AlternateRotation,
    m5stackDisplay_LoadingScreen,
    m5stackDisplay_SetupScreen,
    m5stackDisplayCyclicScreens,
    m5stackDisplay_AnimateCurrentScreen,
    m5stackDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(m5stackDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT
};
#endif