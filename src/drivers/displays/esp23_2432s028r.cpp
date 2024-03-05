#include "displayDriver.h"

#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB) || defined(ESP32_2432S024)

#include <TFT_eSPI.h>
#include <TFT_eTouch.h>
#include "media/images_320_170.h"
#include "media/images_bottom_320_70.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include <SPI.h>

#define WIDTH 130 //320
#define HEIGHT 170 

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in platformio.ini
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite
SPIClass hSPI(HSPI);
TFT_eTouch<TFT_eSPI> touch(tft, ETOUCH_CS, 0xFF, hSPI); 

extern monitor_data mMonitor;
extern pool_data pData;
extern DisplayDriver *currentDisplayDriver;

bool hasChangedScreen = true;

void esp32_2432S028R_Init(void)
{ 
  tft.init();
  tft.setRotation(1);
  #ifdef ESP32_2432S028_2USB
  /*
  In addition to TFT_INVERSION this adjusts the gamma curve to have better
  picture quality for this type of ESP32_2432S028 TFT with for example two USB connectors
  */
  tft.writecommand(ILI9341_GAMMASET); // Gamma curve selected
  tft.writedata(2);
  delay(120);
  tft.writecommand(ILI9341_GAMMASET); // Gamma curve selected
  tft.writedata(1);
  #endif
  tft.setSwapBytes(true); // Swap the colour byte order when rendering
  hSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, ETOUCH_CS);
  touch.init();

  TFT_eTouchBase::Calibation calibation = { 233, 3785, 3731, 120, 2 };
  touch.setCalibration(calibation);
 
  //background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold)))
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }
  
  pinMode(LED_PIN, OUTPUT);
  pData.bestDifficulty = "0";
  pData.workersHash = "0";
  pData.workersCount = 0;
  //Serial.println("=========== Fim Display ==============") ;
}

void esp32_2432S028R_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

void esp32_2432S028R_AlternateRotation(void)
{
  tft.getRotation() == 1 ? tft.setRotation(3) : tft.setRotation(1);
  hasChangedScreen = true;
}

bool bottomScreenBlue = true;

void printPoolData(){
  
  pData = getPoolData();
  background.createSprite(320,70); //Background Sprite
  background.setSwapBytes(true);
  if (bottomScreenBlue) {
    background.pushImage(0, 0, 320, 70, bottonPoolScreen);
    //background.fillRect(295,43,25,10,TFT_CYAN);
    
  } else {
    background.pushImage(0, 0, 320, 70, bottonPoolScreen_g);
  }
  //background.setTextDatum(MC_DATUM);
  render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(1);
  
  render.setFontSize(24);
  render.cdrawString(String(pData.workersCount).c_str(), 155, 35, TFT_BLACK);
  render.setFontSize(18);
  render.setAlignment(Align::BottomRight);
  render.cdrawString(pData.workersHash.c_str(), 265, 34, TFT_BLACK);
  render.setAlignment(Align::BottomLeft);
  render.cdrawString(pData.bestDifficulty.c_str(), 54, 34, TFT_BLACK);

  background.pushSprite(0,170);
  background.deleteSprite();
}

void printheap(){
  //Serial.print("============ Free Heap:");
  //Serial.println(ESP.getFreeHeap()); 
}

void createBackgroundSprite(int16_t wdt, int16_t hgt){  // Set the background and link the render, used multiple times to fit in heap
  background.createSprite(wdt, hgt) ; //Background Sprite
  printheap();
  background.setColorDepth(16);
  background.setSwapBytes(true);
  render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9);
}

void esp32_2432S028R_MinerScreen(unsigned long mElapsed)
{
  if (hasChangedScreen) tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
  hasChangedScreen = false;

  mining_data data = getMiningData(mElapsed);
 
  //Serial.println("Proximo sprite...");
  int wdtOffset = 190;
  // Recreate sprite to the right side of the screen
  createBackgroundSprite(WIDTH-5, HEIGHT-7);
  //Print background screen    
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

  // Push prepared background to screen
  background.pushSprite(190, 0);

  // Delete sprite to free the memory heap
  background.deleteSprite();   
  printheap();

   //Serial.println("=========== Mining Display ==============") ;
  // Create background sprite to print data at once
  createBackgroundSprite(WIDTH-7, HEIGHT-100); // initHeight); //Background Sprite
  //Print background screen    
  background.pushImage(0, -90, MinerWidth, MinerHeight, MinerScreen);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Hashrate 
  render.setFontSize(35);
  render.setCursor(19, 118);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 118, 114-90, TFT_BLACK);
  
  // Push prepared background to screen
  background.pushSprite(0, 90);
  
  // Delete sprite to free the memory heap
  background.deleteSprite(); 
  //delay(50);
  
  printPoolData();   

  #ifdef DEBUG_MEMORY
    // Print heap
    printheap();
  #endif
}

