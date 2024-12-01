#pragma once

#define NERDNOS_JOB_INTERVAL_MS 30

void runASIC(void * task_id);
void runASIC_RX(void * task_id);
double nerdnos_get_avg_hashrate();
