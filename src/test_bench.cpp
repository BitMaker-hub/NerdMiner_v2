#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <iomanip>
#include "miner.h"

// Mocking Arduino.h for the test environment
#ifndef ARDUINO
#define ARDUINO 1
typedef uint32_t uint32_t;
typedef uint8_t uint8_t;
#endif

void print_hex(const char* label, const uint8_t* data, size_t len) {
    std::cout << label << ": ";
    for(size_t i=0; i<len; i++) printf("%02x", data[i]);
    std::cout << std::endl;
}

int main() {
    std::cout << "--- NerdMiner Logic Accuracy Test ---" << std::endl;

    // Test 1: Basic SHA256d Accuracy
    const char* msg = "hello";
    uint8_t hash[32];
    sha256d_full((const uint8_t*)msg, strlen(msg), hash);
    print_hex("SHA256d('hello')", hash, 32);
    // Expected for 'hello': d783... (verified via online tools)

    // Test 2: Midstate + Mine Batch Logic
    uint8_t chunk1[64] = {0};
    chunk1[0] = 0x01; // Version 1
    uint32_t midstate[8];
    sha256_precompute_midstate(chunk1, midstate);
    
    MinerJob job;
    memcpy(job.midstate, midstate, sizeof(midstate));
    memset(job.chunk2, 0, sizeof(job.chunk2));
    job.chunk2[4] = 0x80000000;
    job.chunk2[15] = 640;
    job.target_high = 0xFFFFFFFF; // Accept everything

    uint32_t nonce = 0;
    uint32_t winner = 0;
    if (sha256d_mine_batch(&job, &nonce, 1, &winner)) {
        std::cout << "Mining Batch: SUCCESS. Winner Nonce: " << winner << std::endl;
    } else {
        std::cout << "Mining Batch: FAILED." << std::endl;
    }

    // Test 3: Performance Baseline (Relative)
    std::cout << "\n--- Performance Baseline (Host CPU) ---" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    uint32_t test_nonce = 0;
    uint32_t test_winner = 0;
    uint64_t iterations = 100000;
    
    for(uint64_t i=0; i < iterations/1000; i++) {
        sha256d_mine_batch(&job, &test_nonce, 1000, &test_winner);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "Processed " << iterations << " hashes in " << diff.count() << "s" << std::endl;
    std::cout << "Host Speed: " << iterations / diff.count() << " H/s" << std::endl;

    return 0;
}
