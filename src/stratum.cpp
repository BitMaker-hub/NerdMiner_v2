#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "stratum.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "lwip/sockets.h"
#include "utils.h"
#include "version.h"

#ifndef STRATUM_SUBSCRIBE_TIMEOUT_MS
#define STRATUM_SUBSCRIBE_TIMEOUT_MS 8000
#endif
#ifndef STRATUM_SUBSCRIBE_POLL_MS
#define STRATUM_SUBSCRIBE_POLL_MS 20
#endif
#ifndef STRATUM_VERBOSE_LOG
#define STRATUM_VERBOSE_LOG 0
#endif


StaticJsonDocument<BUFFER_JSON_DOC> doc;
unsigned long id = 1;

//Get next JSON RPC Id
unsigned long getNextId(unsigned long id) {
    if (id == ULONG_MAX) {
      id = 1;
      return id;
    }
    return ++id;
}

//Verify Payload doesn't has zero lenght
bool verifyPayload(String* line){
  if (!line) return false;
  line->trim();
  return line->length() > 0;
}

bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> &doc) {
  
  if (!doc.containsKey("error")) return false;
  
  if (doc["error"].size() == 0 || doc["error"].isNull()) return false;

  const int code = (const int)doc["error"][0];
  const char *reason = (const char*)doc["error"][1];
  bool stale_like = (code == 21);
  if (reason != nullptr)
  {
    stale_like = stale_like || strstr(reason, "stale") || strstr(reason, "Stale") ||
                 strstr(reason, "job not found") || strstr(reason, "Job not found");
  }
  if (!stale_like || STRATUM_VERBOSE_LOG)
    Serial.printf("ERROR: %d | reason: %s \n", code, reason ? reason : "");

  return true;  
}


// STEP 1: Pool server connection (SUBSCRIBE)
    // Docs: 
    // - https://cs.braiins.com/stratum-v1/docs
    // - https://github.com/aeternity/protocol/blob/master/STRATUM.md#mining-subscribe
bool tx_mining_subscribe(WiFiClient& client, mining_subscribe& mSubscribe, const char *resume_id)
{
    char payload[BUFFER] = {0};
    
    // Subscribe
    id = 1; //Initialize id messages
    const bool use_resume = (resume_id != nullptr && resume_id[0] != '\0');
    #ifndef HAN
    if (use_resume)
      // snprintf(payload, sizeof(payload), "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"NerdMinerV2/%s\", \"%s\"]}\n", id, CURRENT_VERSION, resume_id);
      snprintf(payload, sizeof(payload), "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"MYMinerV2/%s\", \"%s\"]}\n", id, CURRENT_VERSION, resume_id);
    else
      // snprintf(payload, sizeof(payload), "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"NerdMinerV2/%s\"]}\n", id, CURRENT_VERSION);
      snprintf(payload, sizeof(payload), "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"MYMinerV2/%s\"]}\n", id, CURRENT_VERSION);
    #else
    if (use_resume)
      snprintf(payload, sizeof(payload), "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"HAN_SOLOminer/%s\", \"%s\"]}\n", id, CURRENT_VERSION, resume_id);
    else
      snprintf(payload, sizeof(payload), "{\"id\": %u, \"method\": \"mining.subscribe\", \"params\": [\"HAN_SOLOminer/%s\"]}\n", id, CURRENT_VERSION);
    #endif
    
    Serial.printf("[WORKER] ==> Mining subscribe%s\n", use_resume ? " (resume)" : "");
    Serial.print("  Sending  : "); Serial.println(payload);
    client.print(payload);
    
    uint32_t started = millis();
    while ((uint32_t)(millis() - started) < STRATUM_SUBSCRIBE_TIMEOUT_MS)
    {
      if (!client.connected())
        return false;

      if (!client.available())
      {
        vTaskDelay(STRATUM_SUBSCRIBE_POLL_MS / portTICK_PERIOD_MS);
        continue;
      }

      String line = client.readStringUntil('\n');
      if (line.length() == 0)
        continue;
      if (parse_mining_subscribe(line, mSubscribe))
        break;
    }

    if (mSubscribe.extranonce1.length() == 0)
      return false;

  
    Serial.print("    sub_details: "); Serial.println(mSubscribe.sub_details);
    Serial.print("    extranonce1: "); Serial.println(mSubscribe.extranonce1);
    Serial.print("    extranonce2_size: "); Serial.println(mSubscribe.extranonce2_size);

    if((mSubscribe.extranonce1.length() == 0) ) { 
        Serial.printf("[WORKER] >>>>>>>>> Work aborted\n"); 
        Serial.printf("extranonce1 length: %u \n", mSubscribe.extranonce1.length());
        doc.clear();
        doc.garbageCollect();
        return false; 
    }
    return true;
}