void esp32_2432S028R_ClockScreen(unsigned long mElapsed)
{

  if (hasChangedScreen) tft.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen);
  hasChangedScreen = false;

  clock_data data = getClockData(mElapsed);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

 // Create background sprite to print data at once
  createBackgroundSprite(270,36);

  // Print background screen
  background.pushImage(0, -130, minerClockWidth, minerClockHeight, minerClockScreen);
  // Hashrate
  render.setFontSize(25);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_BLACK);

  // Push prepared background to screen
  background.pushSprite(0, 130);
  // Delete sprite to free the memory heap
  background.deleteSprite(); 

  createBackgroundSprite(169,105);
  // Print background screen
  background.pushImage(-130, -3, minerClockWidth, minerClockHeight, minerClockScreen);
  
  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 202-130, 0, GFXFF);
 
  // Print Hour
  background.setFreeFont(FF23);
  background.setTextSize(2);
  background.setTextColor(0xDEDB, TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 0, 50, GFXFF);
 
  // Push prepared background to screen
  background.pushSprite(130, 3);

  // Delete sprite to free the memory heap
  background.deleteSprite();   

  #ifdef ESP32_2432S028R      
   printPoolData();
  #endif      

  #ifdef DEBUG_MEMORY
  // Print heap
  printheap();
  #endif
}

void esp32_2432S028R_GlobalHashScreen(unsigned long mElapsed)
{
  if (hasChangedScreen) tft.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen);
  hasChangedScreen = false;
  coin_data data = getCoinData(mElapsed);

  // Create background sprite to print data at once
  createBackgroundSprite(169,105);
  // Print background screen
  background.pushImage(-160, -3, minerClockWidth, minerClockHeight, globalHashScreen);
  //background.fillScreen(TFT_BLUE);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 198-160, 0, GFXFF);
  // Print Hour
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 268-160, 0, GFXFF);

  // Print Last Pool Block
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.halfHourFee.c_str(), 302-160, 49, GFXFF);

  // Print Difficulty
  background.setFreeFont(FSS9);
  background.setTextDatum(TR_DATUM);
  background.setTextColor(0x9C92);
  background.drawString(data.netwrokDifficulty.c_str(), 302-160, 85, GFXFF);
  // Push prepared background to screen
  background.pushSprite(160, 3);
  // Delete sprite to free the memory heap
  background.deleteSprite();   

 // Create background sprite to print data at once
  createBackgroundSprite(280,30);
  // Print background screen
  background.pushImage(0, -139, minerClockWidth, minerClockHeight, globalHashScreen);
  //background.fillSprite(TFT_CYAN);
  // Print Global Hashrate
  render.setFontSize(17);
  render.rdrawString(data.globalHashRate.c_str(), 274, 145-139, TFT_BLACK);

  // Draw percentage rectangle
  int x2 = 2 + (138 * data.progressPercent / 100);
  background.fillRect(2, 149-139, x2, 168, 0xDEDB);

  // Print Remaining BLocks
  background.setTextFont(FONT2);
  background.setTextSize(1); 
  background.setTextDatum(MC_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.remainingBlocks.c_str(), 72, 159-139, FONT2);

  // Push prepared background to screen
  background.pushSprite(0, 139);
  // Delete sprite to free the memory heap
  background.deleteSprite();   

 // Create background sprite to print data at once
  createBackgroundSprite(140,40);
  // Print background screen
  background.pushImage(-5, -100, minerClockWidth, minerClockHeight, globalHashScreen);
  //background.fillSprite(TFT_CYAN);
  // Print BlockHeight
  render.setFontSize(28);
  render.rdrawString(data.blockHeight.c_str(), 140-5, 104-100, 0xDEDB);

  // Push prepared background to screen
  background.pushSprite(5, 100);
  // Delete sprite to free the memory heap
  background.deleteSprite();   
  #ifdef ESP32_2432S028R      
   printPoolData();
  #endif      

  #ifdef DEBUG_MEMORY
  // Print heap
  printheap();
  #endif
}
void esp32_2432S028R_BTCprice(unsigned long mElapsed)
{
  
  if (hasChangedScreen) tft.pushImage(0, 0, priceScreenWidth, priceScreenHeight, priceScreen);
  hasChangedScreen = false;

  clock_data data = getClockData(mElapsed);

  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

 // Create background sprite to print data at once
  createBackgroundSprite(270,36);

  // Print background screen
  background.pushImage(0, -130, priceScreenWidth, priceScreenHeight, priceScreen);
  // Hashrate
  render.setFontSize(25);
  render.setFontColor(TFT_BLACK);
  render.rdrawString(data.currentHashRate.c_str(), 95, 0, TFT_BLACK);

  // Print BlockHeight
  render.setFontSize(18);
  render.rdrawString(data.blockHeight.c_str(), 254, 9, TFT_WHITE);

  // Push prepared background to screen
  background.pushSprite(0, 130);
  // Delete sprite to free the memory heap
  background.deleteSprite(); 

  createBackgroundSprite(169,105);
  // Print background screen
  background.pushImage(-130, -3, priceScreenWidth, priceScreenHeight, priceScreen);
  
  // Print BTC Price
  background.setFreeFont(FSSB9);
  background.setTextSize(1);
  background.setTextDatum(TL_DATUM);
  background.setTextColor(TFT_BLACK);
  background.drawString(data.currentTime.c_str(), 202-130, 0, GFXFF);
 
  // Print Hour
  background.setFreeFont(FF22);
  background.setTextSize(2);
  background.setTextColor(0xDEDB, TFT_BLACK);
  background.drawString(data.btcPrice.c_str(), 0, 50, GFXFF);
 
  // Push prepared background to screen
  background.pushSprite(130, 3);

  // Delete sprite to free the memory heap
  background.deleteSprite();   

   #ifdef ESP32_2432S028R      
   printPoolData();
  #endif      

  #ifdef DEBUG_MEMORY
  // Print heap
  printheap();
  #endif
}

