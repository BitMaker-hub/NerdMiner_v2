#pragma once
#include <stdint.h>
#include "nerdnos.h"

// forward declaration to resolve circular inclusions
typedef struct mining_subscribe mining_subscribe;
typedef struct mining_job mining_job;

// set the asic hardware difficulty
void nerdnos_set_asic_difficulty(uint32_t current_difficulty);

// create asic job
void nerdnos_create_job(mining_subscribe *mWorker, mining_job *job, bm_job_t *next_job, uint32_t extranonce_2, uint32_t stratum_difficulty);

// send new work to the asic
void nerdnos_send_work(bm_job_t *next_bm_job, uint8_t job_id);

// receive and process responses
task_result *nerdnos_proccess_work(uint32_t version, uint16_t timeout);

// test difficulty
double nerdnos_test_nonce_value(const bm_job_t *job, const uint32_t nonce, const uint32_t rolled_version, uint8_t hash_result[32]);

// free allocated RAM for strings on bm_job_t
void nerdnos_free_bm_job(bm_job_t *job);
