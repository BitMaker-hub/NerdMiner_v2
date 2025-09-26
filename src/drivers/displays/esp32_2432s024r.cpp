#include "displayDriver.h"

#if defined(ESP32_2432S024R)

#include <TFT_eSPI.h>
#include "media/images_320_170.h"
#include "media/images_bottom_320_70.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include <SPI.h>
#include "rotation.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"

#define WIDTH 130 //320
#define HEIGHT 170 

extern nvMemory nvMem;

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in platformio.ini
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite

extern monitor_data mMonitor;
extern pool_data pData;
extern DisplayDriver *currentDisplayDriver;
extern bool invertColors; 
extern TSettings Settings;
bool hasChangedScreen = true;

void esp32_2432S024R_Init(void)
{ 
  tft.init();
  if (nvMem.loadConfig(&Settings))
    {      
      invertColors = Settings.invertColors;           
    }  
  tft.invertDisplay(invertColors);
  tft.setRotation(1);    
  tft.setSwapBytes(true); // Swap the colour byte order when rendering
  
  if (invertColors) {
    tft.writecommand(ILI9341_GAMMASET);
    tft.writedata(2);
    delay(120);
    tft.writecommand(ILI9341_GAMMASET); //Gamma curve selected
    tft.writedata(1); 
  }
  
  // Initialize touchscreen with TFT_eSPI integrated support
  uint16_t calData[5] = { 300, 3600, 300, 3600, 1 };
  tft.setTouch(calData);

  // Configuring screen backlight brightness using ledcontrol channel 0.
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, Settings.Brightness);

  // Load the font and check it can be read OK
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }
  
  // Configure RGB LEDs with PWM (channels 1, 2, 3)
  ledcSetup(1, 5000, 8);  // Canal 1 para LED rojo
  ledcSetup(2, 5000, 8);  // Canal 2 para LED azul  
  ledcSetup(3, 5000, 8);  // Canal 3 para LED verde
  ledcAttachPin(LED_PIN, 1);    // LED rojo al canal 1
  ledcAttachPin(LED_PIN_B, 2);  // LED azul al canal 2
  ledcAttachPin(LED_PIN_G, 3);  // LED verde al canal 3
  
  // Configure LEDs off initially (inverted logic: 255=off, 0=full on)
  ledcWrite(1, 255);  // LED rojo apagado
  ledcWrite(2, 255);  // LED azul apagado  
  ledcWrite(3, 255);  // LED verde apagado
  
  pData.bestDifficulty = "0";
  pData.workersHash = "0";
  pData.workersCount = 0;
}

void esp32_2432S024R_AlternateScreenState(void)
{
  Serial.println("Switching display state");
  int screen_state_duty = ledcRead(0);
  if (screen_state_duty > 0) {
    ledcWrite(0, 0);
  } else {
    ledcWrite(0, Settings.Brightness);
  }
}

void esp32_2432S024R_AlternateRotation(void)
{
  tft.setRotation( flipRotation(tft.getRotation()) );
  hasChangedScreen = true;
}

// Touch reading function for ESP32-2432S024R using TFT_eSPI integrated touch
bool esp32_2432S024R_getTouch(uint16_t *x, uint16_t *y) {
  return tft.getTouch(x, y, 200);  // 200 is touch threshold
}

bool bottomScreenBlue = true;

void printheap(){
  Serial.print("$$ Free Heap:");
  Serial.println(ESP.getFreeHeap()); 
}

bool createBackgroundSprite(int16_t wdt, int16_t hgt){
  background.createSprite(wdt, hgt);
  if (background.created()) {
      background.setColorDepth(16);
      background.setSwapBytes(true);
      render.setDrawer(background);
      render.setLineSpaceRatio(0.9);      
  } else {
    Serial.println("#### Sprite Error ####");
    Serial.printf("Size w:%d h:%d \n", wdt, hgt);
    printheap();
  }
  return background.created();
}

extern unsigned long mPoolUpdate;

