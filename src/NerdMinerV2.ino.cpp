#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "mbedtls/md.h"

#include <TFT_eSPI.h> // Graphics and font library
#include "Lib/images.h"
#include "Lib/myFonts.h"
#include "OpenFontRender.h"
#include "wManager.h"
#include "mining.h"

OpenFontRender render;

/**********************⚡ GLOBAL Vars *******************************/
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft);  // Invoke library sprite

static long templates = 0;
static long hashes = 0;
static int halfshares = 0; // increase if blockhash has 16 bits of zeroes
static int shares = 0; // increase if blockhash has 32 bits of zeroes
static int valids = 0; // increased if blockhash <= target                            

//void runMonitor(void *name);

/********* INIT *****/
void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(0);
  delay(100);

  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  disableCore0WDT();
  
  /******** INIT NERDMINER ************/
  Serial.println("NerdMinerv2 started......");
  
  /******** INIT DISPLAY ************/
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);// Swap the colour byte order when rendering
  background.createSprite(initWidth,initHeight); //Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background); // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); //Espaciado entre texto

  // Load the font and check it can be read OK
  //if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers))){
    Serial.println("Initialise error");
    return;
  }
  
  /******** PRINT INIT SCREEN *****/
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, initWidth, initHeight, initScreen);

  delay(2000);

  /******** INIT WIFI ************/
  init_WifiManager();
  
  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  xTaskCreate(runMonitor, "Monitor", 5000, NULL, 4, NULL);

  /******** CREATE TASKs TO PRINT SCREEN *****/
  for (int i = 0; i < 2; i++) {
    //char *name = (char*) malloc(32);
    //sprintf(name, "Worker[%d]", i);

    // Start mining tasks
    //BaseType_t res = xTaskCreate(runWorker, name, 35000, (void*)name, 1, NULL);
    xTaskCreate(runWorker, "worker", 35000, NULL, 1, NULL);
    //Serial.printf("Starting %s %s!\n", "worker", res == pdPASS? "successful":"failed");
  }
}

int oldStatus = 0;
unsigned long start = millis();
void loop() {

  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code  
  
  if(WiFi.status() != oldStatus) {
    if(WiFi.status() == WL_CONNECTED)
         Serial.println("CONNECTED - Current ip: " + WiFi.localIP().toString());
    else Serial.print("[Error] - current status: ");Serial.println(WiFi.status());
  }
  oldStatus = WiFi.status();

  //runWorker();
}




