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
#include "mining.h"

#define TARGET_BUFFER_SIZE 64
#define BUFFER_JSON_DOC 4096

static unsigned long templates = 0;
static unsigned long hashes= 0;
static unsigned long Mhashes = 0;
static unsigned long totalKHashes = 0;

static int halfshares; // increase if blockhash has 16 bits of zeroes
static int shares; // increase if blockhash has 32 bits of zeroes
static int valids; // increased if blockhash <= target
bool enableGlobalHash = false;

// Variables to hold data from custom textboxes
extern char poolString[80];
extern int portNumber;
extern char btcString[80];

extern OpenFontRender render;
extern TFT_eSprite background;



bool checkValid(unsigned char* hash, unsigned char* target) {
  bool valid = true;
  for(uint8_t i=31; i>=0; i--) {
    if(hash[i] > target[i]) {
      valid = false;
      break;
    } else if (hash[i] < target[i]) {
      valid = true;
      break;
    }
  }
  #ifdef DEBUG_MINING
  if (valid) {
    Serial.print("\tvalid : ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x ", hash[i]);
    Serial.println();
  }
  #endif
  return valid;
}

uint8_t hex(char ch) {
    uint8_t r = (ch > 57) ? (ch - 55) : (ch - 48);
    return r & 0x0F;
}

int to_byte_array(const char *in, size_t in_size, uint8_t *out) {
    int count = 0;
    if (in_size % 2) {
        while (*in && out) {
            *out = hex(*in++);
            if (!*in)
                return count;
            *out = (*out << 4) | hex(*in++);
            *out++;
            count++;
        }
        return count;
    } else {
        while (*in && out) {
            *out++ = (hex(*in++) << 4) | hex(*in++);
            count++;
        }
        return count;
    }
}

bool verifyPayload (String* line){
  if(line->length() == 0) return false;
  line->trim();
  if(line->isEmpty()) return false;
  return true;
}

unsigned long getNextId(unsigned long id) {
    if (id == ULONG_MAX) {
      id = 1;
      return id;
    }
    return ++id;
}

void getNextExtranonce2(int extranonce2_size, char *extranonce2) {
  
  unsigned long extranonce2_number = strtoul(extranonce2, NULL, 10);
  extranonce2_number++;
  
  memset(extranonce2, '0', 2 * extranonce2_size);
  if (extranonce2_number > long(pow(10, 2 * extranonce2_size))) {
    return;
  }
  
  char next_extranounce2[2 * extranonce2_size + 1];
  memset(extranonce2, '0', 2 * extranonce2_size);
  ultoa(extranonce2_number, next_extranounce2, 10);
  memcpy(extranonce2 + (2 * extranonce2_size) - long(log10(extranonce2_number)) - 1 , next_extranounce2, strlen(next_extranounce2));
  extranonce2[2 * extranonce2_size] = 0;
}

bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc) {
  if (doc["error"].size() == 0) {
    return false;
  }
  Serial.printf("ERROR: %d | reason: %s \n", (const int) doc["error"][0], (const char*) doc["error"][1]);
  return true;  
}

