#pragma once

#include <stdint.h>

#define BM1397_INITIAL_DIFFICULTY 128

typedef struct
{
    uint8_t job_id;
    uint32_t nonce;
    uint32_t rolled_version;
    uint32_t data;
    uint8_t reg;
    uint8_t is_reg_resp;
} task_result;

typedef struct
{
    uint32_t version;
    uint32_t version_mask;
    uint8_t prev_block_hash[32];
    uint8_t prev_block_hash_be[32];
    uint8_t merkle_root[32];
    uint8_t merkle_root_be[32];
    uint32_t ntime;
    uint32_t target; // aka difficulty, aka nbits
    uint32_t starting_nonce;

    uint8_t num_midstates;
    uint8_t midstate[32];
    uint8_t midstate1[32];
    uint8_t midstate2[32];
    uint8_t midstate3[32];
    char *jobid;
    char *extranonce2;
    uint32_t pool_diff;
} bm_job_t;

/*
typedef struct
{
    float frequency;
} bm1397Module;
*/
typedef enum
{
    JOB_PACKET = 0,
    CMD_PACKET = 1,
} packet_type_t;

typedef enum
{
    JOB_RESP = 0,
    CMD_RESP = 1,
} response_type_t;

typedef struct __attribute__((__packed__))
{
    uint8_t job_id;
    uint8_t num_midstates;
    uint8_t starting_nonce[4];
    uint8_t nbits[4];
    uint8_t ntime[4];
    uint8_t merkle4[4];
    uint8_t midstate[32];
    uint8_t midstate1[32];
    uint8_t midstate2[32];
    uint8_t midstate3[32];
} job_packet_t;

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
bool nerdnos_proccess_work(uint32_t version, uint16_t timeout, task_result *result);

// test difficulty
double nerdnos_test_nonce_value(const bm_job_t *job, const uint32_t nonce, const uint32_t rolled_version, uint8_t hash_result[32]);

// free allocated RAM for strings on bm_job_t
void nerdnos_free_bm_job(bm_job_t *job);

void nerdnos_init();
