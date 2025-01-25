
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
#include "timeconst.h"

#ifdef TOUCH_ENABLE
#include "TouchHandler.h"
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

//void runMonitor(void *name);

#ifdef HW_SHA256_TEST

#include "ShaTests/nerdSHA256plus.h"
#include "mbedtls/sha256.h"
#include <sha/sha_dma.h>
#include <hal/sha_hal.h>
#include <hal/sha_ll.h>

static const uint8_t s_test_buffer[128] = 
{
  0x00, 0x00, 0x00, 0x22, 0x99, 0x44, 0xbb, 0xff, 0xbb, 0x00, 0x00, 0x77, 0x44, 0xcc, 0x11, 0x77,
  0x88, 0x55, 0xbb, 0x44, 0x55, 0x00, 0x77, 0x88, 0x99, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xbb, 0xbb, 0x66, 0x11, 0x88, 0x33, 0x44, 0x99, 0xcc, 0x33, 0xff, 0x22,
  0x11, 0xaa, 0x77, 0xee, 0xbb, 0x66, 0xee, 0xcc, 0xee, 0x66, 0xee, 0xdd, 0x77, 0x55, 0x22, 0x22,
  0xcc, 0xcc, 0x66, 0xee, 0x22, 0xdd, 0x99, 0x66, 0x66, 0x88, 0x00, 0x11, 0x2e, 0x33, 0x41, 0x19,

  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80
};

static const uint8_t s_test_buffer_aligned[128] __attribute__((aligned(256))) = 
{
  0x00, 0x00, 0x00, 0x22, 0x99, 0x44, 0xbb, 0xff, 0xbb, 0x00, 0x00, 0x77, 0x44, 0xcc, 0x11, 0x77,
  0x88, 0x55, 0xbb, 0x44, 0x55, 0x00, 0x77, 0x88, 0x99, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xbb, 0xbb, 0x66, 0x11, 0x88, 0x33, 0x44, 0x99, 0xcc, 0x33, 0xff, 0x22,
  0x11, 0xaa, 0x77, 0xee, 0xbb, 0x66, 0xee, 0xcc, 0xee, 0x66, 0xee, 0xdd, 0x77, 0x55, 0x22, 0x22,
  0xcc, 0xcc, 0x66, 0xee, 0x22, 0xdd, 0x99, 0x66, 0x66, 0x88, 0x00, 0x11, 0x2e, 0x33, 0x41, 0x19,

  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80
};

static uint8_t interResult_aligned[64] __attribute__((aligned(256)));
static uint8_t midstate_aligned[32] __attribute__((aligned(256)));
static uint8_t hash_aligned[64] __attribute__((aligned(256)));

IRAM_ATTR void nerd_sha_hal_wait_idle()
{
    while (sha_ll_busy())
    {}
}

IRAM_ATTR void HwShaTest()
{
  uint8_t interResult[64];
  uint8_t midstate[32];
  uint8_t hash[64];
  memset(interResult, 0, sizeof(interResult));
  interResult[32] = 0x80;
  interResult[62] = 0x01;
  interResult[63] = 0x00;

  memset(interResult_aligned, 0, sizeof(interResult_aligned));
  interResult_aligned[32] = 0x80;
  interResult_aligned[62] = 0x01;
  interResult_aligned[63] = 0x00;
  
  uint32_t bake[16];

  uint32_t time_start = micros();
  const int test_count = 1000000;

#if 0
  //Generic software  16KH/s
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  for (int i = 0; i < test_count; ++i)
  { 
    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, s_test_buffer, 80);
    mbedtls_sha256_finish_ret(&ctx, interResult);

    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, interResult, 32);
    mbedtls_sha256_finish_ret(&ctx, hash);
  }
  mbedtls_sha256_free(&ctx);
#endif

#if 0
  //nerdSha256 (ESP32 39KH/s)
  nerdSHA256_context ctx;
  nerd_mids(&ctx, s_test_buffer);
  for (int i = 0; i < test_count; ++i)
  {
    nerd_sha256d(&ctx, s_test_buffer+64, hash);
  }
#endif

#if 0
  //nerdSha256 bake (ESP32 41KH/s)
  nerdSHA256_context ctx;
  nerd_mids(&ctx, s_test_buffer);
  nerd_sha256_bake(ctx.digest, s_test_buffer+64, bake);  //15 words
  for (int i = 0; i < test_count; ++i)
  {
    nerd_sha256d_baked(ctx.digest, s_test_buffer+64, bake, hash);
  }
#endif

#if 0
  //Hardware high level 62KH/s
  esp_sha_acquire_hardware();
  for (int i = 0; i < test_count; ++i)
  {
      esp_sha_dma(SHA2_256, s_test_buffer+64, 64, s_test_buffer, 64, true);
      esp_sha_read_digest_state(SHA2_256, interResult);
      esp_sha_dma(SHA2_256, 0, 0, interResult, 64, true);
      esp_sha_read_digest_state(SHA2_256, hash);
  }
  esp_sha_release_hardware();
#endif

#if 0
  //Hardware block
  //NOT avaliable
  esp_sha_lock_engine(SHA2_256);
  for (int i = 0; i < test_count; ++i)
  {
      esp_sha_block(SHA2_256, s_test_buffer, true);
      esp_sha_block(SHA2_256, s_test_buffer+64, false);
      esp_sha_read_digest_state(SHA2_256, interResult);
      esp_sha_block(SHA2_256, interResult, true);
      esp_sha_read_digest_state(SHA2_256, hash);
  }
  esp_sha_unlock_engine(SHA2_256);
