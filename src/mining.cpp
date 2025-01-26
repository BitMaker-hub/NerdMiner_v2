#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <nvs.h>
//#include "ShaTests/nerdSHA256.h"
#include "ShaTests/nerdSHA256plus.h"
#include "stratum.h"
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "timeconst.h"
#include "drivers/displays/display.h"
#include "drivers/storage/storage.h"
#include <mutex>
#include "mbedtls/sha256.h"

//#define SHA256_VALIDATE
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
#define HARDWARE_SHA265
#endif

#ifdef HARDWARE_SHA265
#include <sha/sha_dma.h>
#include <hal/sha_hal.h>
#include <hal/sha_ll.h>
#endif

nvs_handle_t stat_handle;

uint32_t templates = 0;
uint32_t hashes = 0;
uint32_t Mhashes = 0;
uint32_t totalKHashes = 0;
uint32_t elapsedKHs = 0;
uint64_t upTime = 0;

volatile uint32_t shares; // increase if blockhash has 32 bits of zeroes
volatile uint32_t valids; // increased if blockhash <= target

static std::mutex s_nonce_batch_mutex;
static volatile uint32_t s_nonce_batch = 0;

static volatile uint8_t s_thread_busy[2] = {0, 0};
static volatile uint32_t s_thread_task_id = 0;
static volatile uint32_t s_thread_task_aborted_id = 0;

// Track best diff
double best_diff = 0.0;

// Variables to hold data from custom textboxes
//Track mining stats in non volatile memory
extern TSettings Settings;

IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres

//Global work data 
static WiFiClient client;
static std::mutex s_client_mutex;
static miner_data mMiner; //Global miner data (Create a miner class TODO)
mining_subscribe mWorker;
mining_job mJob;
monitor_data mMonitor;
bool isMinerSuscribed = false;
unsigned long mLastTXtoPool = millis();

int saveIntervals[7] = {5 * 60, 15 * 60, 30 * 60, 1 * 3600, 3 * 3600, 6 * 3600, 12 * 3600};
int saveIntervalsSize = sizeof(saveIntervals)/sizeof(saveIntervals[0]);
int currentIntervalIndex = 0;

bool checkPoolConnection(void) {
  
  if (client.connected()) {
    return true;
  }
  
  isMinerSuscribed = false;

  Serial.println("Client not connected, trying to connect..."); 
  
  //Resolve first time pool DNS and save IP
  if(serverIP == IPAddress(1,1,1,1)) {
    WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP);
    Serial.printf("Resolved DNS and save ip (first time) got: %s\n", serverIP.toString());
  }

  //Try connecting pool IP
  if (!client.connect(serverIP, Settings.PoolPort)) {
    Serial.println("Imposible to connect to : " + Settings.PoolAddress);
    WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP);
    Serial.printf("Resolved DNS got: %s\n", serverIP.toString());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return false;
  }

  return true;
}

//Implements a socketKeepAlive function and 
//checks if pool is not sending any data to reconnect again.
//Even connection could be alive, pool could stop sending new job NOTIFY
unsigned long mStart0Hashrate = 0;
bool checkPoolInactivity(unsigned int keepAliveTime, unsigned long inactivityTime){ 

    unsigned long currentKHashes = (Mhashes*1000) + hashes/1000;
    unsigned long elapsedKHs = currentKHashes - totalKHashes; 

    // If no shares sent to pool
    // send something to pool to hold socket oppened
    if(millis() - mLastTXtoPool > keepAliveTime){
      mLastTXtoPool = millis();
      Serial.println("  Sending  : KeepAlive suggest_difficulty");
      //if (client.print("{}\n") == 0) {
      {
        std::lock_guard<std::mutex> lock(s_client_mutex);
        tx_suggest_difficulty(client, DEFAULT_DIFFICULTY);
      }
      /*if(tx_suggest_difficulty(client, DEFAULT_DIFFICULTY)){
        Serial.println("  Sending keepAlive to pool -> Detected client disconnected");
        return true;
      }*/
    }

    if(elapsedKHs == 0){
      //Check if hashrate is 0 during inactivityTIme
      if(mStart0Hashrate == 0) mStart0Hashrate  = millis(); 
      if((millis()-mStart0Hashrate) > inactivityTime) { mStart0Hashrate=0; return true;}
      return false;
    }

  mStart0Hashrate = 0;
  return false;
}

