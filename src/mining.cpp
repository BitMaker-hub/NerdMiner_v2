#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <algorithm>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include "media/Free_Fonts.h"
#include "media/images.h"
#include "mbedtls/md.h"
#include "OpenFontRender.h"
#include "mining.h"

#define TARGET_BUFFER_SIZE 64

unsigned long templates = 0;
unsigned long hashes= 0;
unsigned long Mhashes = 0;

int halfshares; // increase if blockhash has 16 bits of zeroes
int shares; // increase if blockhash has 32 bits of zeroes
int valids; // increased if blockhash <= target

// Variables to hold data from custom textboxes
extern char poolString[80];
extern int portNumber;
extern char btcString[80];

extern OpenFontRender render;
extern TFT_eSprite background;

bool checkHalfShare(unsigned char* hash) {
  bool valid = true;
  for(uint8_t i=31; i>31-2; i--) {
    if(hash[i] != 0) {
      valid = false;
      break;
    }
  }
  #ifdef DEBUG_MINING
  if (valid) {
    Serial.print("\thalf share : ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x ", hash[i]);
    Serial.println();
  }
  #endif
  return valid;
}

bool checkShare(unsigned char* hash) {
  bool valid = true;
  for(uint8_t i=31; i>31-4; i--) {
    if(hash[i] != 0) {
      valid = false;
      break;
    }
  }
  #ifdef DEBUG_MINING
  if (valid) {
    Serial.print("\tshare : ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x ", hash[i]);
    Serial.println();
  }
  #endif
  return valid;
}

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
void runWorker(void *name) {

  // TEST: https://bitcoin.stackexchange.com/questions/22929/full-example-data-for-scrypt-stratum-client

  Serial.println("");
  Serial.printf("\n[WORKER] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());
  ;Serial.printf("### [Total Heap / Free heap]: %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap());
  
  String ADDRESS = String(btcString);

  mbedtls_md_context_t ctx;
  // connect to pool
  WiFiClient client;
  bool continueSecuence = false;
  String line, extranonce1;
  unsigned long id = 1;
  unsigned int extranonce2_size;

  while(true) {
      
    if(WiFi.status() != WL_CONNECTED) continue;

    // get template
    DynamicJsonDocument doc(4 * 1024);
    String payload;
    
    if (!client.connect(poolString, portNumber)) {
      continue;
    }
    // STEP 1: Pool server connection
    payload = "{\"id\": "+ String(id++) +", \"method\": \"mining.subscribe\", \"params\": [\"" + ADDRESS + "\", \"password\"]}\n";
    Serial.printf("[WORKER] %s ==> Mining subscribe\n", (char *)name);
    Serial.print("  Sending  : "); Serial.println(payload);
    client.print(payload.c_str());
    line = client.readStringUntil('\n');
    Serial.print("  Receiving: "); Serial.println(line);
    deserializeJson(doc, line);
    int error = doc["error"];
    String sub_details = String((const char*) doc["result"][0][0][1]);
    extranonce1 = String((const char*) doc["result"][1]);
    int extranonce2_size = doc["result"][2];
    
    // DIFFICULTY
    line = client.readStringUntil('\n');
    Serial.print("    sub_details: "); Serial.println(sub_details);
    Serial.print("    extranonce1: "); Serial.println(extranonce1);
    Serial.print("    extranonce2_size: "); Serial.println(extranonce2_size);
    Serial.print("    error: "); Serial.println(error);
    if((extranonce1.length() == 0) || line.length() == 0 || (error != 0)) { 
      Serial.printf("[WORKER] %s >>>>>>>>> Work aborted\n", (char *)name); 
      Serial.printf("extranonce1 length: %u | line2 length: %u | error code: %u \n", extranonce1.length(), line.length(), error);
      client.stop();
      doc.clear();
      doc.garbageCollect();
      continue; 
    }
  
    // STEP 2: Pool authorize work
    payload = "{\"params\": [\"" + ADDRESS + "\", \"password\"], \"id\": "+ String(id++) +", \"method\": \"mining.authorize\"}\n";
    Serial.printf("[WORKER] %s ==> Autorize work\n", (char *)name);
    Serial.print("  Sending  : "); Serial.println(payload);
    client.print(payload.c_str());
    line = client.readStringUntil('\n');
    Serial.print("  Receiving: "); Serial.println(line);
    Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));
    Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));
    client.stop();

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
    if((job_id.length() == 0)||(prevhash.length() == 0)||(coinb2.length() == 0)||(ntime.length() == 0)) { 
      Serial.println(">>>>>>>>> Worker aborted"); 
      client.stop();
      doc.clear();
      doc.garbageCollect();
      continue; 
     }

    doc.clear();
    templates++;

    // calculate target - target = (nbits[2:]+'00'*(int(nbits[:2],16) - 3)).zfill(64)
    
    char target[TARGET_BUFFER_SIZE+1];
    memset(target, '0', TARGET_BUFFER_SIZE);
    int zeros = (int) strtol(nbits.substring(0, 2).c_str(), 0, 16) - 3;
    memcpy(target + zeros - 2, nbits.substring(2).c_str(), nbits.length() - 2);
    target[TARGET_BUFFER_SIZE+1] = 0;
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
    uint32_t extranonce2_a_bin = esp_random();
    uint32_t extranonce2_b_bin = esp_random();
    String extranonce2_a = String(extranonce2_a_bin, HEX);
    String extranonce2_b = String(extranonce2_b_bin, HEX);
    uint8_t pad = 8 - extranonce2_a.length();
    char extranonce2_a_char[pad+1];
    for (int k = 0; k < pad; k++) {
      extranonce2_a_char[k] = '0';
    }
    extranonce2_a_char[pad+1] = 0;
    extranonce2_a = String(extranonce2_a_char) + extranonce2_a;

    pad = 8 - extranonce2_b.length();
    char extranonce2_b_char[pad+1];
    for (int k = 0; k < pad; k++) {
      extranonce2_b_char[k] = '0';
    }

    extranonce2_b_char[pad+1] = 0;
    extranonce2_b = String(extranonce2_b_char) + extranonce2_b;

    String extranonce2 = String(extranonce2_a + extranonce2_b).substring(0, 17 - (2 * extranonce2_size));
    //get coinbase - coinbase_hash_bin = hashlib.sha256(hashlib.sha256(binascii.unhexlify(coinbase)).digest()).digest()
    String coinbase = coinb1 + extranonce1 + extranonce2 + coinb2;
    size_t str_len = coinbase.length()/2;
    uint8_t bytearray[str_len];

    size_t res = to_byte_array(coinbase.c_str(), str_len*2, bytearray);

    #ifdef DEBUG_MINING
    Serial.print("    extranonce2: "); Serial.println(extranonce2);
    Serial.print("    coinbase: "); Serial.println(coinbase);
    Serial.print("    coinbase bytes - size: "); Serial.println(res);
    for (size_t i = 0; i < res; i++)
        Serial.printf("%02x ", bytearray[i]);
    Serial.println("---");
    #endif

    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);

    byte interResult[32]; // 256 bit
    byte shaResult[32]; // 256 bit
  
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, bytearray, str_len);
    mbedtls_md_finish(&ctx, interResult);

    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, interResult, 32);
    mbedtls_md_finish(&ctx, shaResult);

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
    for (size_t k=0; k<merkle_branch.size(); k++) {
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

        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, merkle_concatenated, 64);
        mbedtls_md_finish(&ctx, interResult);

        mbedtls_md_starts(&ctx);
        mbedtls_md_update(&ctx, interResult, 32);
        mbedtls_md_finish(&ctx, merkle_result);

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
    String blockheader = version + prevhash + String(merkle_root) + nbits + ntime + "00000000"; 
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
    Serial.println("version");
    for (size_t i = 0; i < 4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("prev hash");
    for (size_t i = 4; i < 4+32; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("merkle root");
    for (size_t i = 36; i < 36+32; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("time");
    for (size_t i = 68; i < 68+4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("difficulty");
    for (size_t i = 72; i < 72+4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("nonce");
    for (size_t i = 76; i < 76+4; i++)
        Serial.printf("%02x", bytearray_blockheader[i]);
    Serial.println("");
    #endif

    // search a valid nonce
    uint32_t nonce = 0;
    uint32_t startT = micros();
    Serial.println(">>> STARTING TO HASH NONCES");
    while(true) {
      bytearray_blockheader[76] = (nonce >> 0) & 0xFF;
      bytearray_blockheader[77] = (nonce >> 8) & 0xFF;
      bytearray_blockheader[78] = (nonce >> 16) & 0xFF;
      bytearray_blockheader[79] = (nonce >> 24) & 0xFF;

      // double sha
      mbedtls_md_starts(&ctx);
      mbedtls_md_update(&ctx, bytearray_blockheader, 80);
      mbedtls_md_finish(&ctx, interResult);

      mbedtls_md_starts(&ctx);
      mbedtls_md_update(&ctx, interResult, 32);
      mbedtls_md_finish(&ctx, shaResult);

      // check if half share
      if(checkHalfShare(shaResult)) {
        halfshares++;
        // check if share
        if(checkShare(shaResult)) {
          shares++;
          // check if valid header
          if(checkValid(shaResult, bytearray_target)) {
            //Serial.printf("%s on core %d: ", (char *)name, xPortGetCoreID());
            Serial.printf("[WORKER] %s CONGRATULATIONS! Valid completed with nonce: %d | 0x%x\n", (char *)name, nonce, nonce);
            valids++;
            Serial.printf("[WORKER]  %s  Submiting work valid!\n", (char *)name);
            while (!client.connected()) {
              client.connect(poolString, portNumber);
              vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            // STEP 3: Submit mining job
            payload = "{\"params\": [\"" + ADDRESS + "\", \"" + job_id + "\", \"" + extranonce2 + "\", \"" + ntime + "\", \"" + String(nonce, HEX) + "\"], \"id\": "+ String(id++) +", \"method\": \"mining.submit\"}";
            Serial.print("  Sending  : "); Serial.println(payload);
            client.print(payload.c_str());
            line = client.readStringUntil('\n');
            Serial.print("  Receiving: "); Serial.println(line);
            client.stop();
            // exit 
            nonce = MAX_NONCE;
            break;
          }
        } 
      }     

      nonce++;
      hashes++;
      
      if(hashes++>1000000) { Mhashes++; hashes=0;}
      // exit 
      if (nonce >= MAX_NONCE) {
        break;
      }
    } // exit if found a valid result or nonce > MAX_NONCE

    if (nonce == MAX_NONCE) {
        Serial.printf("[WORKER] %s SUBMITING WORK... MAX Nonce reached > MAX_NONCE\n", (char *)name);
        // STEP 3: Submit mining job
        if (client.connect(poolString, portNumber)) {
          payload = "{\"params\": [\"" + ADDRESS + "\", \"" + job_id + "\", \"" + extranonce2 + "\", \"" + ntime + "\", \"" + String(nonce, HEX) + "\"], \"id\": "+ String(id++) +", \"method\": \"mining.submit\"}";
          Serial.print("  Sending  : "); Serial.println(payload);
          client.print(payload.c_str());
          Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));
          while (client.available()) {
            Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));
          }
          client.stop();
        }
    }
    uint32_t duration = micros() - startT;
  }
  mbedtls_md_free(&ctx);
}


