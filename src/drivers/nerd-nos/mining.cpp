#include <Arduino.h>
#include <ArduinoJson.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bm1397.h"
#include "mining.h"
#include "utils.h"
#include "stratum.h"
#include "mbedtls/sha256.h"
#include "../../mining.h"

///////cgminer nonce testing
/* truediffone == 0x00000000FFFF0000000000000000000000000000000000000000000000000000
 */
static const double truediffone = 26959535291011309493156476344723991336010898738574164086137773096960.0;

/* testing a nonce and return the diff - 0 means invalid */
double nerdnos_test_nonce_value(const bm_job_t *job, const uint32_t nonce, const uint32_t rolled_version, uint8_t hash_result[32])
{
    double d64, s64, ds;
    unsigned char header[80];

    // // TODO: use the midstate hash instead of hashing the whole header
    // uint32_t rolled_version = job->version;
    // for (int i = 0; i < midstate_index; i++) {
    //     rolled_version = increment_bitmask(rolled_version, job->version_mask);
    // }

    // copy data from job to header
    memcpy(header, &rolled_version, 4);
    memcpy(header + 4, job->prev_block_hash, 32);
    memcpy(header + 36, job->merkle_root, 32);
    memcpy(header + 68, &job->ntime, 4);
    memcpy(header + 72, &job->target, 4);
    memcpy(header + 76, &nonce, 4);

    unsigned char hash_buffer[32];

    // double hash the header
    mbedtls_sha256(header, 80, hash_buffer, 0);
    mbedtls_sha256(hash_buffer, 32, hash_result, 0);

    d64 = truediffone;
    s64 = le256todouble(hash_result);
    ds = d64 / s64;

    return ds;
}

static void dump(mining_job *job) {
    Serial.printf("job_id: %s\n", job->job_id.c_str());
    Serial.printf("prev_block_hash: %s\n", job->prev_block_hash.c_str());
    Serial.printf("coinb1: %s\n", job->coinb1.c_str());
    Serial.printf("coinb2: %s\n", job->coinb2.c_str());
    Serial.printf("nbits: %s\n", job->nbits.c_str());
    Serial.printf("version: %s\n", job->version.c_str());
    Serial.printf("ntime: %s\n", job->ntime.c_str());
    Serial.printf("taget: %lu\n", job->target);
    Serial.printf("clean_jobs: %s\n", job->clean_jobs ? "true" : "false");
    for (size_t i = 0; i < job->merkle_branch.size(); i++) {
        const char* m = job->merkle_branch[i];
        Serial.printf("merkle_branch[%d]: %s\n", i, m);
    }
}
static void calculate_merkle_root_hash(const char *coinbase_tx, mining_job* job, char merkle_root_hash[65])
{
    size_t coinbase_tx_bin_len = strlen(coinbase_tx) / 2;
    uint8_t coinbase_tx_bin[coinbase_tx_bin_len];
    hex2bin(coinbase_tx, coinbase_tx_bin, coinbase_tx_bin_len);

    uint8_t both_merkles[64];
    uint8_t new_root[32];
    double_sha256_bin(coinbase_tx_bin, coinbase_tx_bin_len, new_root);
    memcpy(both_merkles, new_root, 32);

    for (size_t i = 0; i < job->merkle_branch.size(); i++) {
        const char* m = job->merkle_branch[i];
        // if merkle branch is not what we expect, dump the job
        if (!is_hex_string(m)) {
            dump(job);
        }
        hex2bin(m, &both_merkles[32], 32);
        double_sha256_bin(both_merkles, 64, new_root);
        memcpy(both_merkles, new_root, 32);
    }
    bin2hex(both_merkles, 32, merkle_root_hash, 65);
}

