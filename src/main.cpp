#include <Arduino.h>
#include "miner.h"
#include "stratum.h"

static volatile uint32_t core0_hashes = 0;
static volatile uint32_t core1_hashes = 0;

void miner_worker_core0(void* pvParameters) {
    uint32_t nonce = 0;
    uint32_t local_version = 0;
    MinerJob job;
    while (true) {
        if (!is_stratum_connected) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        // Check for new job
        if (local_version != current_stratum_job.version_id) {
            memcpy(&job, &current_stratum_job, sizeof(MinerJob));
            local_version = job.version_id;
            nonce = 0;
        }
        uint32_t found = 0;
        if (sha256d_mine_batch(&job, &nonce, 1000, &found)) {
            current_submission.nonce = found;
            strncpy(current_submission.job_id, job.job_id, sizeof(current_submission.job_id));
            strncpy(current_submission.extranonce2, job.extranonce2, sizeof(current_submission.extranonce2));
            strncpy(current_submission.ntime, job.ntime, sizeof(current_submission.ntime));
            current_submission.ready = true;
        }
        core0_hashes += 1000;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void miner_worker_core1(void* pvParameters) {
    uint32_t nonce = 0x80000000;
    uint32_t local_version = 0;
    MinerJob job;
    while (true) {
        if (!is_stratum_connected) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        if (local_version != current_stratum_job.version_id) {
            memcpy(&job, &current_stratum_job, sizeof(MinerJob));
            local_version = job.version_id;
            nonce = 0x80000000;
        }
        uint32_t found = 0;
        if (sha256d_mine_batch(&job, &nonce, 2000, &found)) {
            current_submission.nonce = found;
            strncpy(current_submission.job_id, job.job_id, sizeof(current_submission.job_id));
            strncpy(current_submission.extranonce2, job.extranonce2, sizeof(current_submission.extranonce2));
            strncpy(current_submission.ntime, job.ntime, sizeof(current_submission.ntime));
            current_submission.ready = true;
        }
        core1_hashes += 2000;
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n==========================================");
    Serial.println("  NerdMiner USB Stratum Firmware");
    Serial.println("==========================================");
    
    init_wifi();
    
    xTaskCreatePinnedToCore(stratum_task, "Stratum", 8192, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(miner_worker_core0, "Miner0", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(miner_worker_core1, "Miner1", 4096, NULL, 1, NULL, 1);
}

void loop() {
    static uint32_t last_time = 0;
    if (millis() - last_time >= 1000) {
        last_time = millis();
        uint32_t c0 = core0_hashes;
        uint32_t c1 = core1_hashes;
        core0_hashes = 0;
        core1_hashes = 0;
        uint32_t total = c0 + c1;
        Serial.printf("[Mining] Total: %u H/s (%.2f kH/s) | C0: %.2f kH/s | C1: %.2f kH/s\n", 
                      total, total / 1000.0f, c0 / 1000.0f, c1 / 1000.0f);
    }
}
