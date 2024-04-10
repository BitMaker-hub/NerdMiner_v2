#include "displayDriver.h"

#ifdef M5STACK_DISPLAY

#include <Arduino.h>
#include <M5Stack.h>
#include <WiFi.h>

#include "Free_Fonts.h"
#include "monitor.h"
#include "drivers/storage/storage.h"
#include "wManager.h"

extern monitor_data mMonitor;
extern TSettings Settings;

void m5stackDisplay_Init(void)
{
  Serial.println("M5stack display driver initialized");
  M5.begin(); //Init M5Stack
  M5.Power.begin(); //Init power
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setFreeFont(FMB9);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println("");
  M5.Lcd.println("   Han ANother SOLOminer");
  M5.Lcd.drawLine(0,25,320,25,GREENYELLOW);
  M5.Lcd.fillRect(0,30,320,20,WHITE);
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.println("HAN SOLO is a solo miner");
  M5.Lcd.println(" on a ESP32."); M5.Lcd.setTextColor(RED);
  M5.Lcd.println("WARNING: you may have to wait");
  M5.Lcd.println(" longer than the current age");
  M5.Lcd.println(" of the universe to find a ");
  M5.Lcd.println(" valid block."); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawLine(0,200,320,200,GREENYELLOW);
  M5.Lcd.println("Connect via wifi to HanSoloAP");
  M5.Lcd.println(" with password MineYourCoins");
}

void m5stackDisplay_AlternateScreenState(void)
{
}

void m5stackDisplay_AlternateRotation(void)
{
}

void m5stackDisplay_NoScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print hashrate to serial
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
  //Serial.printf(">>> Temperature: %s\n", data.temp.c_str());

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setFreeFont(FMB9);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println("");
  M5.Lcd.println("   Han ANother SOLOminer");
  M5.Lcd.drawLine(0,25,320,25,GREENYELLOW);
  M5.Lcd.fillRect(0,30,320,20,WHITE);
  M5.Lcd.progressBar(0,30,320,20, data.currentHashRate.toInt());
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.print("Avg. hashrate : "); M5.Lcd.setTextColor(GREEN); M5.Lcd.print(data.currentHashRate); M5.Lcd.setTextColor(WHITE); M5.Lcd.println(" KH/s");
  M5.Lcd.print("Running time  : "); M5.Lcd.setTextColor(GREEN); M5.Lcd.println(data.timeMining); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("Total hashes  : "); M5.Lcd.setTextColor(GREEN); M5.Lcd.print(data.totalKHashes); M5.Lcd.setTextColor(WHITE); M5.Lcd.println(" KH");
  M5.Lcd.print("Block templ.  : "); M5.Lcd.setTextColor(YELLOW); M5.Lcd.println(data.templates); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("Best dificulty: "); M5.Lcd.setTextColor(YELLOW); M5.Lcd.println(data.bestDiff); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("Shares 32bits : "); M5.Lcd.setTextColor(YELLOW); M5.Lcd.println(data.completedShares); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("Valid blocks  : "); M5.Lcd.setTextColor(RED); M5.Lcd.println(data.valids); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("");
  M5.Lcd.drawLine(0,200,320,200,GREENYELLOW);
  M5.Lcd.print("Pool: "); M5.Lcd.setTextColor(GREENYELLOW); M5.Lcd.print(Settings.PoolAddress); M5.Lcd.print(":"); M5.Lcd.println(Settings.PoolPort); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("IP  : "); M5.Lcd.setTextColor(GREENYELLOW); M5.Lcd.println(WiFi.localIP()); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("");
}
void m5stackDisplay_LoadingScreen(void)
{
  Serial.println("Initializing...");
}

void m5stackDisplay_SetupScreen(void)
{
  Serial.println("Setup...");
}

void m5stackDisplay_DoLedStuff(unsigned long frame)
{
}

void m5stackDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction m5stackDisplayCyclicScreens[] = {m5stackDisplay_NoScreen};

DisplayDriver m5stackDisplayDriver = {
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
    0,
    0,
};
#endif
