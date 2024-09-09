#pragma once

#include <stdio.h>
#include "common.h"
#include "mining.h"
#include "crc.h"

#define CRC5_MASK 0x1F
#define BM1397_INITIAL_DIFFICULTY 64

#define BM1937_SERIALTX_DEBUG false
#define BM1937_SERIALRX_DEBUG true //false
#define BM1397_DEBUG_WORK false //causes insane amount of debug output


typedef struct
{
    float frequency;
} bm1397Module;

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
} job_packet;

uint8_t BM1397_init(uint64_t frequency, uint16_t asic_count);

void BM1397_send_work(bm_job_t * next_bm_job, uint8_t job_id);
void BM1397_set_job_difficulty_mask(int);
int BM1397_set_max_baud(void);
int BM1397_set_default_baud(void);
void BM1397_send_hash_frequency(float frequency);
task_result *BM1397_proccess_work(uint32_t version, uint16_t timeout);

