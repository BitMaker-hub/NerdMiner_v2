#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <algorithm>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include "media/Free_Fonts.h"
#include "media/images.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "OpenFontRender.h"
#include "stratum.h"
#include "mining.h"
#include "utils.h"
#include "monitor.h"

unsigned long templates = 0;
unsigned long hashes= 0;
unsigned long Mhashes = 0;
unsigned long totalKHashes = 0;
unsigned long elapsedKHs = 0;

unsigned long halfshares; // increase if blockhash has 16 bits of zeroes
unsigned int shares; // increase if blockhash has 32 bits of zeroes
unsigned int valids; // increased if blockhash <= target

// Variables to hold data from custom textboxes
extern char poolString[80];
extern int portNumber;
extern char btcString[80];

extern OpenFontRender render;
extern TFT_eSprite background;

//Global work data 
static WiFiClient client;
static miner_data mMiner; //Global miner data (Create a miner class TODO)
mining_subscribe mWorker;
mining_job mJob;
monitor_data mMonitor;

bool isMinerSuscribed = false;

void checkPoolConnection(bool* isMinerSuscribed) {
  
  if (client.connected()) {
    return;
  }
  
  *isMinerSuscribed = false;
  Serial.println("Client not connected, trying to connect..."); 
  IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres
  WiFi.hostByName(poolString, serverIP);
  Serial.printf("Resolved DNS got: %s\n", serverIP.toString());
  if (!client.connect(poolString, portNumber)) {
    Serial.println("Imposible to connect to : " + String(poolString));
    WiFi.hostByName(poolString, serverIP);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

//Checks if pool is not sending any data and reconnects again
//verifies it even connection is alive, because pool could be stopped sending NOTIFY
unsigned long mStart0Hashrate = 0;
bool checkPoolInactivity(unsigned long inactivityTime){

    unsigned long currentKHashes = (Mhashes*1000) + hashes/1000;
    unsigned long elapsedKHs = currentKHashes - totalKHashes; 

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
  Serial.printf("### [Total Heap / Free heap]: %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap());
  #endif

  // connect to pool
  
  float currentPoolDifficulty = atof(DEFAULT_DIFFICULTY);

  while(true) {
      
    if(WiFi.status() != WL_CONNECTED){
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    } 

    //Test vars:
    //************
    //Nerdminerpool
    // strcpy(poolString, "nerdminerPool"); 
    // portNumber = 3002;
    // strcpy(btcString,"test");
    //Braiins
    //strcpy(poolString, "eu.stratum.braiins.com");
    //portNumber = 3333;
    //strcpy(btcString,"Bitmaker.01");
    //CKpool
    strcpy(poolString, "solo.ckpool.org");
    portNumber = 3333;
    //strcpy(btcString,"test");

    checkPoolConnection(&isMinerSuscribed);

    if(!isMinerSuscribed){

      mWorker = init_mining_subscribe();

      // STEP 1: Pool server connection (SUBSCRIBE)
      if(!tx_mining_subscribe(client, mWorker)) { 
        client.stop();
        continue; 
      }
      
      strcpy(mWorker.wName, btcString);
      strcpy(mWorker.wPass, "x");
      // STEP 2: Pool authorize work (Block Info)
      tx_mining_auth(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO
      //tx_mining_auth2(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO

      // STEP 3: Suggest pool difficulty
      tx_suggest_difficulty(client, DEFAULT_DIFFICULTY);

      isMinerSuscribed=true;
    }

    //Check if pool is down for almost 5minutes and then restart connection with pool (5min=300000ms)
    if(checkPoolInactivity(120000)){
      //Restart connection
      Serial.println("  Detected more than 2 min without data form stratum server. Closing socket and reopening...");
      client.stop();
      isMinerSuscribed=false;
      continue; 
    }

    //Read pending messages from pool
    while(client.connected() && client.available()){

      Serial.println("  Received message from pool");
      String line = client.readStringUntil('\n');
      stratum_method result = parse_mining_method(line);
      switch (result)
      {
          case STRATUM_PARSE_ERROR:   Serial.println("  Parsed JSON: error on JSON"); break;
          case MINING_NOTIFY:         if(parse_mining_notify(line, mJob)){
                                          //Increse templates readed
                                          templates++;
                                          //Stop miner current job
                                          mMiner.inRun = false;
                                          //Prepare data for new job
                                          mMiner=calculateMiningData(mWorker,mJob);
                                          mMiner.poolDifficulty = currentPoolDifficulty;
                                          mMiner.newJob = true;
                                          //Give new job to miner

                                      }
                                      break;
          case MINING_SET_DIFFICULTY: parse_mining_set_difficulty(line, currentPoolDifficulty);
                                      mMiner.poolDifficulty = currentPoolDifficulty;
                                      break;
          default:                    Serial.println("  Parsed JSON: unknown"); break;

      }
    }

    vTaskDelay(200 / portTICK_PERIOD_MS); //Small delay
    
  }
  
}


//////////////////THREAD CALLS///////////////////

//This works only with one thread, TODO -> Class or miner_data for each thread

void runMiner(void * name){
  
  while(1){

    //Wait new job
    while(1){
      if(mMiner.newJob==true) break;
      vTaskDelay(100 / portTICK_PERIOD_MS); //Small delay
    }

    mMiner.newJob = false; //Clear newJob flag
    mMiner.inRun = true; //Set inRun flag

    //Prepare Premining data
    mbedtls_sha256_context midstate[32];
    unsigned char hash[32];
    mbedtls_sha256_context ctx;

    //Calcular midstate
    mbedtls_sha256_init(midstate); 
    mbedtls_sha256_starts_ret(midstate, 0);
    mbedtls_sha256_update_ret(midstate, mMiner.bytearray_blockheader, 64);

    // search a valid nonce
    unsigned long nonce = TARGET_NONCE - MAX_NONCE;
    uint32_t startT = micros();
    unsigned char *header64 = mMiner.bytearray_blockheader + 64;
    Serial.println(">>> STARTING TO HASH NONCES");
    while(true) {
      memcpy(mMiner.bytearray_blockheader + 76, &nonce, 4);

      //Con midstate
      // Primer SHA-256
      mbedtls_sha256_clone(&ctx, midstate); //Clonamos el contexto anterior para continuar el SHA desde allí
      mbedtls_sha256_update_ret(&ctx, header64, 16);
      mbedtls_sha256_finish_ret(&ctx, hash);

      // Segundo SHA-256
      mbedtls_sha256_starts_ret(&ctx, 0);
      mbedtls_sha256_update_ret(&ctx, hash, 32);
      mbedtls_sha256_finish_ret(&ctx, hash);
      /*for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash[i]);
        Serial.println("");   */
      
      hashes++;
      if (nonce++> TARGET_NONCE) break; //exit
      if(!mMiner.inRun) { Serial.println ("MINER WORK ABORTED >> waiting new job"); break;}

      // check if 16bit share
      if(hash[31] !=0 || hash[30] !=0) continue;
      halfshares++;
      
      //Check target to submit
      //Difficulty of 1 > 0x00000000FFFF0000000000000000000000000000000000000000000000000000
      //NM2 pool diff 1e-9 > Target = diff_1 / diff_pool > 0x00003B9ACA00....00
      //Swapping diff bytes little endian >>>>>>>>>>>>>>>> 0x0000DC59D300....00  
      //if((hash[29] <= 0xDC) && (hash[28] <= 0x59))     //0x00003B9ACA00  > diff value for 1e-9
      double diff_hash = diff_from_target(hash);
      if(diff_hash > mMiner.poolDifficulty)//(hash[29] <= 0x3B)//(diff_hash > 1e-9)
      {
        tx_mining_submit(client, mWorker, mJob, nonce);
        Serial.print("   - Current diff share: "); Serial.println(diff_hash,12);
        Serial.print("   - Current pool diff : "); Serial.println(mMiner.poolDifficulty,12);
        Serial.print("   - TX SHARE: ");
        for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash[i]);
        Serial.println("");   
      }
      
      // check if 32bit share
      if(hash[29] !=0 || hash[28] !=0) continue;
      shares++;

        // check if valid header
      if(checkValid(hash, mMiner.bytearray_target)){
        Serial.printf("[WORKER] %s CONGRATULATIONS! Valid completed with nonce: %d | 0x%x\n", (char *)name, nonce, nonce);
        valids++;
        Serial.printf("[WORKER]  %s  Submiting work valid!\n", (char *)name);
        // STEP 3: Submit mining job
        tx_mining_submit(client, mWorker, mJob, nonce);
        client.stop();
        // exit 
        nonce = MAX_NONCE;
        break;
      }    
    } // exit if found a valid result or nonce > MAX_NONCE

    mbedtls_sha256_free(&ctx);
    mbedtls_sha256_free(midstate);

    mMiner.inRun = false;

    if(hashes>=MAX_NONCE) { 
      Mhashes=Mhashes+MAX_NONCE/1000000; 
      hashes=hashes-MAX_NONCE;
    }

    uint32_t duration = micros() - startT;
  }
}

void runMonitor(void *name){

  Serial.println("[MONITOR] started");
  
  unsigned long mLastCheck = 0;

  while(1){
    
    
    unsigned long mElapsed = millis()-mLastCheck;
    mLastCheck = millis();
    unsigned long currentKHashes = (Mhashes*1000) + hashes/1000;
    elapsedKHs = currentKHashes - totalKHashes; 
    totalKHashes = currentKHashes;
    
    switch(mMonitor.screen){
      case SCREEN_MINING: show_MinerScreen(mElapsed); break;
    }
    
    //Monitor state when hashrate is 0.0
    if(elapsedKHs == 0) {
      Serial.printf(">>> [i] Miner: newJob>%s / inRun>%s) - Client: connected>%s / subscribed>%s / wificonnected>%s\n",
      mMiner.newJob ? "true" : "false", mMiner.inRun ? "true" : "false", 
      client.connected() ? "true" : "false", isMinerSuscribed ? "true" : "false", WiFi.status() == WL_CONNECTED ? "true" : "false");
    }

    // Pause the task for 5000ms
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

