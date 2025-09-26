
#include <Wire.h>

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <OneButton.h>

#include "mbedtls/md.h"
#include "wManager.h"
#include "mining.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "ShaTests/nerdSHA_HWTest.h"
#include "timeconst.h"

#ifdef TOUCH_ENABLE
#include "TouchHandler.h"
#endif

#ifdef ESP32_2432S024R
#include <TFT_eSPI.h>
extern TFT_eSPI tft;
extern void esp32_2432S028R_AlternateScreenState(void);
#endif

#include <soc/soc_caps.h>
//#define HW_SHA256_TEST

//3 seconds WDT
#define WDT_TIMEOUT 3
//15 minutes WDT for miner task
#define WDT_MINER_TIMEOUT 900

#ifdef PIN_BUTTON_1
  OneButton button1(PIN_BUTTON_1);
#endif

#ifdef PIN_BUTTON_2
  OneButton button2(PIN_BUTTON_2);
#endif

#ifdef TOUCH_ENABLE
#ifndef ESP32_2432S024R
extern TouchHandler touchHandler;
#endif
#endif

extern monitor_data mMonitor;

#ifdef SD_ID
  SDCard SDCrd = SDCard(SD_ID);
#else  
  SDCard SDCrd = SDCard();
#endif

/**********************⚡ GLOBAL Vars *******************************/

unsigned long start = millis();
const char* ntpServer = "pool.ntp.org";

//void runMonitor(void *name);


/********* INIT *****/
#include "monitor.h"
#include "utils.h"
#include "version.h"

// Watchdog task to monitor the monitor task health
void runWatchdog(void *name) {
  Serial.println("[WATCHDOG] started");
  
  const unsigned long WATCHDOG_TIMEOUT = 120000; // 2 minutes
  
  while (1) {
    unsigned long currentTime = millis();
    
    // Check if monitor task has been inactive for too long
    if (currentTime - lastMonitorActivity > WATCHDOG_TIMEOUT) {
      Serial.println("[WATCHDOG] Monitor task appears frozen, restarting ESP32...");
      ESP.restart();
    }
    
    // Check every 30 seconds
    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
      //Init pin 15 to eneble 5V external power (LilyGo bug)
  #ifdef PIN_ENABLE5V
      pinMode(PIN_ENABLE5V, OUTPUT);
      digitalWrite(PIN_ENABLE5V, HIGH);
  #endif

#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED

  Serial.setTimeout(0);
  delay(SECOND_MS/10);

  esp_task_wdt_init(WDT_MINER_TIMEOUT, true);
  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  disableCore0WDT();
  //disableCore1WDT();

#ifdef HW_SHA256_TEST
  while (1) HwShaTest();
#endif

  // Setup the buttons
#ifdef ESP32_2432S024R
  // ESP32-2432S024R: Simple button behavior - only screen backlight toggle
  #ifdef PIN_BUTTON_1
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(esp32_2432S028R_AlternateScreenState); // Simple backlight toggle
    button1.attachLongPressStart(reset_configuration);
  #endif
  #ifdef PIN_BUTTON_2
    button2.setPressMs(5*SECOND_MS);
    button2.attachClick(esp32_2432S028R_AlternateScreenState); // Simple backlight toggle
    button2.attachLongPressStart(reset_configuration);
  #endif
#else
  // Other devices: Full button functionality
  #if defined(PIN_BUTTON_1) && !defined(PIN_BUTTON_2) //One button device
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(switchToNextScreen);
    button1.attachDoubleClick(alternateScreenRotation);
    button1.attachLongPressStart(reset_configuration);
    button1.attachMultiClick(alternateScreenState);
  #endif

  #if defined(PIN_BUTTON_1) && defined(PIN_BUTTON_2) //Button 1 of two button device
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(alternateScreenState);
    button1.attachDoubleClick(alternateScreenRotation);
  #endif

  #if defined(PIN_BUTTON_2) //Button 2 of two button device
    button2.setPressMs(5*SECOND_MS);
    button2.attachClick(switchToNextScreen);
    button2.attachLongPressStart(reset_configuration);
  #endif
#endif

  /******** INIT NERDMINER ************/
  Serial.println("NerdMiner v2 starting......");

  /******** INIT DISPLAY ************/
  initDisplay();
  
  /******** PRINT INIT SCREEN *****/
  drawLoadingScreen();
  delay(2*SECOND_MS);

  /******** SHOW LED INIT STATUS (devices without screen) *****/
  mMonitor.NerdStatus = NM_waitingConfig;
  doLedStuff(0);

#ifdef SDMMC_1BIT_FIX
  SDCrd.initSDcard();
#endif

  /******** INIT WIFI ************/
  init_WifiManager();

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  
  /******** CREATE WATCHDOG TASK *****/
  BaseType_t resWatchdog = xTaskCreatePinnedToCore(runWatchdog, "Watchdog", 2048, NULL, 1, NULL, 0);
  if (resWatchdog != pdPASS) {
    Serial.println("ERROR creating Watchdog task");
  }
  
  static const char monitor_name[] = "(Monitor)";
  #if defined(CONFIG_IDF_TARGET_ESP32)
  // Increased stack for ESP32 classic due to NVS operations  
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 9500, (void*)monitor_name, 5, NULL,1);
  #else
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 10000, (void*)monitor_name, 5, NULL,1);
  #endif

  /******** CREATE STRATUM TASK *****/
  static const char stratum_name[] = "(Stratum)";
 #if defined(CONFIG_IDF_TARGET_ESP32) && !defined(ESP32_2432S028R) && !defined(ESP32_2432S028_2USB) && !defined(ESP32_2432S024R)
  // Reduced stack for ESP32 classic to save memory
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 12000, (void*)stratum_name, 4, NULL,1);
 #elif defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB) || defined(ESP32_2432S024R)
  // Unified stack size for all 2432 variants to match working configuration
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 13500, (void*)stratum_name, 4, NULL,1);
 #else
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 15000, (void*)stratum_name, 4, NULL,1);
 #endif

  /******** CREATE MINER TASKS *****/
  //for (size_t i = 0; i < THREADS; i++) {
  //  char *name = (char*) malloc(32);
  //  sprintf(name, "(%d)", i);

  // Start mining tasks
  //BaseType_t res = xTaskCreate(runWorker, name, 35000, (void*)name, 1, NULL);
  TaskHandle_t minerTask1, minerTask2 = NULL;
  #ifdef HARDWARE_SHA265
    #if defined(CONFIG_IDF_TARGET_ESP32)
    xTaskCreate(minerWorkerHw, "MinerHw-0", 3584, (void*)0, 3, &minerTask1); // Reduced for ESP32 classic
    //xTaskCreate(minerWorkerSw, "MinerSw-0", 5000, (void*)0, 1, &minerTask1); // Reduced for ESP32 classic
    #else
    xTaskCreate(minerWorkerHw, "MinerHw-0", 4096, (void*)0, 3, &minerTask1);
    #endif
  #else
    #if defined(CONFIG_IDF_TARGET_ESP32)
    xTaskCreate(minerWorkerSw, "MinerSw-0", 5000, (void*)0, 1, &minerTask1); // Reduced for ESP32 classic
    #else
    xTaskCreate(minerWorkerSw, "MinerSw-0", 6000, (void*)0, 1, &minerTask1);
    #endif
  #endif
  esp_task_wdt_add(minerTask1);

