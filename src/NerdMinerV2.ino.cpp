

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <OneButton.h>

#include "mbedtls/md.h"
#include "wManager.h"
#include "mining.h"
#include "monitor.h"
#include "display/display.h"

//3 seconds WDT
#define WDT_TIMEOUT 3
//15 minutes WDT for miner task
#define WDT_MINER_TIMEOUT 900
OneButton button1(PIN_BUTTON_1);
OneButton button2(PIN_BUTTON_2);


extern monitor_data mMonitor;

/**********************⚡ GLOBAL Vars *******************************/

unsigned long start = millis();
const char* ntpServer = "pool.ntp.org";

//void runMonitor(void *name);

/********* INIT *****/
void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(0);
  delay(100);

  esp_task_wdt_init(WDT_MINER_TIMEOUT, true);
  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  disableCore0WDT();
  //disableCore1WDT();

  // Setup the buttons
  #if defined(NERDMINERV2) || defined(NERMINER_S3_AMOLED)
  // Button 1 (Boot)
  button1.setPressTicks(5000);
  button1.attachClick(alternateScreenState);
  button1.attachDoubleClick(alternateScreenRotation);
  // Button 2 (GPIO14)
  button2.setPressTicks(5000);
  button2.attachClick(switchToNextScreen);
  button2.attachLongPressStart(reset_configurations);
  #elif defined(DEVKITV1)
  //Standard ESP32-devKit
  button1.setPressTicks(5000);
  button1.attachLongPressStart(reset_configurations);
  pinMode(LED_PIN, OUTPUT);
  #endif
  



  /******** INIT NERDMINER ************/
  Serial.println("NerdMiner v2 starting......");

  /******** INIT DISPLAY ************/
  initDisplay();
  
  /******** PRINT INIT SCREEN *****/
  drawLoadingScreen();
  delay(2000);

  /******** SHOW LED INIT STATUS (devices without screen) *****/
  mMonitor.NerdStatus = NM_waitingConfig;
  #ifdef DEVKITV1
  doLedStuff(LED_PIN);
  #endif

  /******** INIT WIFI ************/
  init_WifiManager();

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  char *name = (char*) malloc(32);
  sprintf(name, "(%s)", "Monitor");
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 5000, (void*)name, 4, NULL,1);

  /******** CREATE STRATUM TASK *****/
  sprintf(name, "(%s)", "Stratum");
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 15000, (void*)name, 3, NULL,1);


  /******** CREATE MINER TASKS *****/
  //for (size_t i = 0; i < THREADS; i++) {
  //  char *name = (char*) malloc(32);
  //  sprintf(name, "(%d)", i);

  // Start mining tasks
  //BaseType_t res = xTaskCreate(runWorker, name, 35000, (void*)name, 1, NULL);
  TaskHandle_t minerTask1, minerTask2 = NULL;
  xTaskCreate(runMiner, "Miner0", 6000, (void*)0, 1, &minerTask1);
  xTaskCreate(runMiner, "Miner1", 6000, (void*)1, 1, &minerTask2);
 
  esp_task_wdt_add(minerTask1);
  esp_task_wdt_add(minerTask2);

  /******** MONITOR SETUP *****/
  setup_monitor();
  
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

  #ifdef DEVKITV1
  doLedStuff(LED_PIN);
  #endif

  vTaskDelay(50 / portTICK_PERIOD_MS);
}