//////////////////THREAD CALLS///////////////////

void runMonitor(void *name){

  Serial.println("[MONITOR] started");
  
  unsigned long mStart = millis();
  while(1){
    background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen); 
    
    unsigned long mElapsed = millis()-mStart;
    unsigned long totalKHashes = (Mhashes*1000) + hashes/1000; 
    //Serial.println("[runMonitor Task] -> Printing results on screen ");
    Serial.printf(">>> Completed %d share(s), %d Khashes, avg. hashrate %.3f KH/s\n",
      shares, totalKHashes, (1.0*(totalKHashes*1000))/mElapsed);

    //Hashrate
    render.setFontSize(70);
    render.setCursor(19, 118);
    render.setFontColor(TFT_BLACK);
    char tmp[10] = {0};
    sprintf(tmp, "%.2f", (1.0*(totalKHashes*1000))/mElapsed);
    render.rdrawString(tmp, 118, 114, TFT_BLACK);
    //Total hashes
    render.setFontSize(36);
    render.rdrawString(String(Mhashes).c_str(), 268, 138, TFT_BLACK);
    //Block templates
    render.setFontSize(36);
    render.drawString(String(templates).c_str(), 186, 17, 0xDEDB);
    //16Bit shares
    render.setFontSize(36);
    render.drawString(String(halfshares).c_str(), 186, 45, 0xDEDB);
    //32Bit shares
    render.setFontSize(36);
    render.drawString(String(shares).c_str(), 186, 73, 0xDEDB);
    //Hores
    unsigned long secElapsed=mElapsed/1000;
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
    render.drawString(String(valids).c_str(), 281, 55, 0xDEDB);

    //Push prepared background to screen
    background.pushSprite(0,0);
    
    // Pause the task for 5000ms
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