bool parse_mining_subscribe(String line, mining_subscribe& mSubscribe)
{
    if(!verifyPayload(&line)) return false;
    #if STRATUM_VERBOSE_LOG
    Serial.print("  Receiving: "); Serial.println(line);
    #endif
   
    DeserializationError error = deserializeJson(doc, line);

    if (error || checkError(doc)) return false;
    if (!doc.containsKey("result")) return false;

    mSubscribe.sub_details = String((const char*) doc["result"][0][0][1]);
    mSubscribe.extranonce1 = String((const char*) doc["result"][1]);
    mSubscribe.extranonce2_size = doc["result"][2];

    return true;
}

mining_subscribe init_mining_subscribe(void)
{
    mining_subscribe new_mSub;

    new_mSub.extranonce1 = "";
    new_mSub.extranonce2 = "";
    new_mSub.extranonce2_size = 0;
    new_mSub.sub_details = "";


    return new_mSub;
}

// STEP 2: Pool server auth (authorize)
bool tx_mining_auth(WiFiClient& client, const char * user, const char * pass)
{
    char payload[BUFFER] = {0};

    // Authorize
    id = getNextId(id);
    sprintf(payload, "{\"params\": [\"%s\", \"%s\"], \"id\": %u, \"method\": \"mining.authorize\"}\n", 
      user, pass, id);
    
    Serial.printf("[WORKER] ==> Autorize work\n");
    Serial.print("  Sending  : "); Serial.println(payload);
    client.print(payload);

    vTaskDelay(200 / portTICK_PERIOD_MS); //Small delay

    //Don't parse here any answer
    //Miner started to receive mining notifications so better parse all at main thread

    return true;
}


stratum_method parse_mining_method(String line)
{
    if(!verifyPayload(&line)) return STRATUM_PARSE_ERROR;
    #if STRATUM_VERBOSE_LOG
    Serial.print("  Receiving: "); Serial.println(line);
    #endif
    
    DeserializationError error = deserializeJson(doc, line);
    if (error)
      return STRATUM_PARSE_ERROR;

    return parse_mining_method_doc(doc);
}

stratum_method parse_mining_method_doc(const StaticJsonDocument<BUFFER_JSON_DOC>& doc)
{
    if (checkError(doc))
      return STRATUM_PARSE_ERROR;

    if (!doc.containsKey("method"))
    {
      // "error":null means success
      if (doc["error"].isNull())
        return STRATUM_SUCCESS;
      return STRATUM_UNKNOWN;
    }
    const char *method = doc["method"];
    if (method == nullptr)
      return STRATUM_UNKNOWN;

    stratum_method result = STRATUM_UNKNOWN;

    if (strcmp("mining.notify", method) == 0) {
        result = MINING_NOTIFY;
    } else if (strcmp("mining.set_difficulty", method) == 0) {
        result = MINING_SET_DIFFICULTY;
    }

    return result;
}

bool parse_mining_notify(String line, mining_job& mJob)
{
    if(!verifyPayload(&line)) return false;
    #if STRATUM_VERBOSE_LOG
    Serial.println("    Parsing Method [MINING NOTIFY]");
    #endif
   
    DeserializationError error = deserializeJson(doc, line);
    if (error)
      return false;

    return parse_mining_notify_doc(doc, mJob);
}