#if (SOC_CPU_CORES_NUM >= 2)
  #if defined(CONFIG_IDF_TARGET_ESP32)
  xTaskCreate(minerWorkerSw, "MinerSw-1", 5000, (void*)1, 1, &minerTask2); // Reduced for ESP32 classic
  #else
  xTaskCreate(minerWorkerSw, "MinerSw-1", 6000, (void*)1, 1, &minerTask2);
  #endif
  esp_task_wdt_add(minerTask2);
#endif

  vTaskPrioritySet(NULL, 4);

  /******** MONITOR SETUP *****/
  setup_monitor();
}

void app_error_fault_handler(void *arg) {
  // Get stack errors
  char *stack = (char *)arg;

  // Print the stack errors in the console
  esp_log_write(ESP_LOG_ERROR, "APP_ERROR", "Error Stack Code:\n%s", stack);

  // restart ESP32
  esp_restart();
}

void loop() {
  // keep watching the push buttons:
  #ifdef PIN_BUTTON_1
    button1.tick();
  #endif

  #ifdef PIN_BUTTON_2
    button2.tick();
  #endif

#ifdef TOUCH_ENABLE
#ifdef ESP32_2432S024R
  // Simple touch handling for ESP32-2432S024R using TFT_eSPI
  static unsigned long lastTouchTime = 0;
  static bool lastTouchState = false;
  static unsigned long lastDebugTime = 0;
  
  // Debug: Track coordinate ranges
  static uint16_t min_x = 65535, max_x = 0;
  static uint16_t min_y = 65535, max_y = 0;
  static bool ranges_initialized = false;
  
  uint16_t x, y;
  bool touched = tft.getTouch(&x, &y, 200);
  
  // Track coordinate ranges when touching
  if (touched) {
    if (!ranges_initialized) {
      min_x = max_x = x;
      min_y = max_y = y;
      ranges_initialized = true;
    } else {
      if (x < min_x) min_x = x;
      if (x > max_x) max_x = x;
      if (y < min_y) min_y = y;
      if (y > max_y) max_y = y;
    }
    
    // Show ranges every 5 seconds
    if (millis() - lastDebugTime > 5000) {
      Serial.printf("Touch ranges detected - X: %d-%d, Y: %d-%d (Current: x=%d, y=%d)\n", 
                    min_x, max_x, min_y, max_y, x, y);
      lastDebugTime = millis();
    }
  }
  
  if (touched && !lastTouchState && (millis() - lastTouchTime > 500)) {
    // Accept touch anywhere on screen with coordinate validation
    if (x >= 0 && x <= 240 && y >= 0 && y <= 320) {
      Serial.printf("Touch at: x=%d, y=%d - switching display state\n", x, y);
      esp32_2432S028R_AlternateScreenState();
      lastTouchTime = millis();
      lastTouchState = true;
    }
  } else if (!touched) {
    lastTouchState = false;
  }
#else
  touchHandler.isTouched();
#endif
#endif
  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code

  vTaskDelay(50 / portTICK_PERIOD_MS);
}
