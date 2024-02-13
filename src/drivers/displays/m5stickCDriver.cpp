#include "displayDriver.h"

#ifdef M5STICKC_DISPLAY

#include <M5StickC.h>

#include "media/images_160_80.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"

#define WIDTH 80
#define HEIGHT 160

#define GRAY 0x632C
#define LIGHTBLUE 0x4C77

int screen_state = 1;

void m5stickCDriver_Init(void)
{
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Axp.ScreenBreath(10);  //screen brightness 7-15
}

void m5stickCDriver_AlternateScreenState(void)
{
  if (screen_state==1) {
    M5.Lcd.writecommand(ST7735_DISPOFF);
    M5.Axp.ScreenBreath(0);
    screen_state=0;
  } else {
    M5.Lcd.writecommand(ST7735_DISPON);
    M5.Axp.ScreenBreath(10);
    screen_state=1;
  }
}

int m5stickCDriver_AlternateRotation(void)
{
  int screen_rotation;
  if (M5.Lcd.getRotation() == 3) 
    screen_rotation = 1;
  else 
    screen_rotation = 3;
    
  m5.Lcd.setRotation(screen_rotation);
  return screen_rotation;
}

void m5stickCDriver_SetRotation(int rotation)
{
  m5.Lcd.setRotation(rotation);
}

void m5stickCDriver_MinerScreen(unsigned long mElapsed)
{

  mining_data data = getMiningData(mElapsed);

  M5.Lcd.drawBitmap(0,0,MinerWidth, MinerHeight, MinerScreen);
  M5.Lcd.setFreeFont(&DSEG7_Classic_Bold_12);
  M5.Lcd.setTextColor(LIGHTBLUE,BLACK);
  M5.Lcd.setCursor(69, 69);
  M5.Lcd.println(String(data.currentHashRate));

  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.setCursor(117, 56);
  M5.Lcd.println("kH/s");

  M5.Lcd.setFreeFont(FMB9);
  M5.Lcd.setCursor(81, 22);
  M5.Lcd.println("VALID");

  M5.Lcd.setFreeFont(&DSEG7_Classic_Bold_17);
  M5.Lcd.setTextColor(LIGHTBLUE,BLACK);
  M5.Lcd.setCursor(101, 44);
  M5.Lcd.println(String(data.valids));
  
}

void m5stickCDriver_ClockScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  clock_data_t curr_clock_data = getClockData_t(mElapsed);

  
  M5.Lcd.fillScreen(BLACK);

  //Mining Time
  char timeMining[15]; 
  unsigned long secElapsed = millis() / 1000;
  int days = secElapsed / 86400; 
  int hours = (secElapsed - (days * 86400)) / 3600;                                                        //Number of seconds in an hour
  int mins = (secElapsed - (days * 86400) - (hours * 3600)) / 60;                                              //Remove the number of hours and calculate the minutes.
  int secs = secElapsed - (days * 86400) - (hours * 3600) - (mins * 60);   
  sprintf(timeMining, "%01d  %02d:%02d:%02d", days, hours, mins, secs);

  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.setCursor(40, 2);
  M5.Lcd.println("ELAPSED TIME");

  M5.Lcd.setFreeFont(&DSEG7_Classic_Bold_17);
  M5.Lcd.setTextColor(LIGHTBLUE,BLACK);
  M5.Lcd.setCursor(24, 42);
  M5.Lcd.println(String(timeMining));

  M5.Lcd.drawFastHLine(1, 52, 180, ORANGE);

  M5.Lcd.setFreeFont(&DSEG7_Classic_Bold_17);
  M5.Lcd.setTextColor(LIGHTBLUE,BLACK);
  M5.Lcd.setCursor(82, 76);
  M5.Lcd.println(String(data.currentTime));

  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.setCursor(3, 63);
  M5.Lcd.println("TIME NOW");
    
}

void m5stickCDriver_GlobalHashScreen(unsigned long mElapsed)
{
  coin_data data = getCoinData(mElapsed);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  M5.Lcd.fillScreen(BLACK);

  M5.Lcd.setTextFont(2);
  M5.Lcd.setCursor(118, 1);
  M5.Lcd.setTextColor(GREEN,BLACK);
  M5.Lcd.print("STATS");

  M5.Lcd.setCursor(5, 1);
  M5.Lcd.setTextColor(ORANGE,BLACK);
  M5.Lcd.print("BTC    ");
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.print(data.btcPrice.c_str());

  M5.Lcd.setCursor(5, 17);
  M5.Lcd.setTextColor(LIGHTBLUE,BLACK);
  M5.Lcd.print("Fee    ");
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.print(data.halfHourFee.c_str());

  M5.Lcd.setCursor(5, 33);
  M5.Lcd.setTextColor(ORANGE,BLACK);
  M5.Lcd.print("Diff    ");
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.print(data.netwrokDifficulty.c_str());

  M5.Lcd.setCursor(5, 49);
  M5.Lcd.setTextColor(LIGHTBLUE,BLACK);
  M5.Lcd.print("GHash  ");
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.print(data.globalHashRate.c_str());

  M5.Lcd.setCursor(5, 65);
  M5.Lcd.setTextColor(ORANGE,BLACK);
  M5.Lcd.print("Height  ");
  M5.Lcd.setTextColor(GRAY,BLACK);
  M5.Lcd.print(data.blockHeight.c_str());

}

void m5stickCDriver_LoadingScreen(void)
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawBitmap(0,0,MinerWidth, MinerHeight, MinerScreen);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(ORANGE,BLACK);
  M5.Lcd.setCursor(100, 10);
  M5.Lcd.println(CURRENT_VERSION);
}

void m5stickCDriver_SetupScreen(void)
{
 
}

void m5stickCDriver_AnimateCurrentScreen(unsigned long frame)
{
}

void m5stickCDriver_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction m5stickCDriverCyclicScreens[] = { m5stickCDriver_MinerScreen,m5stickCDriver_ClockScreen,m5stickCDriver_GlobalHashScreen};

DisplayDriver m5stickCDriver = {
    m5stickCDriver_Init,
    m5stickCDriver_AlternateScreenState,
    m5stickCDriver_AlternateRotation,
    m5stickCDriver_SetRotation,
    m5stickCDriver_LoadingScreen,
    m5stickCDriver_SetupScreen,
    m5stickCDriverCyclicScreens,
    m5stickCDriver_AnimateCurrentScreen,
    m5stickCDriver_DoLedStuff,
    SCREENS_ARRAY_SIZE(m5stickCDriverCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