void printPoolData(){
  if ((hasChangedScreen) || (mPoolUpdate == 0) || (millis() - mPoolUpdate > UPDATE_POOL_min * 60 * 1000)){     
      if (Settings.PoolAddress != "tn.vkbit.com") { 
          pData = getPoolData();             
          background.createSprite(320,50);
          if (!background.created()) {    
            Serial.println("###### POOL SPRITE ERROR ######");
            printheap();        
          }       
          background.setSwapBytes(true);
          if (bottomScreenBlue) {
            background.pushImage(0, -20, 320, 70, bottonPoolScreen);
            tft.pushImage(0,170,320,20,bottonPoolScreen);      
          } else {
            background.pushImage(0, -20, 320, 70, bottonPoolScreen_g);
            tft.pushImage(0,170,320,20,bottonPoolScreen_g);
          }
                
          render.setDrawer(background);
          render.setLineSpaceRatio(1);
          
          render.setFontSize(24);
          render.cdrawString(String(pData.workersCount).c_str(), 157, 16, TFT_BLACK);
          render.setFontSize(18);
          render.setAlignment(Align::BottomRight);
          render.cdrawString(pData.workersHash.c_str(), 265, 14, TFT_BLACK);
          render.setAlignment(Align::BottomLeft);
          render.cdrawString(pData.bestDifficulty.c_str(), 54, 14, TFT_BLACK);
          background.pushSprite(0,190);      
          background.deleteSprite();
      } else {
        pData.bestDifficulty = "TESTNET";
        pData.workersHash = "TESTNET";
        pData.workersCount = 1;
        tft.fillRect(0,170,320,70, TFT_DARKGREEN);        
        background.createSprite(320,40);
        background.fillSprite(TFT_DARKGREEN);
        if (!background.created()) {    
          Serial.println("###### POOL SPRITE ERROR ######");
          printheap();        
        }
        background.setFreeFont(FF24);
        background.setTextDatum(TL_DATUM);
        background.setTextSize(1);
        background.setTextColor(TFT_WHITE, TFT_DARKGREEN);        
        background.drawString("TESTNET", 50, 0, GFXFF);
        background.pushSprite(0,185);  
        mPoolUpdate = millis();
        Serial.println("Testnet");
        background.deleteSprite();
      }
  }
}

void esp32_2432S024R_MinerScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  printPoolData();

  if (hasChangedScreen) tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
    
  hasChangedScreen = false; 
 
  int wdtOffset = 190;
  createBackgroundSprite(WIDTH-5, HEIGHT-7);
  background.pushImage(-190, 0, MinerWidth, MinerHeight, MinerScreen);
  
  // Total hashes
  render.setFontSize(18);
  render.rdrawString(data.totalMHashes.c_str(), 268-wdtOffset, 138, TFT_BLACK);

  // Block templates
  render.setFontSize(18);
  render.setAlignment(Align::TopLeft);
  render.drawString(data.templates.c_str(), 189-wdtOffset, 20, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 189-wdtOffset, 48, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 189-wdtOffset, 76, 0xDEDB);
  // Hores
  render.setFontSize(14);
  render.rdrawString(data.timeMining.c_str(), 315-wdtOffset, 104, 0xDEDB);

  // Valid Blocks
  render.setFontSize(24);
  render.setAlignment(Align::TopCenter);
  render.drawString(data.valids.c_str(), 290-wdtOffset, 56, 0xDEDB);

  // Print Temp
  render.setFontSize(10);
  render.rdrawString(data.temp.c_str(), 239-wdtOffset, 1, TFT_BLACK);

  render.setFontSize(4);
  render.rdrawString(String(0).c_str(), 244-wdtOffset, 3, TFT_BLACK);

  // Print Hour
  render.setFontSize(10);
  render.rdrawString(data.currentTime.c_str(), 286-wdtOffset, 1, TFT_BLACK);

  background.pushSprite(190, 0);
  background.deleteSprite();   

  createBackgroundSprite(WIDTH-7, HEIGHT-100);
  background.pushImage(0, -90, MinerWidth, MinerHeight, MinerScreen);

  // Hashrate 
  render.setFontSize(35);
  render.setCursor(19, 118);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 118, 114-90, TFT_BLACK);
  
  background.pushSprite(0, 90);
  background.deleteSprite();  

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str()); 
}

void esp32_2432S024R_ClockScreen(unsigned long mElapsed)
{
  if (hasChangedScreen) tft.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);
  
  printPoolData();
  hasChangedScreen = false;

  clock_data data = getClockData(mElapsed);

  createBackgroundSprite(270,36);
  background.pushImage(0, -130, minerClockWidth, minerClockHeight, minerClockScreen);
  
  render.setFontSize(25);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);

  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_BLACK);

  background.pushSprite(0, 130);
  background.deleteSprite(); 

  createBackgroundSprite(169,105);
  background.pushImage(-130, -3, minerClockWidth, minerClockHeight, minerClockScreen);
  
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 202-130, 0, GFXFF);
 
  background.setFreeFont(FF23);
  background.setTextSize(2);
  background.setTextColor(0xDEDB, TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 0, 50, GFXFF);
 
  background.pushSprite(130, 3);
  background.deleteSprite();   

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
}

void esp32_2432S024R_GlobalHashScreen(unsigned long mElapsed)
{
  if (hasChangedScreen) tft.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);
  
  printPoolData();
  hasChangedScreen = false;
  
  coin_data data = getCoinData(mElapsed);

  createBackgroundSprite(169,105);
  background.pushImage(-160, -3, minerClockWidth, minerClockHeight, globalHashScreen);
  
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 198-160, 0, GFXFF);
  
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 268-160, 0, GFXFF);

  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), 302-160, 49, GFXFF);

  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), 302-160, 85, GFXFF);
  
  background.pushSprite(160, 3);
  background.deleteSprite();   

  createBackgroundSprite(280,30);
  background.pushImage(0, -139, minerClockWidth, minerClockHeight, globalHashScreen);
  
  render.setFontSize(17);
  render.rdrawString(data.globalHashRate.c_str(), 274, 145-139, TFT_BLACK);

  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149-139, x2, 168, 0xDEDB);

  background.setTextFont(FONT2);
  background.setTextSize(1); 
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 72, 159-139, FONT2);

  background.pushSprite(0, 139);
  background.deleteSprite();   

  createBackgroundSprite(140,40);
  background.pushImage(-5, -100, minerClockWidth, minerClockHeight, globalHashScreen);
  
  render.setFontSize(28);
  render.rdrawString(data.blockHeight.c_str(), 140-5, 104-100, 0xDEDB);

  background.pushSprite(5, 100);
  background.deleteSprite();   

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
}