void runStratumWorker(void *name) {

// TEST: https://bitcoin.stackexchange.com/questions/22929/full-example-data-for-scrypt-stratum-client

  Serial.println("");
  Serial.printf("\n[WORKER] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());

  #ifdef DEBUG_MEMORY
  Serial.printf("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
  #endif

  // connect to pool
  
  double currentPoolDifficulty = DEFAULT_DIFFICULTY;

  while(true) {
      
    if(WiFi.status() != WL_CONNECTED){
      // WiFi is disconnected, so reconnect now
      mMonitor.NerdStatus = NM_Connecting;
      WiFi.reconnect();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    } 

    if(!checkPoolConnection()){
      //If server is not reachable add random delay for connection retries
      srand(millis());
      //Generate value between 1 and 120 secs
      vTaskDelay(((1 + rand() % 120) * 1000) / portTICK_PERIOD_MS);
    }

    if(!isMinerSuscribed){

      //Stop miner current jobs
      s_thread_task_aborted_id = s_thread_task_id;
      mWorker = init_mining_subscribe();

      // STEP 1: Pool server connection (SUBSCRIBE)
      if(!tx_mining_subscribe(client, mWorker)) { 
        client.stop();
        continue; 
      }
      
      strcpy(mWorker.wName, Settings.BtcWallet);
      strcpy(mWorker.wPass, Settings.PoolPassword);
      // STEP 2: Pool authorize work (Block Info)
      tx_mining_auth(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO
      //tx_mining_auth2(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO

      // STEP 3: Suggest pool difficulty
      tx_suggest_difficulty(client, DEFAULT_DIFFICULTY);

      isMinerSuscribed=true;
      mLastTXtoPool = millis();
    }

    //Check if pool is down for almost 5minutes and then restart connection with pool (1min=600000ms)
    if(checkPoolInactivity(KEEPALIVE_TIME_ms, POOLINACTIVITY_TIME_ms)){
      //Restart connection
      Serial.println("  Detected more than 2 min without data form stratum server. Closing socket and reopening...");
      {
        std::lock_guard<std::mutex> lock(s_client_mutex);
        client.stop();
      }
      s_thread_task_aborted_id = s_thread_task_id;
      isMinerSuscribed=false;
      continue; 
    }

    //Read pending messages from pool
    while(true)
    {
      String line;
      {
        std::lock_guard<std::mutex> lock(s_client_mutex);
        if (!client.connected() || !client.available())
          break;
        line = client.readStringUntil('\n');
      }

      Serial.println("  Received message from pool");      
      stratum_method result = parse_mining_method(line);
      switch (result)
      {
          case STRATUM_PARSE_ERROR:   Serial.println("  Parsed JSON: error on JSON"); break;
          case MINING_NOTIFY:         if(parse_mining_notify(line, mJob))
                                      {
                                          //Increse templates readed
                                          templates++;
                                          //Stop miner current jobs
                                          //mMiner.inRun = false;
                                          s_thread_task_aborted_id = s_thread_task_id;

                                          #if (SOC_CPU_CORES_NUM >= 2)
                                            while (s_thread_busy[0] || s_thread_busy[1]) { vTaskDelay(1 / portTICK_PERIOD_MS); }
                                          #else
                                            while (s_thread_busy[0]) { vTaskDelay(1 / portTICK_PERIOD_MS); }
                                          #endif

                                          //Prepare data for new jobs
                                          mMiner=calculateMiningData(mWorker,mJob);
                                          mMiner.poolDifficulty = currentPoolDifficulty;
                                          {
                                            std::lock_guard<std::mutex> lock(s_nonce_batch_mutex);
                                            s_nonce_batch = TARGET_NONCE - MAX_NONCE;
                                          }
                                          s_thread_task_id++;
                                          //Give new job to miner
                                      }
                                      break;
          case MINING_SET_DIFFICULTY: parse_mining_set_difficulty(line, currentPoolDifficulty);
                                      mMiner.poolDifficulty = currentPoolDifficulty;
                                      break;
          case STRATUM_SUCCESS:       Serial.println("  Parsed JSON: Success"); break;
          default:                    Serial.println("  Parsed JSON: unknown"); break;

      }
    }

    vTaskDelay(500 / portTICK_PERIOD_MS); //Small delay
    
  }
  
}

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
static inline void nerd_sha_ll_fill_text_block_sha256(const void *input_text)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    REG_WRITE(&reg_addr_buf[0], data_words[0]);
    REG_WRITE(&reg_addr_buf[1], data_words[1]);
    REG_WRITE(&reg_addr_buf[2], data_words[2]);
    REG_WRITE(&reg_addr_buf[3], data_words[3]);
    REG_WRITE(&reg_addr_buf[4], data_words[4]);
    REG_WRITE(&reg_addr_buf[5], data_words[5]);
    REG_WRITE(&reg_addr_buf[6], data_words[6]);
    REG_WRITE(&reg_addr_buf[7], data_words[7]);
    REG_WRITE(&reg_addr_buf[8], data_words[8]);
    REG_WRITE(&reg_addr_buf[9], data_words[9]);
    REG_WRITE(&reg_addr_buf[10], data_words[10]);
    REG_WRITE(&reg_addr_buf[11], data_words[11]);
    REG_WRITE(&reg_addr_buf[12], data_words[12]);
    REG_WRITE(&reg_addr_buf[13], data_words[13]);
    REG_WRITE(&reg_addr_buf[14], data_words[14]);
    REG_WRITE(&reg_addr_buf[15], data_words[15]);
}

static inline void nerd_sha_hal_wait_idle()
{
    while (sha_ll_busy())
    {}
}

#endif
//////////////////THREAD CALLS///////////////////

//This works only with one thread, TODO -> Class or miner_data for each thread

  
void runMiner(void * task_id) {

  unsigned int miner_id = (uint32_t)task_id;

  Serial.printf("[MINER]  %d  Started runMiner Task!\n", miner_id);

  uint32_t task_current_id = 0;

  while(1)
  {
    //Wait new job
    s_thread_busy[miner_id] = 0;
    while (task_current_id == s_thread_task_id)
      vTaskDelay(1 / portTICK_PERIOD_MS); //Small delay to join both mining threads

    task_current_id = s_thread_task_id;
    s_thread_busy[miner_id] = 1;

    Serial.printf("[MINER]  %d  Task=%d\n", miner_id, task_current_id);
    
    mMonitor.NerdStatus = NM_hashing;

    //Prepare Premining data
    nerdSHA256_context nerdMidstate; //NerdShaplus
    uint8_t hash[32];
    uint8_t interResult[64];
    uint8_t hash_validate[32];
    uint8_t midstate[32];
    uint32_t bake[16];
    

    unsigned char *header64;
    //Calcular midstate
    if (miner_id == 0)
    {
      #ifdef HARDWARE_SHA265
      esp_sha_acquire_hardware();
      sha_hal_hash_block(SHA2_256,  mMiner.bytearray_blockheader, 64/4, true);
      sha_hal_read_digest(SHA2_256, midstate);
      esp_sha_release_hardware();

      memset(mMiner.bytearray_blockheader+80, 0, 128-80);
      mMiner.bytearray_blockheader[80] = 0x80;
      mMiner.bytearray_blockheader[126] = 0x02;
      mMiner.bytearray_blockheader[127] = 0x80;

      memset(interResult, 0, sizeof(interResult));
      interResult[32] = 0x80;
      interResult[62] = 0x01;
      interResult[63] = 0x00;

      #else
      nerd_mids(&nerdMidstate, mMiner.bytearray_blockheader); //NerdShaplus
      #endif
      header64 = mMiner.bytearray_blockheader + 64;
    } else
    {
      memcpy(mMiner.bytearray_blockheader2, &mMiner.bytearray_blockheader, 80);
      nerd_mids(&nerdMidstate, mMiner.bytearray_blockheader2); //NerdShaplus
      header64 = mMiner.bytearray_blockheader2 + 64;
      nerd_sha256_bake(nerdMidstate.digest, header64, bake);
    }

    uint32_t nonce = 0;
    uint32_t nonce_end = 0;
    uint32_t startT = micros();
    
    // each miner thread needs to track its own blockheader template
    uint8_t temp;   
    
    bool is16BitShare=true;  
    Serial.println(">>> STARTING TO HASH NONCES");
    while(true)
    {
      if (nonce >= nonce_end)
      {
        std::lock_guard<std::mutex> lock(s_nonce_batch_mutex);
        nonce = s_nonce_batch;
#ifdef HARDWARE_SHA265        
        if (miner_id == 0)
          nonce_end = nonce + 512;
        else
#endif
        nonce_end = nonce + 128;
        if (nonce_end > TARGET_NONCE)
          nonce_end = TARGET_NONCE;
        s_nonce_batch = nonce_end;
      }

#ifdef HARDWARE_SHA265
      if (miner_id == 0)
      {
        //Hardware
        uint32_t nonce_start = nonce;
        esp_sha_acquire_hardware();
        while (nonce < nonce_end)
        {
          //memcpy(header64+12, &nonce, 4);
          ((uint32_t*)(header64+12))[0] = nonce;
          
          sha_ll_write_digest(SHA2_256, midstate, 256 / 32);
          //sha_hal_wait_idle();
          nerd_sha_hal_wait_idle();
          //sha_ll_fill_text_block(header64, 64/4);
          nerd_sha_ll_fill_text_block_sha256(header64);
          sha_ll_continue_block(SHA2_256);
      
          sha_ll_load(SHA2_256);
          //sha_hal_wait_idle();
          nerd_sha_hal_wait_idle();
          sha_ll_read_digest(SHA2_256, interResult, 256 / 32);
      
          //sha_hal_wait_idle();
          nerd_sha_hal_wait_idle();
          //sha_ll_fill_text_block(interResult, 64/4);
          nerd_sha_ll_fill_text_block_sha256(interResult);
          sha_ll_start_block(SHA2_256);

          sha_ll_load(SHA2_256);
          //sha_hal_wait_idle();
          nerd_sha_hal_wait_idle();
          sha_ll_read_digest(SHA2_256, hash, 256 / 32);
          #ifdef SHA256_VALIDATE
          mbedtls_sha256_context ctx;
          mbedtls_sha256_init(&ctx);
          mbedtls_sha256_starts_ret(&ctx,0);
          mbedtls_sha256_update_ret(&ctx, header64-64, 80);
          mbedtls_sha256_finish_ret(&ctx, interResult);

          mbedtls_sha256_starts_ret(&ctx,0);
          mbedtls_sha256_update_ret(&ctx, interResult, 32);
          mbedtls_sha256_finish_ret(&ctx, hash_validate);
          mbedtls_sha256_free(&ctx);

          bool failed = false;
          for (size_t i = 0; i < 32; i++)
          {
            if (hash[i] != hash_validate[i])
              failed = true;
          }
          if (failed)
          {
            Serial.println("Hardware SHA256 Failed");
            Serial.println("HwSha256:");
            for (size_t i = 0; i < 32; i++)
              Serial.printf("%02x,",  hash[i]);
            Serial.println("");

            Serial.println("mbedtls Sha256:");
            for (size_t i = 0; i < 32; i++)
              Serial.printf("%02x,",  hash_validate[i]);              
            Serial.println("");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            return; //Crash Here
          }
          #endif
          nonce++;
          if(hash[31] == 0 && hash[30] == 0)
            break;
        }
        esp_sha_release_hardware();
        hashes += nonce - nonce_start;
      } else
#endif
      {
        while (nonce < nonce_end)
        {
          memcpy(header64+12, &nonce, 4);
          //nerd_sha256d(&nerdMidstate, header64, hash); //Boosted 80Khs sha
          nerd_sha256d_baked(nerdMidstate.digest, header64, bake, hash);

          #ifdef SHA256_VALIDATE
          //Important - Remove Return optimization
          mbedtls_sha256_context ctx;
          mbedtls_sha256_init(&ctx);
          mbedtls_sha256_starts_ret(&ctx,0);
          mbedtls_sha256_update_ret(&ctx, header64-64, 80);
          mbedtls_sha256_finish_ret(&ctx, interResult);

          mbedtls_sha256_starts_ret(&ctx,0);
          mbedtls_sha256_update_ret(&ctx, interResult, 32);
          mbedtls_sha256_finish_ret(&ctx, hash_validate);
          mbedtls_sha256_free(&ctx);

          bool failed = false;
          for (size_t i = 0; i < 32; i++)
          {
            if (hash[i] != hash_validate[i])
              failed = true;
          }
          if (failed)
          {
            Serial.println("SHA256 Failed");
            Serial.println("Input:");
            for (size_t i = 0; i < 80; i++)
            {
              Serial.printf("0x%02x,", (header64-64)[i]);
              if (i % 16 == 15)
                Serial.println("");
            }
            Serial.println("");

            Serial.println("Midstate:");
            for (size_t i = 0; i < 8; i++)
            {
              Serial.printf("0x%08x,",  nerdMidstate.digest[i]);
              Serial.println("");
            }

            Serial.println("NerdSha256:");
            for (size_t i = 0; i < 32; i++)
              Serial.printf("%02x,",  hash[i]);
            Serial.println("");

            Serial.println("mbedtls Sha256:");
            for (size_t i = 0; i < 32; i++)
              Serial.printf("%02x,",  hash_validate[i]);              
            Serial.println("");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            return; //Crash Here
          }
          #endif
          hashes++;
          nonce++;

          if(hash[31] == 0 && hash[30] == 0)
            break;
        }
      }

      /*Serial.print("hash1: ");
      for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash[i]);
        Serial.println("");  
      Serial.print("hash2: ");
      for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash2[i]);
        Serial.println("");  */

      if (nonce >= TARGET_NONCE)
        break; //exit

      
      if(task_current_id <= s_thread_task_aborted_id)
      {
        Serial.printf("MINER %d WORK ABORTED Task=%d Abort=%d\n", miner_id, task_current_id, s_thread_task_aborted_id);
        break;
      }

      // check if 16bit share
      if(hash[31] !=0 || hash[30] !=0)
        continue;

#if 0
      if (miner_id == 1)
      {
        //validate
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts_ret(&ctx,0);
        mbedtls_sha256_update_ret(&ctx, header64-64, 80);
        mbedtls_sha256_finish_ret(&ctx, interResult);

        mbedtls_sha256_starts_ret(&ctx,0);
        mbedtls_sha256_update_ret(&ctx, interResult, 32);
        mbedtls_sha256_finish_ret(&ctx, hash_validate);
        mbedtls_sha256_free(&ctx);
        
        bool failed = false;
        for (size_t i = 0; i < 32; i++)
        {
          if (hash[i] != hash_validate[i])
            failed = true;
        }
        if (failed)
          Serial.printf("MINER %d Sha256 Fail\n", miner_id);
        else
          Serial.printf("MINER %d Sha256 Good\n", miner_id);
      }
#endif
      //Check target to submit
      //Difficulty of 1 > 0x00000000FFFF0000000000000000000000000000000000000000000000000000
      //NM2 pool diff 1e-9 > Target = diff_1 / diff_pool > 0x00003B9ACA00....00
      //Swapping diff bytes little endian >>>>>>>>>>>>>>>> 0x0000DC59D300....00  
      //if((hash[29] <= 0xDC) && (hash[28] <= 0x59))     //0x00003B9ACA00  > diff value for 1e-9
      double diff_hash = diff_from_target(hash);

      // update best diff
      if (diff_hash > best_diff)
        best_diff = diff_hash;

      if(diff_hash > mMiner.poolDifficulty)//(hash[29] <= 0x3B)//(diff_hash > 1e-9)
      {
        {
          std::lock_guard<std::mutex> lock(s_client_mutex);
          tx_mining_submit(client, mWorker, mJob, nonce-1);
        }
        Serial.print("   - Current diff share: "); Serial.println(diff_hash,12);
        Serial.print("   - Current pool diff : "); Serial.println(mMiner.poolDifficulty,12);
        Serial.print("   - TX SHARE: ");
        for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash[i]);
        #ifdef DEBUG_MINING
        Serial.println("");
        Serial.print("   - Current nonce: "); Serial.println(nonce);
        Serial.print("   - Current block header: ");
        for (size_t i = 0; i < 80; i++) {
            Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
        }
        #endif
        Serial.println("");
        mLastTXtoPool = millis();  
      }
      
      // check if 32bit share
      if(hash[29] !=0 || hash[28] !=0)
        continue;
      shares++;

      // check if valid header
      if(checkValid(hash, mMiner.bytearray_target)){
        Serial.printf("[WORKER] %d CONGRATULATIONS! Valid block found with nonce: %d | 0x%x\n", miner_id, nonce, nonce);
        valids++;
        Serial.printf("[WORKER]  %d  Submitted work valid!\n", miner_id);
        // wait for new job
        break;
      }
    } // exit if found a valid result or nonce > MAX_NONCE

    //wc_Sha256Free(&sha256);
    //wc_Sha256Free(midstate);    
    Serial.print(">>> Finished job waiting new data from pool");

    if(hashes>=MAX_NONCE_STEP)
    {
      Mhashes=Mhashes+MAX_NONCE_STEP/1000000;
      hashes=hashes-MAX_NONCE_STEP;
    }

    uint32_t duration = micros() - startT;
    if (esp_task_wdt_reset() == ESP_OK)
      Serial.print(">>> Resetting watchdog timer");
  } //while (1)
}

#define DELAY 100
#define REDRAW_EVERY 10

void restoreStat() {
  if(!Settings.saveStats) return;
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.printf("[MONITOR] NVS partition is full or has invalid version, erasing...\n");
    nvs_flash_init();
  }

  ret = nvs_open("state", NVS_READWRITE, &stat_handle);

  size_t required_size = sizeof(double);
  nvs_get_blob(stat_handle, "best_diff", &best_diff, &required_size);
  nvs_get_u32(stat_handle, "Mhashes", &Mhashes);
  uint32_t nv_shares, nv_valids;
  nvs_get_u32(stat_handle, "shares", &nv_shares);
  nvs_get_u32(stat_handle, "valids", &nv_valids);
  shares = nv_shares;
  valids = nv_valids;
  nvs_get_u32(stat_handle, "templates", &templates);
  nvs_get_u64(stat_handle, "upTime", &upTime);
}

