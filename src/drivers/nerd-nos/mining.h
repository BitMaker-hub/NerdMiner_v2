#pragma once
#include <stdint.h>
#include "common.h"
//#include "stratum.h"


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
} bm_job_t;


void asic_send_work(bm_job_t *next_bm_job, uint8_t job_id);
task_result *asic_proccess_work(uint32_t version, uint16_t timeout);

double asic_test_nonce_value(const bm_job_t *job, const uint32_t nonce, const uint32_t rolled_version);
void asic_free_bm_job(bm_job_t *job);