void esp32_2432S028R_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 33, initWidth, initHeight, initScreen);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(CURRENT_VERSION, 24, 147, FONT2);
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, MinerScreen);
}

void esp32_2432S028R_SetupScreen(void)
{
  tft.pushImage(0, 33, setupModeWidth, setupModeHeight, setupModeScreen);
}

void esp32_2432S028R_AnimateCurrentScreen(unsigned long frame)
{
}

// Variables para controlar el parpadeo con millis()
unsigned long previousMillis = 0;
unsigned long previousTouchMillis = 0;
char currentScreen = 0;

void esp32_2432S028R_DoLedStuff(unsigned long frame)
{
  unsigned long currentMillis = millis();    
  // / Check the touch coordinates 110x185 210x240
  if (currentMillis - previousTouchMillis >= 500)
    { 
      int16_t t_x , t_y;  // To store the touch coordinates
      bool pressed = touch.getXY(t_x, t_y);
      if (pressed) {                        
          if (((t_x > 109)&&(t_x < 211)) && ((t_y > 185)&&(t_y < 241))) bottomScreenBlue ^= true;
          else
            if (t_x > 160) {
              // next screen
              Serial.print(t_x);
              Serial.print(":x Próxima Tela y:");
              Serial.println(t_y);
              currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
            } else if (t_x < 160)
            {
              // Previus screen
              Serial.print(t_x);
              Serial.print(":x Tela anterior y:");
              Serial.println(t_y);
              /* Serial.println(currentDisplayDriver->current_cyclic_screen); */
              currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->current_cyclic_screen - 1;      
              if (currentDisplayDriver->current_cyclic_screen<0) currentDisplayDriver->current_cyclic_screen = currentDisplayDriver->num_cyclic_screens - 1;
              Serial.println(currentDisplayDriver->current_cyclic_screen);
            }
      }
      previousTouchMillis = currentMillis;
    }

    if (currentScreen != currentDisplayDriver->current_cyclic_screen) hasChangedScreen ^= true;
    currentScreen = currentDisplayDriver->current_cyclic_screen;

  switch (mMonitor.NerdStatus)
  {
  case NM_waitingConfig:
    digitalWrite(LED_PIN, HIGH); // LED encendido de forma continua
    break;

  case NM_Connecting:
    if (currentMillis - previousMillis >= 500)
    { // 0.5sec blink
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;

  case NM_hashing:
    if (currentMillis - previousMillis >= 100)
    { // 0.1sec blink
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;
  }
  

}

CyclicScreenFunction esp32_2432S028RCyclicScreens[] = {esp32_2432S028R_MinerScreen, esp32_2432S028R_ClockScreen, esp32_2432S028R_GlobalHashScreen, esp32_2432S028R_BTCprice};

DisplayDriver esp32_2432S028RDriver = {
    esp32_2432S028R_Init,
    esp32_2432S028R_AlternateScreenState,
    esp32_2432S028R_AlternateRotation,
    esp32_2432S028R_LoadingScreen,
    esp32_2432S028R_SetupScreen,
    esp32_2432S028RCyclicScreens,
    esp32_2432S028R_AnimateCurrentScreen,
    esp32_2432S028R_DoLedStuff,
    SCREENS_ARRAY_SIZE(esp32_2432S028RCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
