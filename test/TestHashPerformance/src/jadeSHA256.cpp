#define NDEBUG
#include <stdio.h>
#include <string.h>
#include <Arduino.h>
//#include <wally_address.h>
//#include <wally_transaction.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <esp_timer.h>

#include "jadeSHA256.h"
#include <math.h>
#include <string.h>

#define HASH_SIZE 32

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, data, offset)                                                                                 \
    {                                                                                                                  \
        u.num = n;                                                                                                     \
        p = (data) + (offset);                                                                                         \
        *p = u.b[3];                                                                                                   \
        *(p + 1) = u.b[2];                                                                                             \
        *(p + 2) = u.b[1];                                                                                             \
        *(p + 3) = u.b[0];                                                                                             \
    }
#endif

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(b, i)                                                                                            \
    (((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | ((uint32_t)(b)[(i) + 2] << 8)                       \
        | ((uint32_t)(b)[(i) + 3]))
#endif

//DRAM_ATTR static const uint32_t K[] = {
static const uint32_t K[] = {
    0x428A2F98,
    0x71374491,
    0xB5C0FBCF,
    0xE9B5DBA5,
    0x3956C25B,
    0x59F111F1,
    0x923F82A4,
    0xAB1C5ED5,
    0xD807AA98,
    0x12835B01,
    0x243185BE,
    0x550C7DC3,
    0x72BE5D74,
    0x80DEB1FE,
    0x9BDC06A7,
    0xC19BF174,
    0xE49B69C1,
    0xEFBE4786,
    0x0FC19DC6,
    0x240CA1CC,
    0x2DE92C6F,
    0x4A7484AA,
    0x5CB0A9DC,
    0x76F988DA,
    0x983E5152,
    0xA831C66D,
    0xB00327C8,
    0xBF597FC7,
    0xC6E00BF3,
    0xD5A79147,
    0x06CA6351,
    0x14292967,
    0x27B70A85,
    0x2E1B2138,
    0x4D2C6DFC,
    0x53380D13,
    0x650A7354,
    0x766A0ABB,
    0x81C2C92E,
    0x92722C85,
    0xA2BFE8A1,
    0xA81A664B,
    0xC24B8B70,
    0xC76C51A3,
    0xD192E819,
    0xD6990624,
    0xF40E3585,
    0x106AA070,
    0x19A4C116,
    0x1E376C08,
    0x2748774C,
    0x34B0BCB5,
    0x391C0CB3,
    0x4ED8AA4A,
    0x5B9CCA4F,
    0x682E6FF3,
    0x748F82EE,
    0x78A5636F,
    0x84C87814,
    0x8CC70208,
    0x90BEFFFA,
    0xA4506CEB,
    0xBEF9A3F7,
    0xC67178F2,
};

#define SHR(x, n) ((x & 0xFFFFFFFF) >> n)
//#define ROTR(x, n) ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define ROTR(x, n) (SHR(x, n) | ((x) << (32 - (n))))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define R(t) (W[t] = S1(W[t - 2]) + W[t - 7] + S0(W[t - 15]) + W[t - 16])

#define P(a, b, c, d, e, f, g, h, x, K)                                                                                \
    {                                                                                                                  \
        temp1 = h + S3(e) + F1(e, f, g) + K + x;                                                                       \
        temp2 = S2(a) + F0(a, b, c);                                                                                   \
        d += temp1;                                                                                                    \
        h = temp1 + temp2;                                                                                             \
    }

#define CHECK_BYTES(u1, u2, offset)                                                                                    \
    {                                                                                                                  \
        temp1 = u1 + u2;                                                                                               \
        for (int i = 0; i < 4; ++i) {                                                                                  \
            temp3 = (uint8_t)((temp1 >> (i * 8)) & 0xff);                                                              \
            temp4 = *(target + offset + i);                                                                            \
            if (__builtin_expect(temp4 < temp3, true)) {                                                               \
                return false;                                                                                          \
            }                                                                                                          \
            if (__builtin_expect(temp4 > temp3, false)) {                                                              \
                return true;                                                                                           \
            }                                                                                                          \
        }                                                                                                              \
    }

#define MAINET_TESTNET_INTERVAL 210000
#define REGTEST_INTERVAL 150

const char* TAG = "MINER";

typedef struct {
    uint32_t version;
    uint8_t prev_block[32];
    uint8_t merkle_root[32];
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} block_header;

typedef struct headerandtarget {
    block_header bh;
    uint8_t target[32];
} headerandtarget;

typedef struct task_ctx {
    headerandtarget ht;
    uint32_t hashespersec;
    uint32_t nonce_start;
    uint32_t* nonce_solution;
    uint8_t task_n;
    bool* solution_found;
    bool newwork;
} task_ctx;

typedef struct miner_ctx {
    uint8_t rawtx[300];
    block_header bh;
    int64_t start;
    TaskHandle_t xHandle1;
    TaskHandle_t xHandle2;
    solution_cb cb;
    void* cbctx;
    task_ctx ctx1;
    task_ctx ctx2;
    size_t txlen;
    bool solution_found;
} miner_ctx;

IRAM_ATTR void calc_midstate(uint8_t* buf_ptr, _sha256_context* midstate)
{
    uint32_t A[8] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };

    uint32_t temp1, temp2, W[64];
    uint8_t i;

    /*for (i = 0; i < 16; i++) {
        W[i] = GET_UINT32_BE(buf_ptr, 4 * i);
    }*/
    
    W[0] = GET_UINT32_BE(buf_ptr, 0);
    W[1] = GET_UINT32_BE(buf_ptr, 4);
    W[2] = GET_UINT32_BE(buf_ptr, 8);
    W[3] = GET_UINT32_BE(buf_ptr, 12);
    W[4] = GET_UINT32_BE(buf_ptr, 16);
    W[5] = GET_UINT32_BE(buf_ptr, 20);
    W[6] = GET_UINT32_BE(buf_ptr, 24);
    W[7] = GET_UINT32_BE(buf_ptr, 28);
    W[8] = GET_UINT32_BE(buf_ptr, 32);
    W[9] = GET_UINT32_BE(buf_ptr, 36);
    W[10] = GET_UINT32_BE(buf_ptr, 40);
    W[11] = GET_UINT32_BE(buf_ptr, 44);
    W[12] = GET_UINT32_BE(buf_ptr, 48);
    W[13] = GET_UINT32_BE(buf_ptr, 52);
    W[14] = GET_UINT32_BE(buf_ptr, 56);
    W[15] = GET_UINT32_BE(buf_ptr, 60);
    

    
    for (i = 0; i < 16; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4],
          A[5], A[6], A[7], W[i+0], K[i+0]);
        P(A[7], A[0], A[1], A[2], A[3],
          A[4], A[5], A[6], W[i+1], K[i+1]);
        P(A[6], A[7], A[0], A[1], A[2],
          A[3], A[4], A[5], W[i+2], K[i+2]);
        P(A[5], A[6], A[7], A[0], A[1],
          A[2], A[3], A[4], W[i+3], K[i+3]);
        P(A[4], A[5], A[6], A[7], A[0],
          A[1], A[2], A[3], W[i+4], K[i+4]);
        P(A[3], A[4], A[5], A[6], A[7],
          A[0], A[1], A[2], W[i+5], K[i+5]);
        P(A[2], A[3], A[4], A[5], A[6],
          A[7], A[0], A[1], W[i+6], K[i+6]);
        P(A[1], A[2], A[3], A[4], A[5],
          A[6], A[7], A[0], W[i+7], K[i+7]);
    }

    for (i = 16; i < 64; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4],
          A[5], A[6], A[7], R(i+0), K[i+0]);
        P(A[7], A[0], A[1], A[2], A[3],
          A[4], A[5], A[6], R(i+1), K[i+1]);
        P(A[6], A[7], A[0], A[1], A[2],
          A[3], A[4], A[5], R(i+2), K[i+2]);
        P(A[5], A[6], A[7], A[0], A[1],
          A[2], A[3], A[4], R(i+3), K[i+3]);
        P(A[4], A[5], A[6], A[7], A[0],
          A[1], A[2], A[3], R(i+4), K[i+4]);
        P(A[3], A[4], A[5], A[6], A[7],
          A[0], A[1], A[2], R(i+5), K[i+5]);
        P(A[2], A[3], A[4], A[5], A[6],
          A[7], A[0], A[1], R(i+6), K[i+6]);
        P(A[1], A[2], A[3], A[4], A[5],
          A[6], A[7], A[0], R(i+7), K[i+7]);
    }

    for (i = 0; i < 8; i++) {
        midstate->state[i] += A[i];
    }
    /*
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[0], K[0]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[1], K[1]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[2], K[2]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[3], K[3]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[4], K[4]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[5], K[5]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[6], K[6]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[7], K[7]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[8], K[8]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[9], K[9]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[10], K[10]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[11], K[11]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[12], K[12]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[13], K[13]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[14], K[14]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[15], K[15]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(16), K[16]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(17), K[17]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(18), K[18]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(19), K[19]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(20), K[20]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(21), K[21]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(22), K[22]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(23), K[23]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(24), K[24]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(25), K[25]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(26), K[26]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(27), K[27]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(28), K[28]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(29), K[29]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(30), K[30]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(31), K[31]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(32), K[32]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(33), K[33]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(34), K[34]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(35), K[35]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(36), K[36]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(37), K[37]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(38), K[38]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(39), K[39]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(40), K[40]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(41), K[41]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(42), K[42]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(43), K[43]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(44), K[44]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(45), K[45]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(46), K[46]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(47), K[47]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(48), K[48]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(49), K[49]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(50), K[50]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(51), K[51]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(52), K[52]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(53), K[53]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(54), K[54]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(55), K[55]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(56), K[56]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(57), K[57]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(58), K[58]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(59), K[59]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(60), K[60]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(61), K[61]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(62), K[62]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(63), K[63]);

    midstate->state[0] = 0x6A09E667 + A[0];
    midstate->state[1] = 0xBB67AE85 + A[1];
    midstate->state[2] = 0x3C6EF372 + A[2];
    midstate->state[3] = 0xA54FF53A + A[3];
    midstate->state[4] = 0x510E527F + A[4];
    midstate->state[5] = 0x9B05688C + A[5];
    midstate->state[6] = 0x1F83D9AB + A[6];
    midstate->state[7] = 0x5BE0CD19 + A[7];
    */
    //midstate->buffer[16] = 0x80;
    memcpy(midstate->buffer, buf_ptr + 64, 12);
}