// take a mining_notify struct with ascii hex strings and convert it to a bm_job struct
static void construct_bm_job(mining_job *job, const char *merkle_root, uint32_t version_mask, bm_job_t *new_job)
{
    new_job->version = strtoul(job->version.c_str(), NULL, 16);
    new_job->target = strtoul(job->nbits.c_str(), NULL, 16);
    new_job->ntime = strtoul(job->ntime.c_str(), NULL, 16);
    new_job->starting_nonce = 0;

    hex2bin(merkle_root, new_job->merkle_root, 32);

    // hex2bin(merkle_root, new_job.merkle_root_be, 32);
    swap_endian_words(merkle_root, new_job->merkle_root_be);
    reverse_bytes(new_job->merkle_root_be, 32);

    swap_endian_words(job->prev_block_hash.c_str(), new_job->prev_block_hash);

    hex2bin(job->prev_block_hash.c_str(), new_job->prev_block_hash_be, 32);
    reverse_bytes(new_job->prev_block_hash_be, 32);

    ////make the midstate hash
    uint8_t midstate_data[64];

    // copy 68 bytes header data into midstate (and deal with endianess)
    memcpy(midstate_data, &new_job->version, 4);             // copy version
    memcpy(midstate_data + 4, new_job->prev_block_hash, 32); // copy prev_block_hash
    memcpy(midstate_data + 36, new_job->merkle_root, 28);    // copy merkle_root

    midstate_sha256_bin(midstate_data, 64, new_job->midstate); // make the midstate hash
    reverse_bytes(new_job->midstate, 32);                      // reverse the midstate bytes for the BM job packet

    uint32_t rolled_version = increment_bitmask(new_job->version, version_mask);
    memcpy(midstate_data, &rolled_version, 4);
    midstate_sha256_bin(midstate_data, 64, new_job->midstate1);
    reverse_bytes(new_job->midstate1, 32);

    rolled_version = increment_bitmask(rolled_version, version_mask);
    memcpy(midstate_data, &rolled_version, 4);
    midstate_sha256_bin(midstate_data, 64, new_job->midstate2);
    reverse_bytes(new_job->midstate2, 32);

    rolled_version = increment_bitmask(rolled_version, version_mask);
    memcpy(midstate_data, &rolled_version, 4);
    midstate_sha256_bin(midstate_data, 64, new_job->midstate3);
    reverse_bytes(new_job->midstate3, 32);
    new_job->num_midstates = 4;
}

void nerdnos_create_job(mining_subscribe *mWorker, mining_job *job, bm_job_t *next_job, uint32_t extranonce_2, uint32_t stratum_difficulty) {
    char extranonce_2_str[mWorker->extranonce2_size * 2 + 1]; // +1 zero termination
    snprintf(extranonce_2_str, sizeof(extranonce_2_str), "%0*lx", (int) mWorker->extranonce2_size * 2, extranonce_2);

    // generate coinbase tx
    String coinbase_tx = job->coinb1 + mWorker->extranonce1 + extranonce_2_str + job->coinb2;

    // calculate merkle root
    char merkle_root[65];
    calculate_merkle_root_hash(coinbase_tx.c_str(), job, merkle_root);

    //Serial.printf("asic merkle root: %s\n", merkle_root);
    construct_bm_job(job, merkle_root, 0x1fffe000, next_job);

    next_job->jobid = strdup(job->job_id.c_str());
    next_job->extranonce2 = strdup(extranonce_2_str);
    next_job->pool_diff = stratum_difficulty;
}

void nerdnos_send_work(bm_job_t *next_bm_job, uint8_t job_id) {
    BM1397_send_work(next_bm_job, job_id);
}

task_result *nerdnos_proccess_work(uint32_t version, uint16_t timeout) {
    return BM1397_proccess_work(version, timeout);
}

void nerdnos_free_bm_job(bm_job_t *job) {
    free(job->jobid);
    free(job->extranonce2);
    // mark as free
    job->ntime = 0;
}

void nerdnos_set_asic_difficulty(uint32_t difficulty) {
    BM1397_set_job_difficulty_mask(difficulty);
}