void saveStat() {
  if(!Settings.saveStats) return;
  Serial.printf("[MONITOR] Saving stats\n");
  nvs_set_blob(stat_handle, "best_diff", &best_diff, sizeof(double));
  nvs_set_u32(stat_handle, "Mhashes", Mhashes);
  nvs_set_u32(stat_handle, "shares", shares);
  nvs_set_u32(stat_handle, "valids", valids);
  nvs_set_u32(stat_handle, "templates", templates);
  nvs_set_u64(stat_handle, "upTime", upTime + (esp_timer_get_time()/1000000));
}

void resetStat() {
    Serial.printf("[MONITOR] Resetting NVS stats\n");
    templates = hashes = Mhashes = totalKHashes = elapsedKHs = upTime = shares = valids = 0;
    best_diff = 0.0;
    saveStat();
}

void runMonitor(void *name)
{

  Serial.println("[MONITOR] started");
  restoreStat();

  unsigned long mLastCheck = 0;

  resetToFirstScreen();

  unsigned long frame = 0;

  uint32_t seconds_elapsed = 0;

  totalKHashes = (Mhashes * 1000) + hashes / 1000;;

  while (1)
  {
    if ((frame % REDRAW_EVERY) == 0)
    {
      unsigned long mElapsed = millis() - mLastCheck;
      mLastCheck = millis();
      unsigned long currentKHashes = (Mhashes * 1000) + hashes / 1000;
      elapsedKHs = currentKHashes - totalKHashes;
      totalKHashes = currentKHashes;

      drawCurrentScreen(mElapsed);

      // Monitor state when hashrate is 0.0
      if (elapsedKHs == 0)
      {
        Serial.printf(">>> [i] Miner: newJob>%s / inRun>%s) - Client: connected>%s / subscribed>%s / wificonnected>%s\n",
            (s_thread_task_id != s_thread_task_aborted_id) ? "true" : "false", s_thread_busy[0] ? "true" : "false",
            client.connected() ? "true" : "false", isMinerSuscribed ? "true" : "false", WiFi.status() == WL_CONNECTED ? "true" : "false");
      }

      #ifdef DEBUG_MEMORY
      Serial.printf("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
      Serial.printf("### Max stack usage: %d\n", uxTaskGetStackHighWaterMark(NULL));
      #endif

      seconds_elapsed++;

      if(seconds_elapsed % (saveIntervals[currentIntervalIndex]) == 0){
        saveStat();
        seconds_elapsed = 0;
        if(currentIntervalIndex < saveIntervalsSize - 1)
          currentIntervalIndex++;
      }    
    }
    animateCurrentScreen(frame);
    doLedStuff(frame);

    vTaskDelay(DELAY / portTICK_PERIOD_MS);
    frame++;
  }
}
