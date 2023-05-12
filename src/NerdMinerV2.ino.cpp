//Botón configuración
#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 14

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <TFT_eSPI.h> // Graphics and font library
#include <OneButton.h>

#include "mbedtls/md.h"
#include "media/images.h"
#include "media/myFonts.h"
#include "OpenFontRender.h"
#include "wManager.h"
#include "mining.h"

//3 seconds WDT
#define WDT_TIMEOUT 3
OneButton button1(PIN_BUTTON_1);
OneButton button2(PIN_BUTTON_2);


OpenFontRender render;

/**********************⚡ GLOBAL Vars *******************************/
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft);  // Invoke library sprite

//static long templates = 0;
//static long hashes = 0;
//static int halfshares = 0; // increase if blockhash has 16 bits of zeroes
//static int shares = 0; // increase if blockhash has 32 bits of zeroes
//static int valids = 0; // increased if blockhash <= target

int oldStatus = 0;
unsigned long start = millis();

//void runMonitor(void *name);

void alternate_screen_state() {
  int screen_state= digitalRead(38);
  //Serial.printf("Screen state is '%s', switching to '%s'", screen_state, !screen_state);
  Serial.println("Switching display state");
  digitalWrite(38, !screen_state);
}

void alternate_screen_rotation() {
  tft.getRotation() == 1 ? tft.setRotation(3) : tft.setRotation(1);

}



/********* INIT *****/
void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(0);
  delay(100);

  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  disableCore0WDT();
  disableCore1WDT();

  // Setup the buttons
  // Button 1 (Boot)
  button1.setPressTicks(5000);
  button1.attachClick(alternate_screen_state);
  button1.attachDoubleClick(alternate_screen_rotation);
  // button1.attachLongPressStart([]{Serial.println("Button 1 started a long press");});
  // button1.attachLongPressStop([]{Serial.println("Button 1 stopped a long press");});
  // button1.attachDuringLongPress([]{Serial.println("Button 1 is being held down");});

  // Button 2 (GPIO14)
  button2.setPressTicks(5000);
  // button2.attachClick();
  // button2.attachDoubleClick([]{Serial.println("Button 2 was double clicked");});
  button2.attachLongPressStart(reset_configurations);
  // button2.attachLongPressStop(reset_configurations);
  // button2.attachDuringLongPress([]{Serial.println("Button 2 is being held down");});


  /******** INIT NERDMINER ************/
  Serial.println("NerdMiner v2 starting......");


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
  Serial.println("");
  Serial.println("Initiating tasks...");
  xTaskCreate(runMonitor, "Monitor", 5000, NULL, 4, NULL);

  /******** CREATE MINER TASKS *****/
  for (size_t i = 0; i < THREADS; i++) {
    char *name = (char*) malloc(32);
    sprintf(name, "(%d)", i);

    // Start mining tasks
    BaseType_t res = xTaskCreate(runWorker, name, 30000, (void*)name, 1, NULL);
    Serial.printf("Starting %s %s!\n", name, res == pdPASS? "successful":"failed");
  }
}

void app_error_fault_handler(void *arg) {
  // Get stack errors
  char *stack = (char *)arg;

  // Print the stack errors in the console
  esp_log_write(ESP_LOG_ERROR, "APP_ERROR", "Pila de errores:\n%s", stack);

  // restart ESP32
  esp_restart();
}

void loop() {
  // keep watching the push buttons:
  button1.tick();
  button2.tick();
  
  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code  
  
  int newStatus = WiFi.status();
  if (newStatus != oldStatus) {
    if (newStatus == WL_CONNECTED) {
      Serial.println("CONNECTED - Current ip: " + WiFi.localIP().toString());
    } else {
      Serial.print("[Error] - current status: ");
      Serial.println(newStatus);
    }
    oldStatus = newStatus;
  }

    //Run miner on main core when there is time --Currently on test
  // runMiner();

}