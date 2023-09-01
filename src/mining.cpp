#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "ShaTests/nerdSHA256.h"
//#include "ShaTests/nerdSHA256plus.h"
#include "stratum.h"
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "drivers/display.h"

unsigned long templates = 0;
unsigned long hashes= 0;
unsigned long Mhashes = 0;
unsigned long totalKHashes = 0;
unsigned long elapsedKHs = 0;

unsigned int shares; // increase if blockhash has 32 bits of zeroes
unsigned int valids; // increased if blockhash <= target

// Track best diff
double best_diff = 0.0;

// Variables to hold data from custom textboxes
extern char poolString[80];
extern int portNumber;
extern char btcString[80];
IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres

//Global work data 
static WiFiClient client;
static miner_data mMiner; //Global miner data (Create a miner class TODO)
mining_subscribe mWorker;
mining_job mJob;
monitor_data mMonitor;
bool isMinerSuscribed = false;
unsigned long mLastTXtoPool = millis();

bool checkPoolConnection(void) {
  
  if (client.connected()) {
    return true;
  }
  
  isMinerSuscribed = false;

  Serial.println("Client not connected, trying to connect..."); 
  
  //Resolve first time pool DNS and save IP
  if(serverIP == IPAddress(1,1,1,1)) {
    WiFi.hostByName(poolString, serverIP);
    Serial.printf("Resolved DNS and save ip (first time) got: %s\n", serverIP.toString());
  }

  //Try connecting pool IP
  if (!client.connect(serverIP, portNumber)) {
    Serial.println("Imposible to connect to : " + String(poolString));
    WiFi.hostByName(poolString, serverIP);
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
        tx_suggest_difficulty(client, DEFAULT_DIFFICULTY);
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
  Serial.printf("### [Total Heap / Free heap]: %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap());
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
    //strcpy(poolString, "solo.ckpool.org");
    //portNumber = 3333;
    //strcpy(btcString,"test");

    if(!checkPoolConnection())
      //If server is not reachable add random delay for connection retries
      srand(millis());
      //Generate value between 1 and 15 secs
      vTaskDelay(((1 + rand() % 15) * 1000) / portTICK_PERIOD_MS);

    if(!isMinerSuscribed){

      //Stop miner current jobs
      mMiner.inRun = false;
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
      mLastTXtoPool = millis();
    }

    //Check if pool is down for almost 5minutes and then restart connection with pool (1min=600000ms)
    if(checkPoolInactivity(KEEPALIVE_TIME_ms, POOLINACTIVITY_TIME_ms)){
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
                                          //Stop miner current jobs
                                          mMiner.inRun = false;
                                          //Prepare data for new jobs
                                          mMiner=calculateMiningData(mWorker,mJob);
                                          mMiner.poolDifficulty = currentPoolDifficulty;
                                          mMiner.newJob = true;
                                          mMiner.newJob2 = true;
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


//////////////////THREAD CALLS///////////////////

//This works only with one thread, TODO -> Class or miner_data for each thread

  
void runMiner(void * task_id) {

  unsigned int miner_id = (uint32_t)task_id;

  Serial.printf("[MINER]  %d  Started runMiner Task!\n", miner_id);

  while(1){

    //Wait new job
    while(1){
      if(mMiner.newJob==true || mMiner.newJob2==true) break;
      vTaskDelay(100 / portTICK_PERIOD_MS); //Small delay
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); //Small delay to join both mining threads

    if(mMiner.newJob)
      mMiner.newJob = false; //Clear newJob flag
    else if(mMiner.newJob2)
      mMiner.newJob2 = false; //Clear newJob flag
    mMiner.inRun = true; //Set inRun flag

    mMonitor.NerdStatus = NM_hashing;

    //Prepare Premining data
    nerd_sha256 nerdMidstate;
    //nerdSHA256_context nerdMidstate; //NerdShaplus
    uint8_t hash[32];
    

    //Calcular midstate
    nerd_midstate(&nerdMidstate, mMiner.bytearray_blockheader, 64);
    //nerd_mids(&nerdMidstate, mMiner.bytearray_blockheader); //NerdShaplus


    // search a valid nonce
    unsigned long nonce = TARGET_NONCE - MAX_NONCE;
    // split up odd/even nonces between miner tasks
    nonce += miner_id;
    uint32_t startT = micros();
    unsigned char *header64;
    // each miner thread needs to track its own blockheader template
    uint8_t temp;

    memcpy(mMiner.bytearray_blockheader2, &mMiner.bytearray_blockheader, 80);
    if (miner_id == 0)
      header64 = mMiner.bytearray_blockheader + 64;
    else
      header64 = mMiner.bytearray_blockheader2 + 64;
    
    bool is16BitShare=true;  
    Serial.println(">>> STARTING TO HASH NONCES");
    while(true) {
      if (miner_id == 0)
        memcpy(mMiner.bytearray_blockheader + 76, &nonce, 4);
      else
        memcpy(mMiner.bytearray_blockheader2 + 76, &nonce, 4);


      nerd_double_sha2(&nerdMidstate, header64, hash);
      //is16BitShare=nerd_sha256d(&nerdMidstate, header64, hash); //Boosted 80Khs sha

      /*Serial.print("hash1: ");
      for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash[i]);
        Serial.println("");  
      Serial.print("hash2: ");
      for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", hash2[i]);
        Serial.println("");  */

      hashes++;
      if (nonce > TARGET_NONCE) break; //exit
      if(!mMiner.inRun) { Serial.println ("MINER WORK ABORTED >> waiting new job"); break;}

      // check if 16bit share
      if(hash[31] !=0 || hash[30] !=0) {
      //if(!is16BitShare){
        // increment nonce
        nonce += 2;
        continue;
      }

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
        tx_mining_submit(client, mWorker, mJob, nonce);
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
      if(hash[29] !=0 || hash[28] !=0) {
        // increment nonce
        nonce += 2;
        continue;
      }
      shares++;

      // check if valid header
      if(checkValid(hash, mMiner.bytearray_target)){
        Serial.printf("[WORKER] %d CONGRATULATIONS! Valid block found with nonce: %d | 0x%x\n", miner_id, nonce, nonce);
        valids++;
        Serial.printf("[WORKER]  %d  Submitted work valid!\n", miner_id);
        // wait for new job
        break;
      }
      // increment nonce
      nonce += 2;
    } // exit if found a valid result or nonce > MAX_NONCE

    //wc_Sha256Free(&sha256);
    //wc_Sha256Free(midstate);

    mMiner.inRun = false;
    Serial.print(">>> Finished job waiting new data from pool");

    if(hashes>=MAX_NONCE_STEP) {
      Mhashes=Mhashes+MAX_NONCE_STEP/1000000;
      hashes=hashes-MAX_NONCE_STEP;
    }

    uint32_t duration = micros() - startT;
    if (esp_task_wdt_reset() == ESP_OK)
      Serial.print(">>> Resetting watchdog timer");
  }
}

#define DELAY 100
#define REDRAW_EVERY 10

void runMonitor(void *name)
{

  Serial.println("[MONITOR] started");

  unsigned long mLastCheck = 0;

  resetToFirstScreen();

  unsigned long frame = 0;

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
                      mMiner.newJob ? "true" : "false", mMiner.inRun ? "true" : "false",
                      client.connected() ? "true" : "false", isMinerSuscribed ? "true" : "false", WiFi.status() == WL_CONNECTED ? "true" : "false");
      }
    }
    animateCurrentScreen(frame);
    doLedStuff(frame);

    // Pause the task for 1000ms
    vTaskDelay(DELAY / portTICK_PERIOD_MS);
    frame++;
  }
}
