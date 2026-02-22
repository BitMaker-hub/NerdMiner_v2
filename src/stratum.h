#ifndef STRATUM_API_H
#define STRATUM_API_H

#include "cJSON.h"
#include <stdint.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define MAX_MERKLE_BRANCHES 32
#define HASH_SIZE 32
#define COINBASE_SIZE 384
#define COINBASE2_SIZE 384
#define STRATUM_SUB_DETAILS_SIZE 96
#define STRATUM_EXTRANONCE1_SIZE 64
#define STRATUM_EXTRANONCE2_SIZE 64
#define STRATUM_JOB_ID_SIZE 96
#define STRATUM_VERSION_SIZE 16
#define STRATUM_NBITS_SIZE 16
#define STRATUM_NTIME_SIZE 16

#define BUFFER_JSON_DOC 4096
#define BUFFER 1024

typedef struct {
    char sub_details[STRATUM_SUB_DETAILS_SIZE];
    char extranonce1[STRATUM_EXTRANONCE1_SIZE];
    char extranonce2[STRATUM_EXTRANONCE2_SIZE];
    int extranonce2_size;
    char wName[80];
    char wPass[80];
} mining_subscribe;

typedef struct {
    char job_id[STRATUM_JOB_ID_SIZE];
    char prev_block_hash[(HASH_SIZE * 2) + 1];
    char coinb1[COINBASE_SIZE];
    char coinb2[COINBASE2_SIZE];
    char nbits[STRATUM_NBITS_SIZE];
    JsonArray merkle_branch;
    char version[STRATUM_VERSION_SIZE];
    uint32_t target;
    char ntime[STRATUM_NTIME_SIZE];
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
bool verifyPayload(String* line);
bool checkError(const StaticJsonDocument<BUFFER_JSON_DOC> &doc);

//Method Mining.subscribe
mining_subscribe init_mining_subscribe(void);
bool tx_mining_subscribe(WiFiClient& client, mining_subscribe& mSubscribe, const char *resume_id = nullptr);
bool parse_mining_subscribe(String line, mining_subscribe& mSubscribe);

//Method Mining.authorise
bool tx_mining_auth(WiFiClient& client, const char * user, const char * pass);
stratum_method parse_mining_method(String line);
stratum_method parse_mining_method_doc(const StaticJsonDocument<BUFFER_JSON_DOC>& doc);
bool parse_mining_notify(String line, mining_job& mJob);
bool parse_mining_notify_doc(StaticJsonDocument<BUFFER_JSON_DOC>& doc, mining_job& mJob);

//Method Mining.submit
bool tx_mining_submit(WiFiClient& client, const mining_subscribe& mWorker, const mining_job& mJob, unsigned long nonce, unsigned long &submit_id);

//Difficulty Methods 
bool tx_suggest_difficulty(WiFiClient& client, double difficulty);
bool parse_mining_set_difficulty(String line, double& difficulty);
bool parse_mining_set_difficulty_doc(const StaticJsonDocument<BUFFER_JSON_DOC>& doc, double& difficulty);

unsigned long parse_extract_id(String line);
unsigned long parse_extract_id_doc(const StaticJsonDocument<BUFFER_JSON_DOC>& doc);

#endif // STRATUM_API_H