bool parse_mining_notify_doc(StaticJsonDocument<BUFFER_JSON_DOC>& doc, mining_job& mJob)
{
    if (!doc.containsKey("params"))
      return false;

    mJob.job_id = String((const char*)doc["params"][0]);
    mJob.prev_block_hash = String((const char*)doc["params"][1]);
    mJob.coinb1 = String((const char*)doc["params"][2]);
    mJob.coinb2 = String((const char*)doc["params"][3]);
    mJob.merkle_branch = doc["params"][4];
    mJob.version = String((const char*)doc["params"][5]);
    mJob.nbits = String((const char*)doc["params"][6]);
    mJob.ntime = String((const char*)doc["params"][7]);
    mJob.clean_jobs = doc["params"][8]; //bool

    #ifdef DEBUG_MINING
    Serial.print("    job_id: "); Serial.println(mJob.job_id);
    Serial.print("    prevhash: "); Serial.println(mJob.prev_block_hash);
    Serial.print("    coinb1: "); Serial.println(mJob.coinb1);
    Serial.print("    coinb2: "); Serial.println(mJob.coinb2);
    Serial.print("    merkle_branch size: "); Serial.println(mJob.merkle_branch.size());
    Serial.print("    version: "); Serial.println(mJob.version);
    Serial.print("    nbits: "); Serial.println(mJob.nbits);
    Serial.print("    ntime: "); Serial.println(mJob.ntime);
    Serial.print("    clean_jobs: "); Serial.println(mJob.clean_jobs);
    #endif
    //Check if parameters where correctly received
    if (checkError(doc)) {
      Serial.printf("[WORKER] >>>>>>>>> Work aborted\n"); 
      return false;
    }
    return true;
}


bool tx_mining_submit(WiFiClient& client, mining_subscribe mWorker, mining_job mJob, unsigned long nonce, unsigned long &submit_id)
{
    char payload[BUFFER] = {0};

    // Submit
    id = getNextId(id);
    submit_id = id;
    sprintf(payload, "{\"id\":%u,\"method\":\"mining.submit\",\"params\":[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]}\n",
        id,
        mWorker.wName,//"bc1qvv469gmw4zz6qa4u4dsezvrlmqcqszwyfzhgwj", //mWorker.name,
        mJob.job_id.c_str(),
        mWorker.extranonce2.c_str(),
        mJob.ntime.c_str(),
        String(nonce, HEX).c_str()
        );
    #if STRATUM_VERBOSE_LOG
    Serial.print("  Sending  : "); Serial.print(payload);
    #endif
    client.print(payload);
    //Serial.print("  Receiving: "); Serial.println(client.readStringUntil('\n'));

    return true;
}

bool parse_mining_set_difficulty(String line, double& difficulty)
{
    if(!verifyPayload(&line)) return false;
    #if STRATUM_VERBOSE_LOG
    Serial.println("    Parsing Method [SET DIFFICULTY]");
    #endif
   
    DeserializationError error = deserializeJson(doc, line);
    if (error)
      return false;

    return parse_mining_set_difficulty_doc(doc, difficulty);
}

bool parse_mining_set_difficulty_doc(const StaticJsonDocument<BUFFER_JSON_DOC>& doc, double& difficulty)
{
    if (!doc.containsKey("params"))
      return false;

    #if STRATUM_VERBOSE_LOG
    Serial.print("    difficulty: "); Serial.println((double)doc["params"][0],12);
    #endif
    difficulty = (double)doc["params"][0];

    return true;
}

bool tx_suggest_difficulty(WiFiClient& client, double difficulty)
{
    char payload[BUFFER] = {0};

    id = getNextId(id);
    sprintf(payload, "{\"id\":%d,\"method\":\"mining.suggest_difficulty\",\"params\":[%.10g]}\n", id, difficulty);
    
    Serial.print("  Sending  : "); Serial.print(payload);
    return client.print(payload);

}


unsigned long parse_extract_id(String line)
{
    DeserializationError error = deserializeJson(doc, line);
    if (error)
        return 0;

    return parse_extract_id_doc(doc);
}

unsigned long parse_extract_id_doc(const StaticJsonDocument<BUFFER_JSON_DOC>& doc)
{
    if (!doc.containsKey("id"))
      return 0;

    return doc["id"];
}
