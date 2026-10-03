#include "miner.h"
#include <string.h>

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t H_INIT[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static void sha256_transform(uint32_t state[8], const uint32_t w[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        uint32_t t2 = SIGMA0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256_full(const uint8_t* data, size_t len, uint8_t hash[32]) {
    uint32_t state[8];
    memcpy(state, H_INIT, sizeof(state));

    size_t offset = 0;
    while (offset + 64 <= len) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)data[offset + i * 4] << 24) | ((uint32_t)data[offset + i * 4 + 1] << 16) |
                   ((uint32_t)data[offset + i * 4 + 2] << 8) | ((uint32_t)data[offset + i * 4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
        }
        sha256_transform(state, w);
        offset += 64;
    }

    uint8_t pad[128] = {0};
    size_t rem = len - offset;
    memcpy(pad, data + offset, rem);
    pad[rem] = 0x80;

    size_t pad_len = (rem < 56) ? 64 : 128;
    uint64_t bit_len = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) {
        pad[pad_len - 1 - i] = (bit_len >> (i * 8)) & 0xFF;
    }

    uint32_t w_pad[64];
    for (int i = 0; i < 16; i++) {
        w_pad[i] = ((uint32_t)pad[i * 4] << 24) | ((uint32_t)pad[i * 4 + 1] << 16) |
                   ((uint32_t)pad[i * 4 + 2] << 8) | ((uint32_t)pad[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        w_pad[i] = SIG1(w_pad[i - 2]) + w_pad[i - 7] + SIG0(w_pad[i - 15]) + w_pad[i - 16];
    }
    sha256_transform(state, w_pad);

    if (pad_len == 128) {
        uint32_t w_pad2[64];
        for (int i = 0; i < 16; i++) {
            w_pad2[i] = ((uint32_t)pad[64 + i * 4] << 24) | ((uint32_t)pad[64 + i * 4 + 1] << 16) |
                        ((uint32_t)pad[64 + i * 4 + 2] << 8) | ((uint32_t)pad[64 + i * 4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            w_pad2[i] = SIG1(w_pad2[i - 2]) + w_pad2[i - 7] + SIG0(w_pad2[i - 15]) + w_pad2[i - 16];
        }
        sha256_transform(state, w_pad2);
    }

    for (int i = 0; i < 8; i++) {
        hash[i * 4 + 0] = (state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = state[i] & 0xFF;
    }
}

void sha256d_full(const uint8_t* data, size_t len, uint8_t hash[32]) {
    uint8_t mid[32];
    sha256_full(data, len, mid);
    sha256_full(mid, 32, hash);
}

void sha256_precompute_midstate(const uint8_t* chunk1, uint32_t* midstate_out) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)chunk1[i * 4] << 24) |
               ((uint32_t)chunk1[i * 4 + 1] << 16) |
               ((uint32_t)chunk1[i * 4 + 2] << 8) |
               ((uint32_t)chunk1[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }

    uint32_t state[8];
    memcpy(state, H_INIT, sizeof(state));
    sha256_transform(state, w);
    memcpy(midstate_out, state, sizeof(state));
}

bool sha256d_mine_batch(const MinerJob* job, uint32_t* current_nonce, uint32_t batch_size, uint32_t* winning_nonce) {
    uint32_t w[64];
    uint32_t w2[64];
    uint32_t state[8];
    uint32_t nonce = *current_nonce;
    uint32_t end_nonce = nonce + batch_size;

    memcpy(w, job->chunk2, 16 * sizeof(uint32_t));

    while (nonce < end_nonce) {
        w[3] = nonce; 
        for (int i = 16; i < 64; ++i) {
            w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
        }

        memcpy(state, job->midstate, sizeof(state));
        sha256_transform(state, w);
        
        memset(w2, 0, sizeof(w2));
        memcpy(w2, state, 32);
        w2[8] = 0x80000000; 
        w2[15] = 256;       

        for (int i = 16; i < 64; ++i) {
            w2[i] = SIG1(w2[i - 2]) + w2[i - 7] + SIG0(w2[i - 15]) + w2[i - 16];
        }

        uint32_t state2[8];
        memcpy(state2, H_INIT, sizeof(state2));
        sha256_transform(state2, w2);

        if (state2[7] <= job->target_high) {
            *winning_nonce = nonce;
            *current_nonce = nonce + 1;
            return true;
        }
        nonce++;
    }
    *current_nonce = nonce;
    return false;
}