void runWorker(void *name) {

// TEST: https://bitcoin.stackexchange.com/questions/22929/full-example-data-for-scrypt-stratum-client

  Serial.println("");
  Serial.printf("\n[WORKER] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());

  #ifdef DEBUG_MEMORY
  Serial.printf("### [Total Heap / Free heap]: %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap());
  #endif

  // connect to pool
  WiFiClient client;
  IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres
  bool isMinerSuscribed = false;
  bool continueSecuence = false;
  String line, extranonce1, extranonce2 = String("0");
  unsigned long id = 0, extranonce_number = 0;
  unsigned int extranonce2_size;

  while(true) {
      
    if(WiFi.status() != WL_CONNECTED){
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    } 

    // get template
    StaticJsonDocument<BUFFER_JSON_DOC> doc;
    
    char payload[BUFFER_JSON_DOC] = {0};
    
    if (!client.connected()) {
      isMinerSuscribed = false;
      Serial.println("Client not connected, trying to connect..."); 
      if (!client.connect(serverIP, portNumber)) {
        Serial.println("Imposible to connect to : " + String(poolString));
        WiFi.hostByName(poolString, serverIP);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      } 
    }

    // STEP 1: Pool server connection (SUBSCRIBE)
    // Docs: 
    // - https://cs.braiins.com/stratum-v1/docs
    // - https://github.com/aeternity/protocol/blob/master/STRATUM.md#mining-subscribe
    if(!isMinerSuscribed){
      id = getNextId(id);
      sprintf(payload, "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"NerdMinerV2\"]}\n", id);
      Serial.printf("[WORKER] %s ==> Mining subscribe\n", (char *)name);
      Serial.print("  Sending  : "); Serial.println(payload);
      client.print(payload);
      vTaskDelay(200 / portTICK_PERIOD_MS);
      line = client.readStringUntil('\n');
      if(!verifyPayload(&line)) continue;
      Serial.print("  Receiving: "); Serial.println(line);
      deserializeJson(doc, line);
      if (checkError(doc)) {
        Serial.printf("[WORKER] %s >>>>>>>>> Work aborted\n", (char *)name); 
        continue;
      }
      String sub_details = String((const char*) doc["result"][0][0][1]);
      extranonce1 = String((const char*) doc["result"][1]);
      int extranonce2_size = doc["result"][2];
      
      // DIFFICULTY
      line = client.readStringUntil('\n');
      Serial.print("  Receiving: "); Serial.println(line);
      Serial.print("    sub_details: "); Serial.println(sub_details);
      Serial.print("    extranonce1: "); Serial.println(extranonce1);
      Serial.print("    extranonce2_size: "); Serial.println(extranonce2_size);

      if((extranonce1.length() == 0) || line.length() == 0) { 
        Serial.printf("[WORKER] %s >>>>>>>>> Work aborted\n", (char *)name); 
        Serial.printf("extranonce1 length: %u | line2 length: %u \n", extranonce1.length(), line.length());
        client.stop();
        doc.clear();
        doc.garbageCollect();
        continue; 
      }
      isMinerSuscribed=true;
    }
  
    // STEP 2: Pool authorize work (Block Info)
    id = getNextId(id);
    sprintf(payload, "{\"params\": [\"%s\", \"x\"], \"id\": %u, \"method\": \"mining.authorize\"}\n", 
      btcString,
      id);
    Serial.printf("[WORKER] %s ==> Autorize work\n", (char *)name);
    Serial.print("  Sending  : "); Serial.println(payload);
    client.print(payload);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    line = client.readStringUntil('\n');
    if(!verifyPayload(&line)) continue;
    Serial.print("  Receiving: "); Serial.println(line);
    Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));
    Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));
    // client.stop();
    
    deserializeJson(doc, line);
    String job_id = String((const char*) doc["params"][0]);
    String prevhash = String((const char*) doc["params"][1]);
    String coinb1 = String((const char*) doc["params"][2]);
    String coinb2 = String((const char*) doc["params"][3]);
    JsonArray merkle_branch = doc["params"][4];
    String version = String((const char*) doc["params"][5]);
    String nbits = String((const char*) doc["params"][6]);
    String ntime = String((const char*) doc["params"][7]);
    bool clean_jobs = doc["params"][8]; //bool

    #ifdef DEBUG_MINING
    Serial.print("    job_id: "); Serial.println(job_id);
    Serial.print("    prevhash: "); Serial.println(prevhash);
    Serial.print("    coinb1: "); Serial.println(coinb1);
    Serial.print("    coinb2: "); Serial.println(coinb2);
    Serial.print("    merkle_branch size: "); Serial.println(merkle_branch.size());
    Serial.print("    version: "); Serial.println(version);
    Serial.print("    nbits: "); Serial.println(nbits);
    Serial.print("    ntime: "); Serial.println(ntime);
    Serial.print("    clean_jobs: "); Serial.println(clean_jobs);
    #endif
    //Check if parameters where correctly received
    if (checkError(doc)) {
      Serial.printf("[WORKER] %s >>>>>>>>> Work aborted\n", (char *)name); 
      continue;
    }

    templates++;

    // calculate target - target = (nbits[2:]+'00'*(int(nbits[:2],16) - 3)).zfill(64)
    
    char target[TARGET_BUFFER_SIZE+1];
    memset(target, '0', TARGET_BUFFER_SIZE);
    int zeros = (int) strtol(nbits.substring(0, 2).c_str(), 0, 16) - 3;
    memcpy(target + zeros - 2, nbits.substring(2).c_str(), nbits.length() - 2);
    target[TARGET_BUFFER_SIZE] = 0;
    Serial.print("    target: "); Serial.println(target);
    // bytearray target
    uint8_t bytearray_target[32];
    size_t size_target = to_byte_array(target, 32, bytearray_target);
    // uint8_t buf;
    // for (size_t j = 0; j < 16; j++) {
    //     buf = bytearray_target[j];
    //     bytearray_target[j] = bytearray_target[size_target - 1 - j];
    //     bytearray_target[size_target - 1 - j] = buf;
    // }
    for (size_t j = 0; j < 8; j++) {
      bytearray_target[j] ^= bytearray_target[size_target - 1 - j];
      bytearray_target[size_target - 1 - j] ^= bytearray_target[j];
      bytearray_target[j] ^= bytearray_target[size_target - 1 - j];
    }

    // get extranonce2 - extranonce2 = hex(random.randint(0,2**32-1))[2:].zfill(2*extranonce2_size)
    char extranonce2_char[2 * extranonce2_size+1];	
	  extranonce2.toCharArray(extranonce2_char, 2 * extranonce2_size + 1);
    getNextExtranonce2(extranonce2_size, extranonce2_char);
    //extranonce2 = String(extranonce2_char);
    extranonce2 = "00000002";
    
    //get coinbase - coinbase_hash_bin = hashlib.sha256(hashlib.sha256(binascii.unhexlify(coinbase)).digest()).digest()
    String coinbase = coinb1 + extranonce1 + extranonce2 + coinb2;
    Serial.print("    coinbase: "); Serial.println(coinbase);
    size_t str_len = coinbase.length()/2;
    uint8_t bytearray[str_len];

    size_t res = to_byte_array(coinbase.c_str(), str_len*2, bytearray);

    #ifdef DEBUG_MINING
    Serial.print("    extranonce2: "); Serial.println(extranonce2);
    Serial.print("    coinbase: "); Serial.println(coinbase);
    Serial.print("    coinbase bytes - size: "); Serial.println(res);
    for (size_t i = 0; i < res; i++)
        Serial.printf("%02x", bytearray[i]);
    Serial.println("---");
    #endif

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
  
    byte interResult[32]; // 256 bit
    byte shaResult[32]; // 256 bit
  
    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, bytearray, str_len);
    mbedtls_sha256_finish_ret(&ctx, interResult);

    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, interResult, 32);
    mbedtls_sha256_finish_ret(&ctx, shaResult);
    mbedtls_sha256_free(&ctx);

    #ifdef DEBUG_MINING
    Serial.print("    coinbase double sha: ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x", shaResult[i]);
    Serial.println("");
    #endif

    byte merkle_result[32];
    // copy coinbase hash
    memcpy(merkle_result, shaResult, sizeof(shaResult));
    
    byte merkle_concatenated[32 * 2];
    for (size_t k=0; k < merkle_branch.size(); k++) {
        const char* merkle_element = (const char*) merkle_branch[k];
        uint8_t bytearray[32];
        size_t res = to_byte_array(merkle_element, 64, bytearray);

        #ifdef DEBUG_MINING
        Serial.print("    merkle element    "); Serial.print(k); Serial.print(": "); Serial.println(merkle_element);
        #endif
        for (size_t i = 0; i < 32; i++) {
          merkle_concatenated[i] = merkle_result[i];
          merkle_concatenated[32 + i] = bytearray[i];
        }

        #ifdef DEBUG_MINING
        Serial.print("    merkle element    "); Serial.print(k); Serial.print(": "); Serial.println(merkle_element);
        Serial.print("    merkle concatenated: ");
        for (size_t i = 0; i < 64; i++)
            Serial.printf("%02x", merkle_concatenated[i]);
        Serial.println("");
        #endif

        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts_ret(&ctx,0);
        mbedtls_sha256_update_ret(&ctx, merkle_concatenated, 64);
        mbedtls_sha256_finish_ret(&ctx, interResult);

        mbedtls_sha256_starts_ret(&ctx,0);
        mbedtls_sha256_update_ret(&ctx, interResult, 32);
        mbedtls_sha256_finish_ret(&ctx, merkle_result);
        mbedtls_sha256_free(&ctx);

        #ifdef DEBUG_MINING
        Serial.print("    merkle sha         : ");
        for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", merkle_result[i]);
        Serial.println("");
        #endif
    }
    // merkle root from merkle_result
    
    Serial.print("    merkle sha         : ");
    char merkle_root[65];
    for (int i = 0; i < 32; i++) {
      Serial.printf("%02x", merkle_result[i]);
      snprintf(&merkle_root[i*2], 3, "%02x", merkle_result[i]);
    }
    merkle_root[65] = 0;
    Serial.println("");
    
    // calculate blockheader
    // j.block_header = ''.join([j.version, j.prevhash, merkle_root, j.ntime, j.nbits])
    String blockheader = version + prevhash + String(merkle_root) + ntime + nbits + "00000000"; 
    str_len = blockheader.length()/2;
    uint8_t bytearray_blockheader[str_len];
    res = to_byte_array(blockheader.c_str(), str_len*2, bytearray_blockheader);

    #ifdef DEBUG_MINING
    Serial.println("    blockheader bytes "); Serial.print(str_len); Serial.print(" -> ");
    #endif

    // reverse version
    uint8_t buff;
    size_t bsize, boffset;
    boffset = 0;
    bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = bytearray_blockheader[j];
        bytearray_blockheader[j] = bytearray_blockheader[2 * boffset + bsize - 1 - j];
        bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }

    // reverse merkle 
    boffset = 36;
    bsize = 32;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = bytearray_blockheader[j];
        bytearray_blockheader[j] = bytearray_blockheader[2 * boffset + bsize - 1 - j];
        bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }
    // reverse difficulty
    boffset = 72;
    bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = bytearray_blockheader[j];
        bytearray_blockheader[j] = bytearray_blockheader[2 * boffset + bsize - 1 - j];
        bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }


    #ifdef DEBUG_MINING
    Serial.print(" >>> bytearray_blockheader     : "); 
    for (size_t i = 0; i < 4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("version     ");
    for (size_t i = 0; i < 4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("prev hash   ");
    for (size_t i = 4; i < 4+32; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("merkle root ");
    for (size_t i = 36; i < 36+32; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("nbits       ");
    for (size_t i = 68; i < 68+4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("difficulty  ");
    for (size_t i = 72; i < 72+4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("nonce       ");
    for (size_t i = 76; i < 76+4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("bytearray_blockheader: ");
    for (size_t i = 0; i < str_len; i++) {
      Serial.printf("%02x", bytearray_blockheader[i]);
    }
    Serial.println("");
    #endif

    mbedtls_sha256_context midstate[32];
    unsigned char hash[32];

    //Calcular midstate
    mbedtls_sha256_init(midstate); 
    mbedtls_sha256_starts_ret(midstate, 0);
    mbedtls_sha256_update_ret(midstate, bytearray_blockheader, 64);

    // search a valid nonce
    enableGlobalHash = true;

    unsigned long nonce = TARGET_NONCE - MAX_NONCE;
    uint32_t startT = micros();
    unsigned char *header64 = bytearray_blockheader + 64;
    Serial.println(">>> STARTING TO HASH NONCES");
    while(true) {
      memcpy(bytearray_blockheader + 76, &nonce, 4);

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

      // check if 16bit share
      if(hash[31] !=0 || hash[30] !=0) continue;
      halfshares++;
      // check if 32bit share
      if(hash[29] !=0 || hash[28] !=0) continue;
      shares++;

       // check if valid header
      if(checkValid(hash, bytearray_target)){
        Serial.printf("[WORKER] %s CONGRATULATIONS! Valid completed with nonce: %d | 0x%x\n", (char *)name, nonce, nonce);
        valids++;
        Serial.printf("[WORKER]  %s  Submiting work valid!\n", (char *)name);
        while (!client.connected()) {
          client.connect(poolString, portNumber);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        // STEP 3: Submit mining job
        id = getNextId(id);
        sprintf(payload, "{\"params\": [\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"], \"id\": %u, \"method\": \"mining.submit\"}",
          btcString,
          job_id,
          extranonce2,
          ntime,
          String(nonce, HEX),
          id
          );
        Serial.print("  Sending  : "); Serial.println(payload);
        client.print(payload);
        Serial.print("  Receiving: "); Serial.println(client.readString());
        client.stop();
        // exit 
        nonce = MAX_NONCE;
        break;
      }    
    } // exit if found a valid result or nonce > MAX_NONCE

    mbedtls_sha256_free(&ctx);
    mbedtls_sha256_free(midstate);
    enableGlobalHash = false;

    // TODO Pending doub 
    if(hashes>=MAX_NONCE) { Mhashes=Mhashes+MAX_NONCE/1000000; hashes=hashes-MAX_NONCE;}

    uint32_t duration = micros() - startT;
  }
  
}


//////////////////THREAD CALLS///////////////////

//Testeamos hashrate final usando hilo principal
//this is currently on test

void runMiner(void){
  uint32_t nonce=0;
  unsigned char bytearray_blockheader[80];

  if(!enableGlobalHash) return;

  mbedtls_sha256_context midstate[32], ctx;
  unsigned char hash[32];

  //Calcular midstate
  mbedtls_sha256_init(midstate); 
  mbedtls_sha256_starts_ret(midstate, 0);
  mbedtls_sha256_update_ret(midstate, bytearray_blockheader, 64);

  //Iteraciones
  unsigned char *header64 = bytearray_blockheader + 64;
  
  for(nonce = 0; nonce < 10000; nonce++){
    memcpy(bytearray_blockheader + 77, &nonce, 3);
    mbedtls_sha256_clone(&ctx, midstate); //Clonamos el contexto anterior para continuar el SHA desde allí
    mbedtls_sha256_update_ret(&ctx, header64, 16);
    mbedtls_sha256_finish_ret(&ctx, hash);

    // Segundo SHA-256
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, hash, 32);
    mbedtls_sha256_finish_ret(&ctx, hash);

    hashes++;
  }

  mbedtls_sha256_free(&ctx);
  mbedtls_sha256_free(midstate);

}

void runMonitor(void *name){

  Serial.println("[MONITOR] started");
  
  unsigned long mLastCheck = 0;

  while(1){
    background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen); 
    
    unsigned long mElapsed = millis()-mLastCheck;
    mLastCheck = millis();
    unsigned long currentKHashes = (Mhashes*1000) + hashes/1000;
    unsigned long elapsedKHs = currentKHashes - totalKHashes; 
    totalKHashes = currentKHashes;
    //Serial.println("[runMonitor Task] -> Printing results on screen ");
    
     Serial.printf(">>> Completed %d share(s), %d Khashes, avg. hashrate %.3f KH/s\n",
      shares, totalKHashes, (1.0*(elapsedKHs*1000))/mElapsed);

    //Hashrate
    render.setFontSize(70);
    render.setCursor(19, 118);
    render.setFontColor(TFT_BLACK);
    char tmp[10] = {0};
    sprintf(tmp, "%.2f", (1.0*(elapsedKHs*1000))/mElapsed);
    render.rdrawString(tmp, 118, 114, TFT_BLACK);
    //Total hashes
    render.setFontSize(36);
    render.rdrawString(String(Mhashes).c_str(), 268, 138, TFT_BLACK);
    //Block templates
    render.setFontSize(36);
    render.drawString(String(templates).c_str(), 186, 20, 0xDEDB);
    //16Bit shares
    render.setFontSize(36);
    render.drawString(String(halfshares).c_str(), 186, 48, 0xDEDB);
    //32Bit shares
    render.setFontSize(36);
    render.drawString(String(shares).c_str(), 186, 76, 0xDEDB);
    //Hores
    unsigned long secElapsed=millis()/1000;
    int hr = secElapsed/3600;                                                        //Number of seconds in an hour
    int mins = (secElapsed-(hr*3600))/60;                                              //Remove the number of hours and calculate the minutes.
    int sec = secElapsed-(hr*3600)-(mins*60);   
    render.setFontSize(36);
    render.rdrawString(String(hr).c_str(), 208, 99, 0xDEDB);
    //Minutss
    render.setFontSize(36);
    render.rdrawString(String(mins).c_str(), 253, 99, 0xDEDB);
    //Segons
    render.setFontSize(36);
    render.rdrawString(String(sec).c_str(), 298, 99, 0xDEDB);
    //Valid Blocks
    render.setFontSize(48);
    render.drawString(String(valids).c_str(), 285, 56, 0xDEDB);
    //Print Temp
    background.setTextColor(TFT_BLACK);
    //background.setFreeFont(FF0);
    background.drawString("30", 230, 4);
    //Print Hour
    background.drawString("22:10", 250, 4);

    //Push prepared background to screen
    background.pushSprite(0,0);
    
    // Pause the task for 5000ms
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