IRAM_ATTR bool make_double_sha(_sha256_context* midstate)
{
    uint32_t temp1, temp2;
    uint8_t temp3, temp4;

    uint32_t W[64] = { GET_UINT32_BE(midstate->buffer, 0), GET_UINT32_BE(midstate->buffer, 4),
        GET_UINT32_BE(midstate->buffer, 8), GET_UINT32_BE(midstate->buffer, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 640, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint32_t A[8] = { midstate->state[0], midstate->state[1], midstate->state[2], midstate->state[3],
        midstate->state[4], midstate->state[5], midstate->state[6], midstate->state[7] };
//0x80000000
    union {
        uint32_t num;
        uint8_t b[4];
    } u;
    uint8_t* p = NULL;

    uint8_t i;

    for (i = 0; i < 16; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4],
          A[5], A[6], A[7], W[i+0], K[i+0]);
        P(A[7], A[0], A[1], A[2], A[3],
          A[4], A[5], A[6], W[i+1], K[i+1]);
        P(A[6], A[7], A[0], A[1], A[2],
          A[3], A[4], A[5], W[i+2], K[i+2]);
        P(A[5], A[6], A[7], A[0], A[1],
          A[2], A[3], A[4], W[i+3], K[i+3]);
        P(A[4], A[5], A[6], A[7], A[0],
          A[1], A[2], A[3], W[i+4], K[i+4]);
        P(A[3], A[4], A[5], A[6], A[7],
          A[0], A[1], A[2], W[i+5], K[i+5]);
        P(A[2], A[3], A[4], A[5], A[6],
          A[7], A[0], A[1], W[i+6], K[i+6]);
        P(A[1], A[2], A[3], A[4], A[5],
          A[6], A[7], A[0], W[i+7], K[i+7]);
    }

    for (i = 16; i < 64; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4],
          A[5], A[6], A[7], R(i+0), K[i+0]);
        P(A[7], A[0], A[1], A[2], A[3],
          A[4], A[5], A[6], R(i+1), K[i+1]);
        P(A[6], A[7], A[0], A[1], A[2],
          A[3], A[4], A[5], R(i+2), K[i+2]);
        P(A[5], A[6], A[7], A[0], A[1],
          A[2], A[3], A[4], R(i+3), K[i+3]);
        P(A[4], A[5], A[6], A[7], A[0],
          A[1], A[2], A[3], R(i+4), K[i+4]);
        P(A[3], A[4], A[5], A[6], A[7],
          A[0], A[1], A[2], R(i+5), K[i+5]);
        P(A[2], A[3], A[4], A[5], A[6],
          A[7], A[0], A[1], R(i+6), K[i+6]);
        P(A[1], A[2], A[3], A[4], A[5],
          A[6], A[7], A[0], R(i+7), K[i+7]);
    }

    /*
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[0], K[0]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[1], K[1]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[2], K[2]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[3], K[3]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[4], K[4]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[5], K[5]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[6], K[6]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[7], K[7]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[8], K[8]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[9], K[9]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[10], K[10]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[11], K[11]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[12], K[12]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[13], K[13]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[14], K[14]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[15], K[15]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(16), K[16]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(17), K[17]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(18), K[18]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(19), K[19]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(20), K[20]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(21), K[21]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(22), K[22]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(23), K[23]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(24), K[24]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(25), K[25]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(26), K[26]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(27), K[27]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(28), K[28]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(29), K[29]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(30), K[30]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(31), K[31]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(32), K[32]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(33), K[33]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(34), K[34]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(35), K[35]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(36), K[36]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(37), K[37]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(38), K[38]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(39), K[39]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(40), K[40]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(41), K[41]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(42), K[42]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(43), K[43]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(44), K[44]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(45), K[45]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(46), K[46]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(47), K[47]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(48), K[48]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(49), K[49]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(50), K[50]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(51), K[51]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(52), K[52]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(53), K[53]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(54), K[54]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(55), K[55]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(56), K[56]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(57), K[57]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(58), K[58]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(59), K[59]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(60), K[60]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(61), K[61]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(62), K[62]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(63), K[63]);
    */

    PUT_UINT32_BE(midstate->state[0] + A[0], midstate->buffer, 0);
    PUT_UINT32_BE(midstate->state[1] + A[1], midstate->buffer, 4);
    PUT_UINT32_BE(midstate->state[2] + A[2], midstate->buffer, 8);
    PUT_UINT32_BE(midstate->state[3] + A[3], midstate->buffer, 12);
    PUT_UINT32_BE(midstate->state[4] + A[4], midstate->buffer, 16);
    PUT_UINT32_BE(midstate->state[5] + A[5], midstate->buffer, 20);
    PUT_UINT32_BE(midstate->state[6] + A[6], midstate->buffer, 24);
    PUT_UINT32_BE(midstate->state[7] + A[7], midstate->buffer, 28);

    /* Calculate the second hash (double SHA-256) */
    A[0] = 0x6A09E667;
    A[1] = 0xBB67AE85;
    A[2] = 0x3C6EF372;
    A[3] = 0xA54FF53A;
    A[4] = 0x510E527F;
    A[5] = 0x9B05688C;
    A[6] = 0x1F83D9AB;
    A[7] = 0x5BE0CD19;

    midstate->buffer[32] = 0x80;
    W[0] = GET_UINT32_BE(midstate->buffer, 0);
    W[1] = GET_UINT32_BE(midstate->buffer, 4);
    W[2] = GET_UINT32_BE(midstate->buffer, 8);
    W[3] = GET_UINT32_BE(midstate->buffer, 12);
    W[4] = GET_UINT32_BE(midstate->buffer, 16);
    W[5] = GET_UINT32_BE(midstate->buffer, 20);
    W[6] = GET_UINT32_BE(midstate->buffer, 24);
    W[7] = GET_UINT32_BE(midstate->buffer, 28);
    W[8] = GET_UINT32_BE(midstate->buffer, 32);
    W[9] = GET_UINT32_BE(midstate->buffer, 36);
    W[10] = GET_UINT32_BE(midstate->buffer, 40);
    W[11] = GET_UINT32_BE(midstate->buffer, 44);
    W[12] = GET_UINT32_BE(midstate->buffer, 48);
    W[13] = GET_UINT32_BE(midstate->buffer, 52);
    W[14] = 0;
    W[15] = 256;

    for (i = 0; i < 16; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4],
          A[5], A[6], A[7], W[i+0], K[i+0]);
        P(A[7], A[0], A[1], A[2], A[3],
          A[4], A[5], A[6], W[i+1], K[i+1]);
        P(A[6], A[7], A[0], A[1], A[2],
          A[3], A[4], A[5], W[i+2], K[i+2]);
        P(A[5], A[6], A[7], A[0], A[1],
          A[2], A[3], A[4], W[i+3], K[i+3]);
        P(A[4], A[5], A[6], A[7], A[0],
          A[1], A[2], A[3], W[i+4], K[i+4]);
        P(A[3], A[4], A[5], A[6], A[7],
          A[0], A[1], A[2], W[i+5], K[i+5]);
        P(A[2], A[3], A[4], A[5], A[6],
          A[7], A[0], A[1], W[i+6], K[i+6]);
        P(A[1], A[2], A[3], A[4], A[5],
          A[6], A[7], A[0], W[i+7], K[i+7]);
    }

    for (i = 16; i < 64; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4],
          A[5], A[6], A[7], R(i+0), K[i+0]);
        P(A[7], A[0], A[1], A[2], A[3],
          A[4], A[5], A[6], R(i+1), K[i+1]);
        P(A[6], A[7], A[0], A[1], A[2],
          A[3], A[4], A[5], R(i+2), K[i+2]);
        P(A[5], A[6], A[7], A[0], A[1],
          A[2], A[3], A[4], R(i+3), K[i+3]);
        P(A[4], A[5], A[6], A[7], A[0],
          A[1], A[2], A[3], R(i+4), K[i+4]);
        P(A[3], A[4], A[5], A[6], A[7],
          A[0], A[1], A[2], R(i+5), K[i+5]);
        P(A[2], A[3], A[4], A[5], A[6],
          A[7], A[0], A[1], R(i+6), K[i+6]);
        P(A[1], A[2], A[3], A[4], A[5],
          A[6], A[7], A[0], R(i+7), K[i+7]);
    }

    /*
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[0], K[0]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[1], K[1]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[2], K[2]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[3], K[3]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[4], K[4]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[5], K[5]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[6], K[6]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[7], K[7]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[8], K[8]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[9], K[9]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[10], K[10]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[11], K[11]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[12], K[12]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[13], K[13]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[14], K[14]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[15], K[15]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(16), K[16]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(17), K[17]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(18), K[18]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(19), K[19]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(20), K[20]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(21), K[21]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(22), K[22]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(23), K[23]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(24), K[24]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(25), K[25]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(26), K[26]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(27), K[27]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(28), K[28]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(29), K[29]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(30), K[30]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(31), K[31]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(32), K[32]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(33), K[33]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(34), K[34]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(35), K[35]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(36), K[36]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(37), K[37]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(38), K[38]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(39), K[39]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(40), K[40]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(41), K[41]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(42), K[42]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(43), K[43]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(44), K[44]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(45), K[45]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(46), K[46]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(47), K[47]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(48), K[48]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(49), K[49]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(50), K[50]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(51), K[51]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(52), K[52]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(53), K[53]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(54), K[54]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(55), K[55]);
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(56), K[56]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(57), K[57]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(58), K[58]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(59), K[59]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(60), K[60]);

    //CHECK_BYTES(0x5BE0CD19, A[7], 0);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(61), K[61]);
    //CHECK_BYTES(0x1F83D9AB, A[6], 4);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(62), K[62]);
    //CHECK_BYTES(0x9B05688C, A[5], 8);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(63), K[63]);
    */
    /*CHECK_BYTES(0x510E527F, A[4], 12);
    CHECK_BYTES(0xA54FF53A, A[3], 16);
    CHECK_BYTES(0x3C6EF372, A[2], 20);
    CHECK_BYTES(0xBB67AE85, A[1], 24);
    CHECK_BYTES(0x6A09E667, A[0], 28);*/

    PUT_UINT32_BE(midstate->state[0] + A[0], midstate->buffer, 0);
    PUT_UINT32_BE(midstate->state[1] + A[1], midstate->buffer, 4);
    PUT_UINT32_BE(midstate->state[2] + A[2], midstate->buffer, 8);
    PUT_UINT32_BE(midstate->state[3] + A[3], midstate->buffer, 12);
    PUT_UINT32_BE(midstate->state[4] + A[4], midstate->buffer, 16);
    PUT_UINT32_BE(midstate->state[5] + A[5], midstate->buffer, 20);
    PUT_UINT32_BE(midstate->state[6] + A[6], midstate->buffer, 24);
    PUT_UINT32_BE(midstate->state[7] + A[7], midstate->buffer, 28);

    return true;
}

