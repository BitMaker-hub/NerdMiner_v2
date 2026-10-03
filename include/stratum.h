#ifndef STRATUM_H
#define STRATUM_H

#include "miner.h"
#include <Arduino.h>

extern MinerJob current_stratum_job;
extern volatile bool is_stratum_connected;

struct ShareSubmission {
    char job_id[32];
    char extranonce2[16];
    char ntime[16];
    uint32_t nonce;
    volatile bool ready;
};

extern ShareSubmission current_submission;

void init_wifi();
void stratum_task(void* pvParameters);
void submit_share(uint32_t nonce, const MinerJob* job);

#endif // STRATUM_H
