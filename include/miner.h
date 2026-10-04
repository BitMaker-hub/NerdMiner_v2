#ifndef MINER_H
#define MINER_H

#include <Arduino.h>
#include <stdint.h>

struct MinerJob {
    uint32_t midstate[8];
    uint32_t chunk2[16];   
    uint32_t target_high;  
};

void sha256_full(const uint8_t* data, size_t len, uint8_t hash[32]);
void sha256d_full(const uint8_t* data, size_t len, uint8_t hash[32]);
void sha256_precompute_midstate(const uint8_t* chunk1, uint32_t* midstate_out);
bool sha256d_mine_batch(const MinerJob* job, uint32_t* current_nonce, uint32_t batch_size, uint32_t* winning_nonce);

#endif // MINER_H
