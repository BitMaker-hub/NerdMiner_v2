#pragma once

#include <stdio.h>
#include "nerdnos.h"
#include "crc.h"

#define CRC5_MASK 0x1F

#define BM1397_SERIALTX_DEBUG false
#define BM1397_SERIALRX_DEBUG true //false
#define BM1397_DEBUG_WORK false //causes insane amount of debug output

uint8_t BM1397_init(uint64_t frequency, uint16_t asic_count);

void BM1397_send_work(bm_job_t * next_bm_job, uint8_t job_id);
void BM1397_set_job_difficulty_mask(int);
int BM1397_set_max_baud(void);
int BM1397_set_default_baud(void);
void BM1397_send_hash_frequency(float frequency);
bool BM1397_proccess_work(uint32_t version, uint16_t timeout, task_result *result);
void BM1397_read_hashrate(void);