static void minertask(void* pctx)
{
    assert(pctx);
    task_ctx* tctx ;

    headerandtarget header;
    bool* newwork = &tctx->newwork;
    while (1) {
        if (*newwork) {
            *newwork = false;
            break;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    header = tctx->ht;

    uint32_t* hashespersec = &tctx->hashespersec;

    while (true) {
        _sha256_context midstate_cached = { 0 };
        calc_midstate((uint8_t*)&header.bh, &midstate_cached);

        *((uint32_t*)&midstate_cached.buffer[12]) = tctx->nonce_start;
        _sha256_context ctx = midstate_cached;
        while (true) {
            //const bool within = verify_nonce(&ctx, header.target);
            const bool within = false;
            if (__builtin_expect(within, false)) {
                *tctx->nonce_solution = *((uint32_t*)&midstate_cached.buffer[12]);
                *tctx->solution_found = true;

                /* wait until we have a new header to work on */
                while (1) {
                    if (__builtin_expect(*newwork, false)) {
                        *newwork = false;
                        header = tctx->ht;
                        break;
                    }
                    vTaskDelay(1 / portTICK_PERIOD_MS);
                }
                break;
            }

            if (__builtin_expect(*newwork, false)) {
                *newwork = false;
                header = tctx->ht;
                break;
            }
            *hashespersec = (*((uint32_t*)&midstate_cached.buffer[12]) += 1) - tctx->nonce_start;
            ctx = midstate_cached;
        }
    }
}

bool check_solutions(void* ctx)
{
    assert(ctx);
    miner_ctx* mctx;
    /* missing memory barrier but appers to work */
    /* FIXME: find upper bound for solution len ?*/
    if (!mctx->solution_found) {
        return false;
    }

    uint8_t solution[600];
    memcpy(solution, &mctx->bh, 80);
    solution[80] = 0x01; /* number of transactions, solo mining :( */
    memcpy(solution + 81, mctx->rawtx, mctx->txlen);
    mctx->cb(mctx->cbctx, solution, 81 + mctx->txlen);
    mctx->solution_found = false;
    return true;
}

void check_speed(void* ctx, uint32_t* speed)
{
    /* missing memory barrier but appers to work */
    assert(ctx);
    miner_ctx* mctx;
    *speed = ((mctx->ctx1.hashespersec + mctx->ctx2.hashespersec) / ((esp_timer_get_time() - mctx->start) / 1000000.0));
}