#endif

#if 0
  //Hardware low level 132KH/s
  esp_sha_acquire_hardware();
  for (int i = 0; i < test_count; ++i)
  {
      sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
      sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
      sha_hal_read_digest(SHA2_256, interResult);
      sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
      sha_hal_read_digest(SHA2_256, hash);
  }
  esp_sha_release_hardware();
#endif

#if 0
  //Hardware low level + midstate 156KH/s
  esp_sha_acquire_hardware();
  sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
  sha_hal_read_digest(SHA2_256, midstate);
  for (int i = 0; i < test_count; ++i)
  {
      sha_hal_write_digest(SHA2_256, midstate);
      sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
      sha_hal_read_digest(SHA2_256, interResult);
      sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
      sha_hal_read_digest(SHA2_256, hash);
  }
  esp_sha_release_hardware();
#endif

#if 0
  //Hardware low level + midstate + aligned 156KH/s (No sense)
  esp_sha_acquire_hardware();
  sha_hal_hash_block(SHA2_256, s_test_buffer_aligned, 64/4, true);
  sha_hal_read_digest(SHA2_256, midstate_aligned);
  for (int i = 0; i < test_count; ++i)
  {
      sha_hal_write_digest(SHA2_256, midstate_aligned);
      sha_hal_hash_block(SHA2_256, s_test_buffer_aligned+64, 64/4, false);
      sha_hal_read_digest(SHA2_256, interResult_aligned);
      sha_hal_hash_block(SHA2_256, interResult_aligned, 64/4, true);
      sha_hal_read_digest(SHA2_256, hash_aligned);
  }
  esp_sha_release_hardware();
  memcpy(hash, hash_aligned, sizeof(hash_aligned));
#endif

#if 1
  //Hardware LL 161KH/s
  esp_sha_acquire_hardware();
  //sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
  sha_hal_wait_idle();
  sha_ll_fill_text_block(s_test_buffer, 64/4);
  sha_ll_start_block(SHA2_256);

  //sha_hal_read_digest(SHA2_256, midstate);
  sha_ll_load(SHA2_256);
  sha_hal_wait_idle();
  sha_ll_read_digest(SHA2_256, midstate, 256 / 32);

  for (int i = 0; i < test_count; ++i)
  {
      //sha_hal_write_digest(SHA2_256, midstate);
      sha_ll_write_digest(SHA2_256, midstate, 256 / 32);
      
      //sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
      sha_hal_wait_idle();
      sha_ll_fill_text_block(s_test_buffer+64, 64/4);
      sha_ll_continue_block(SHA2_256);
      
      //sha_hal_read_digest(SHA2_256, interResult);
      sha_ll_load(SHA2_256);
      sha_hal_wait_idle();
      sha_ll_read_digest(SHA2_256, interResult, 256 / 32);
      
      //sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
      sha_hal_wait_idle();
      sha_ll_fill_text_block(interResult, 64/4);
      sha_ll_start_block(SHA2_256);

      //sha_hal_read_digest(SHA2_256, hash);
      sha_ll_load(SHA2_256);
      sha_hal_wait_idle();
      sha_ll_read_digest(SHA2_256, hash, 256 / 32);
  }
  esp_sha_release_hardware();
#endif

  uint32_t time_end = micros();
  double hash_rate = ((double)test_count * 1000000) / (double)(time_end - time_start);
  Serial.print("DmaHashrate=");
  Serial.print((int)hash_rate/1000);
  Serial.println("KH/s");

  Serial.print("interResult: ");
  for (size_t i = 0; i < 32; i++)
    Serial.printf("%02x", interResult[i]);
  Serial.println("");

    Serial.print("hash: ");
  for (size_t i = 0; i < 32; i++)
    Serial.printf("%02x", hash[i]);
  Serial.println("");
  
  //should be
  //6fa464b007f2d577edfa5dfe9dfc3f9209f36d1a6711d314ea68ccdd03000000
}

#endif

/********* INIT *****/
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
  while (1)
  {
    HwShaTest();
  }
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

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  char *name = (char*) malloc(32);
  sprintf(name, "(%s)", "Monitor");
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 10000, (void*)name, 4, NULL,1);

  /******** CREATE STRATUM TASK *****/
  sprintf(name, "(%s)", "Stratum");
 #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
 // Free a little bit of the heap to the screen
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 13500, (void*)name, 3, NULL,1);
 #else
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 15000, (void*)name, 3, NULL,1);
 #endif

  /******** CREATE MINER TASKS *****/
  //for (size_t i = 0; i < THREADS; i++) {
  //  char *name = (char*) malloc(32);
  //  sprintf(name, "(%d)", i);

  // Start mining tasks
  //BaseType_t res = xTaskCreate(runWorker, name, 35000, (void*)name, 1, NULL);
  TaskHandle_t minerTask1, minerTask2 = NULL;
  xTaskCreate(runMiner, "Miner0", 6000, (void*)0, 1, &minerTask1);
  esp_task_wdt_add(minerTask1);

#if (SOC_CPU_CORES_NUM >= 2)
  xTaskCreate(runMiner, "Miner1", 6000, (void*)1, 1, &minerTask2);
  esp_task_wdt_add(minerTask2);
#endif

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
  touchHandler.isTouched();
#endif
  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code

  vTaskDelay(50 / portTICK_PERIOD_MS);
}
