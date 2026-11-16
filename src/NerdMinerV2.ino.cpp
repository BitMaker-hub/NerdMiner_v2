
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

#include <soc/soc_caps.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include "version.h"

#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif
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
extern TouchHandler touchHandler;
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

//Task handles, global so the OTA hooks can suspend everything during a flash
TaskHandle_t minerTask1 = NULL, minerTask2 = NULL;
TaskHandle_t monitorTask = NULL, stratumTask = NULL;

//void runMonitor(void *name);


/********* INIT *****/
void setup()
{
      //Init pin 15 to eneble 5V external power (LilyGo bug)
      //Also used as power-hold latch on M5StickC Plus2 (GPIO4)
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

#ifdef OTA_ONLY_TEST
  //Diagnostic build: bring up WiFi + OTA only, no miner/monitor/stratum tasks.
  Serial.println("OTA_ONLY_TEST: no mining tasks started");
  vTaskPrioritySet(NULL, 4);
  return;
#endif

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  static const char monitor_name[] = "(Monitor)";
  #if defined(CONFIG_IDF_TARGET_ESP32)
  // Increased stack for ESP32 classic due to NVS operations  
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 9500, (void*)monitor_name, 5, &monitorTask,1);
  #else
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 10000, (void*)monitor_name, 5, &monitorTask,1);
  #endif

  /******** CREATE STRATUM TASK *****/
  static const char stratum_name[] = "(Stratum)";
 #if defined(CONFIG_IDF_TARGET_ESP32) && !defined(ESP32_2432S028R) && !defined(ESP32_2432S028_2USB)
  // Reduced stack for ESP32 classic to save memory
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 12000, (void*)stratum_name, 4, &stratumTask,1);
 #elif defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  // Free a little bit of the heap to the screen
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 13500, (void*)stratum_name, 4, &stratumTask,1);
 #else
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 15000, (void*)stratum_name, 4, &stratumTask,1);
 #endif

  /******** CREATE MINER TASKS *****/
  //for (size_t i = 0; i < THREADS; i++) {
  //  char *name = (char*) malloc(32);
  //  sprintf(name, "(%d)", i);

  // Start mining tasks
  //BaseType_t res = xTaskCreate(runWorker, name, 35000, (void*)name, 1, NULL);
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

//OTA over WiFi: firmware (upload) and SPIFFS config (uploadfs) via espota.
//Init deferred until WiFi is up; handle() below is a non-blocking UDP poll.
static bool s_ota_ready = false;
static volatile bool s_ota_in_progress = false;
static volatile uint32_t s_ota_last_progress_ms = 0;

//ArduinoOTA.handle() blocks inside loop() for the whole transfer, so a stalled
//transfer (dead sender, wifi drop) can never be detected from loop() itself:
//without this task the miners would stay suspended forever.
static void otaStallWatchdog(void *unused) {
  while (true) {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    if (s_ota_in_progress && (millis() - s_ota_last_progress_ms) > 90000) {
      Serial.println("OTA: stalled >90s, restarting");
      ESP.restart();
    }
  }
}

static void setupOTA() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  static char host[24];
  snprintf(host, sizeof(host), "nerdminer-%02x%02x", mac[4], mac[5]);
  ArduinoOTA.setHostname(host);
  //Never expose an unauthenticated OTA endpoint: anyone on the LAN could flash the
  //device. Builds that do not define OTA_PASSWORD simply run without OTA.
  if (sizeof(OTA_PASSWORD) <= 1) {
    Serial.println("OTA disabled: no OTA_PASSWORD set at build time");
    return;
  }
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    //Ask the miners to idle at a safe point and release the SHA engine lock,
    //then give in-flight jobs a moment to finish. Suspending them mid-hash would
    //keep that lock and deadlock Update.end()'s image SHA-256 verification.
    ota_active = true;
    vTaskDelay(300 / portTICK_PERIOD_MS);
    if (minerTask1) esp_task_wdt_delete(minerTask1);
    if (minerTask2) esp_task_wdt_delete(minerTask2);
    //Suspend the monitor (screen redraw, SPI) and stratum (pool socket): they
    //don't touch the SHA engine, so a plain suspend is safe and frees CPU/SPI.
    if (monitorTask) vTaskSuspend(monitorTask);
    if (stratumTask) vTaskSuspend(stratumTask);
    if (ArduinoOTA.getCommand() == U_SPIFFS)
      SPIFFS.end();
    s_ota_last_progress_ms = millis();
    s_ota_in_progress = true;
    Serial.println("OTA: transfer started, mining suspended");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    s_ota_last_progress_ms = millis();
    static unsigned int last = 0;
    if (progress < last) last = 0; //new transfer
    if (progress - last >= 131072 || progress == total) {
      last = progress;
      Serial.printf("OTA: %u/%u\n", progress, total);
    }
  });
  ArduinoOTA.onEnd([]() {
    s_ota_in_progress = false; //device reboots right after
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //No ESP.restart() here: ArduinoOTA reports the error detail to the sender
    //AFTER this callback (Update.printError). Rearm the stall watchdog so the
    //device reboots ~10s later instead, once the error left the socket.
    Serial.printf("OTA: error %u, reboot in ~10s\n", error);
    s_ota_in_progress = true;
    s_ota_last_progress_ms = millis() - 20000;
  });
  ArduinoOTA.begin();
  //Deployed version readable from the LAN: avahi-browse -r _arduino._tcp
  MDNS.addServiceTxt("arduino", "tcp", "fw_version", CURRENT_VERSION);
#ifdef AUTO_VERSION
  MDNS.addServiceTxt("arduino", "tcp", "fw_build", AUTO_VERSION);
#endif
  xTaskCreate(otaStallWatchdog, "OTAdog", 2048, NULL, 1, NULL);
  Serial.printf("OTA: ready as %s.local\n", host);
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
  touchHandler.isTouched();
#endif
  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code

  if (!s_ota_ready && WiFi.status() == WL_CONNECTED) {
    setupOTA();
    s_ota_ready = true;
  }
  if (s_ota_ready)
    ArduinoOTA.handle();

  vTaskDelay(50 / portTICK_PERIOD_MS);
}
