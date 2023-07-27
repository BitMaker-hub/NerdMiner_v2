#ifndef STRATUM_API_H
#define STRATUM_API_H

#include "cJSON.h"
#include <stdint.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define MAX_MERKLE_BRANCHES 32
#define HASH_SIZE 32
#define COINBASE_SIZE 100
#define COINBASE2_SIZE 128

#define BUFFER_JSON_DOC 4096
#define BUFFER 1024

typedef struct {
    String sub_details;
    String extranonce1;
    String extranonce2;
    int extranonce2_size;
    char wName[80];
    char wPass[20];
} mining_subscribe;

typedef struct {
    String job_id;
    String prev_block_hash;
    String coinb1;
    String coinb2;
    String nbits;
    JsonArray merkle_branch;
    String version;
    uint32_t target;
    String ntime;
    bool clean_jobs;
} mining_job;

typedef enum {
    STRATUM_SUCCESS,
    STRATUM_UNKNOWN,
    STRATUM_PARSE_ERROR,
    MINING_NOTIFY,
    MINING_SET_DIFFICULTY
} stratum_method;

unsigned long getNextId(unsigned long id);
bool verifyPayload (String* line);
bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> doc);

//Method Mining.subscribe
mining_subscribe init_mining_subscribe(void);
bool tx_mining_subscribe(WiFiClient& client, mining_subscribe& mSubscribe);
bool parse_mining_subscribe(String line, mining_subscribe& mSubscribe);

//Method Mining.authorise
bool tx_mining_auth(WiFiClient& client, const char * user, const char * pass);
stratum_method parse_mining_method(String line);
bool parse_mining_notify(String line, mining_job& mJob);

//Method Mining.submit
bool tx_mining_submit(WiFiClient& client, mining_subscribe mWorker, mining_job mJob, unsigned long nonce);

//Difficulty Methods 
bool tx_suggest_difficulty(WiFiClient& client, double difficulty);
bool parse_mining_set_difficulty(String line, double& difficulty);


#endif // STRATUM_API_H