void esp32_2432S024R_BTCprice(unsigned long mElapsed)
{
  if (hasChangedScreen) tft.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);
  printPoolData();
  hasChangedScreen = false;

  clock_data data = getClockData(mElapsed);

  createBackgroundSprite(270,36);
  background.pushImage(0, -130, priceScreenWidth, priceScreenHeight, priceScreen);
  
  render.setFontSize(25);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);

  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_WHITE);

  background.pushSprite(0, 130);
  background.deleteSprite(); 

  createBackgroundSprite(180,105);
  background.pushImage(-130, -3, priceScreenWidth, priceScreenHeight, priceScreen);
  
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 202-130, 0, GFXFF);
 
  background.setFreeFont(FF24);
  background.setTextDatum(TL_DATUM);
  background.setTextSize(1);
  background.setTextColor(0xDEDB, TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 0, 50, GFXFF);
 
  background.pushSprite(130, 3);
  background.deleteSprite();   

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());
}

void esp32_2432S024R_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 33, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(CURRENT_VERSION, 24, 147, FONT2);
}

void esp32_2432S024R_SetupScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 33, setupModeWidth, setupModeHeight, setupModeScreen);
}

void esp32_2432S024R_AnimateCurrentScreen(unsigned long frame)
{
}

// Variables para controlar el parpadeo con millis()
unsigned long previousMillis = 0;
unsigned long previousTouchMillis = 0;
char currentScreen = 0;

void esp32_2432S024R_DoLedStuff(unsigned long frame)
{
  unsigned long currentMillis = millis();    
  
  // Check the touch coordinates
  if (currentMillis - previousTouchMillis >= 500)
    { 
      uint16_t t_x, t_y;
      if (esp32_2432S024R_getTouch(&t_x, &t_y)) {                        
          if (((t_x > 109)&&(t_x < 211)) && ((t_y > 185)&&(t_y < 241))) {
            bottomScreenBlue ^= true;
            hasChangedScreen = true;
          } else if((t_x > 235) && ((t_y > 0)&&(t_y < 16))) {
            esp32_2432S024R_AlternateScreenState();
          }
          else
            if (t_x > 160) {
              currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
            } else if (t_x < 160)
            {
              currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->current_cyclic_screen - 1;      
              if (currentDisplayDriver->current_cyclic_screen<0) currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->num_cyclic_screens - 1;              
            }
      }
      previousTouchMillis = currentMillis;
    }

    if (currentScreen != currentDisplayDriver->current_cyclic_screen) hasChangedScreen ^= true;
    currentScreen = currentDisplayDriver->current_cyclic_screen;

  switch (mMonitor.NerdStatus)
  {
  case NM_waitingConfig:
    ledcWrite(1, 0); // LED rojo encendido continuo
    ledcWrite(2, 255); // LED azul apagado
    ledcWrite(3, 255); // LED verde apagado
    break;

  case NM_Connecting:
    if (currentMillis - previousMillis >= 500)
    { 
      previousMillis = currentMillis;
      static bool ledState = false;
      ledState = !ledState;
      ledcWrite(1, ledState ? 0 : 255); // LED rojo parpadeando
      ledcWrite(2, ledState ? 255 : 0); // LED azul parpadeando inverso
      ledcWrite(3, 255); // LED verde apagado
    }
    break;

  case NM_hashing:
    if (currentMillis - previousMillis >= 500)
    { 
      previousMillis = currentMillis;
      static bool ledState = false;
      ledState = !ledState;
      ledcWrite(1, ledState ? 0 : 255); // LED rojo parpadeando
      ledcWrite(2, 255); // LED azul apagado
      ledcWrite(3, 255); // LED verde apagado
    }
    break;
  }
}

CyclicScreenFunction esp32_2432S024RCyclicScreens[] = {esp32_2432S024R_MinerScreen, esp32_2432S024R_ClockScreen, esp32_2432S024R_GlobalHashScreen, esp32_2432S024R_BTCprice};

DisplayDriver esp32_2432S024RDriver = {
    esp32_2432S024R_Init,
    esp32_2432S024R_AlternateScreenState,
    esp32_2432S024R_AlternateRotation,
    esp32_2432S024R_LoadingScreen,
    esp32_2432S024R_SetupScreen,
    esp32_2432S024RCyclicScreens,
    esp32_2432S024R_AnimateCurrentScreen,
    esp32_2432S024R_DoLedStuff,
    SCREENS_ARRAY_SIZE(esp32_2432S024RCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif