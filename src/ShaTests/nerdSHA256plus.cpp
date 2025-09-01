/************************************************************************************
*   written by: Bitmaker
*   based on: Blockstream Jade shaLib
*   thanks to @LarryBitcoin

*   Description:

*   NerdSha256plus is a custom C implementation of sha256d based on Blockstream Jade 
    code https://github.com/Blockstream/Jade

    The folowing file can be used on any ESP32 implementation using both cores

*************************************************************************************/

#define NDEBUG
#include <stdio.h>
#include <string.h>
#include <Arduino.h>

#include <esp_log.h>
#include <esp_timer.h>

#include "nerdSHA256plus.h"
#include <math.h>
#include <string.h>

//#pragma GCC optimize ("Ofast")
//#pragma GCC optimize ("jump-tables")
//#pragma GCC optimize ("tree-switch-conversion")
//#pragma GCC optimize ("no-stack-check")

#define ROTR(x, n) ((x >> n) | (x << ((sizeof(x) << 3) - n)))

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
DRAM_ATTR static const uint32_t K[64] = {
        0x428A2F98L, 0x71374491L, 0xB5C0FBCFL, 0xE9B5DBA5L, 0x3956C25BL,
        0x59F111F1L, 0x923F82A4L, 0xAB1C5ED5L, 0xD807AA98L, 0x12835B01L,
        0x243185BEL, 0x550C7DC3L, 0x72BE5D74L, 0x80DEB1FEL, 0x9BDC06A7L,
        0xC19BF174L, 0xE49B69C1L, 0xEFBE4786L, 0x0FC19DC6L, 0x240CA1CCL,
        0x2DE92C6FL, 0x4A7484AAL, 0x5CB0A9DCL, 0x76F988DAL, 0x983E5152L,
        0xA831C66DL, 0xB00327C8L, 0xBF597FC7L, 0xC6E00BF3L, 0xD5A79147L,
        0x06CA6351L, 0x14292967L, 0x27B70A85L, 0x2E1B2138L, 0x4D2C6DFCL,
        0x53380D13L, 0x650A7354L, 0x766A0ABBL, 0x81C2C92EL, 0x92722C85L,
        0xA2BFE8A1L, 0xA81A664BL, 0xC24B8B70L, 0xC76C51A3L, 0xD192E819L,
        0xD6990624L, 0xF40E3585L, 0x106AA070L, 0x19A4C116L, 0x1E376C08L,
        0x2748774CL, 0x34B0BCB5L, 0x391C0CB3L, 0x4ED8AA4AL, 0x5B9CCA4FL,
        0x682E6FF3L, 0x748F82EEL, 0x78A5636FL, 0x84C87814L, 0x8CC70208L,
        0x90BEFFFAL, 0xA4506CEBL, 0xBEF9A3F7L, 0xC67178F2L
    };


#define SHR(x, n) ((x & 0xFFFFFFFF) >> n)

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

uint32_t rotlFixed(uint32_t x, uint32_t y)
    {
        return (x << y) | (x >> (sizeof(y) * 8 - y));
    }

uint32_t ByteReverseWord32(uint32_t value){
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return rotlFixed(value, 16U);
}

void ByteReverseWords(uint32_t* out, const uint32_t* in, uint32_t byteCount)
{
    uint32_t count, i;
    count = byteCount/(uint32_t)sizeof(uint32_t);
    for (i = 0; i < count; i++)  out[i] = ByteReverseWord32(in[i]);
}


IRAM_ATTR void nerd_mids(uint32_t* digest, const uint8_t* dataIn)
{
    uint32_t A[8] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };

    uint32_t temp1, temp2, W[64];
    uint8_t i;

    W[0] = GET_UINT32_BE(dataIn, 0);
    W[1] = GET_UINT32_BE(dataIn, 4);
    W[2] = GET_UINT32_BE(dataIn, 8);
    W[3] = GET_UINT32_BE(dataIn, 12);
    W[4] = GET_UINT32_BE(dataIn, 16);
    W[5] = GET_UINT32_BE(dataIn, 20);
    W[6] = GET_UINT32_BE(dataIn, 24);
    W[7] = GET_UINT32_BE(dataIn, 28);
    W[8] = GET_UINT32_BE(dataIn, 32);
    W[9] = GET_UINT32_BE(dataIn, 36);
    W[10] = GET_UINT32_BE(dataIn, 40);
    W[11] = GET_UINT32_BE(dataIn, 44);
    W[12] = GET_UINT32_BE(dataIn, 48);
    W[13] = GET_UINT32_BE(dataIn, 52);
    W[14] = GET_UINT32_BE(dataIn, 56);
    W[15] = GET_UINT32_BE(dataIn, 60);
    
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

    digest[0] = 0x6A09E667 + A[0];
    digest[1] = 0xBB67AE85 + A[1];
    digest[2] = 0x3C6EF372 + A[2];
    digest[3] = 0xA54FF53A + A[3];
    digest[4] = 0x510E527F + A[4];
    digest[5] = 0x9B05688C + A[5];
    digest[6] = 0x1F83D9AB + A[6];
    digest[7] = 0x5BE0CD19 + A[7];
}

IRAM_ATTR bool nerd_sha256d(nerdSHA256_context* midstate, const uint8_t* dataIn, uint8_t* doubleHash)
{
    uint32_t temp1, temp2;
    uint8_t temp3, temp4;
    uint32_t* buffer32;
    //*********** Init 1rst SHA ***********

    uint32_t W[64] = {
#if 0
        __builtin_bswap32(((const uint32_t*)dataIn)[0]),
        __builtin_bswap32(((const uint32_t*)dataIn)[1]),
        __builtin_bswap32(((const uint32_t*)dataIn)[2]),
        __builtin_bswap32(((const uint32_t*)dataIn)[3]),
#else
        GET_UINT32_BE(dataIn, 0),
        GET_UINT32_BE(dataIn, 4),
        GET_UINT32_BE(dataIn, 8),
        GET_UINT32_BE(dataIn, 12),
#endif
        0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 640}; 

    uint32_t A[8] = { midstate->digest[0], midstate->digest[1], midstate->digest[2], midstate->digest[3],
        midstate->digest[4], midstate->digest[5], midstate->digest[6], midstate->digest[7] };

    union {
        uint32_t num;
        uint8_t b[4];
    } u;
    uint8_t* p = NULL;

    uint8_t i;

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
    
   //*********** end SHA_finish ***********

    /* Calculate the second hash (double SHA-256) */
 
    W[0] = A[0] + midstate->digest[0];
    W[1] = A[1] + midstate->digest[1];
    W[2] = A[2] + midstate->digest[2];
    W[3] = A[3] + midstate->digest[3];
    W[4] = A[4] + midstate->digest[4];
    W[5] = A[5] + midstate->digest[5];
    W[6] = A[6] + midstate->digest[6];
    W[7] = A[7] + midstate->digest[7];
    W[8] = 0x80000000;
    W[9] = 0;
    W[10] = 0;
    W[11] = 0;
    W[12] = 0;
    W[13] = 0;
    W[14] = 0;
    W[15] = 256;

  
    A[0] = 0x6A09E667;
    A[1] = 0xBB67AE85;
    A[2] = 0x3C6EF372;
    A[3] = 0xA54FF53A;
    A[4] = 0x510E527F;
    A[5] = 0x9B05688C;
    A[6] = 0x1F83D9AB;
    A[7] = 0x5BE0CD19;

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
    if ((uint32_t)(A[7] & 0xFFFF) != 0x32E7)
    {
        doubleHash[30] = 0xFF;
        return false;
    }
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(61), K[61]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(62), K[62]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(63), K[63]);

#if 1
    //Best performance
    PUT_UINT32_BE(0x6A09E667 + A[0], doubleHash, 0);
    PUT_UINT32_BE(0xBB67AE85 + A[1], doubleHash, 4);
    PUT_UINT32_BE(0x3C6EF372 + A[2], doubleHash, 8);
    PUT_UINT32_BE(0xA54FF53A + A[3], doubleHash, 12);
    PUT_UINT32_BE(0x510E527F + A[4], doubleHash, 16);
    PUT_UINT32_BE(0x9B05688C + A[5], doubleHash, 20);
    PUT_UINT32_BE(0x1F83D9AB + A[6], doubleHash, 24);
    PUT_UINT32_BE(0x5BE0CD19 + A[7], doubleHash, 28);
#endif

#if 0
    temp1 = 0x6A09E667 + A[0]; ((uint32_t*)doubleHash)[0] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0xBB67AE85 + A[1]; ((uint32_t*)doubleHash)[1] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x3C6EF372 + A[2]; ((uint32_t*)doubleHash)[2] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0xA54FF53A + A[3]; ((uint32_t*)doubleHash)[3] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x510E527F + A[4]; ((uint32_t*)doubleHash)[4] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x9B05688C + A[5]; ((uint32_t*)doubleHash)[5] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x1F83D9AB + A[6]; ((uint32_t*)doubleHash)[6] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x5BE0CD19 + A[7]; ((uint32_t*)doubleHash)[7] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
#endif

#if 0
    temp1 = 0x6A09E667 + A[0]; ((uint32_t*)doubleHash)[0] = __builtin_bswap32(temp1);
    temp1 = 0xBB67AE85 + A[1]; ((uint32_t*)doubleHash)[1] = __builtin_bswap32(temp1);
    temp1 = 0x3C6EF372 + A[2]; ((uint32_t*)doubleHash)[2] = __builtin_bswap32(temp1);
    temp1 = 0xA54FF53A + A[3]; ((uint32_t*)doubleHash)[3] = __builtin_bswap32(temp1);
    temp1 = 0x510E527F + A[4]; ((uint32_t*)doubleHash)[4] = __builtin_bswap32(temp1);
    temp1 = 0x9B05688C + A[5]; ((uint32_t*)doubleHash)[5] = __builtin_bswap32(temp1);
    temp1 = 0x1F83D9AB + A[6]; ((uint32_t*)doubleHash)[6] = __builtin_bswap32(temp1);
    temp1 = 0x5BE0CD19 + A[7]; ((uint32_t*)doubleHash)[7] = __builtin_bswap32(temp1);
#endif


#if 0
    ((uint32_t*)doubleHash)[0] = __builtin_bswap32( (0x6A09E667 + A[0]) );
    ((uint32_t*)doubleHash)[1] = __builtin_bswap32( (0xBB67AE85 + A[1]) );
    ((uint32_t*)doubleHash)[2] = __builtin_bswap32( (0x3C6EF372 + A[2]) );
    ((uint32_t*)doubleHash)[3] = __builtin_bswap32( (0xA54FF53A + A[3]) );
    ((uint32_t*)doubleHash)[4] = __builtin_bswap32( (0x510E527F + A[4]) );    
    ((uint32_t*)doubleHash)[5] = __builtin_bswap32( (0x9B05688C + A[5]) );
    ((uint32_t*)doubleHash)[6] = __builtin_bswap32( (0x1F83D9AB + A[6]) );
    ((uint32_t*)doubleHash)[7] = __builtin_bswap32( (0x5BE0CD19 + A[7]) );
#endif

    return true;
}


IRAM_ATTR void nerd_sha256_bake(const uint32_t* digest, const uint8_t* dataIn, uint32_t* bake)  //15 words
{
    bake[0] = GET_UINT32_BE(dataIn, 0);
    bake[1] = GET_UINT32_BE(dataIn, 4);
    bake[2] = GET_UINT32_BE(dataIn, 8);
    //w[3] = GET_UINT32_BE(dataIn, 12);

    bake[3] = S1(  0) + 0 + S0(bake[1]) + bake[0];
    bake[4] = S1(640) + 0 + S0(bake[2]) + bake[1];

    uint32_t* a = bake + 5;
    a[0] = digest[0];
    a[1] = digest[1];
    a[2] = digest[2];
    a[3] = digest[3];
    a[4] = digest[4];
    a[5] = digest[5];
    a[6] = digest[6];
    a[7] = digest[7];

    uint32_t temp1, temp2;
    P(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], bake[0], K[0]);
    P(a[7], a[0], a[1], a[2], a[3], a[4], a[5], a[6], bake[1], K[1]);
    P(a[6], a[7], a[0], a[1], a[2], a[3], a[4], a[5], bake[2], K[2]);

    //P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[3], K[3]);
    //P(a,    b,    c,    d,    e,    f,    g,    h,    x,    K)
    bake[13] = a[4] + S3(a[1]) + F1(a[1], a[2], a[3]) + K[3];// + x;
    bake[14] = S2(a[5]) + F0(a[5], a[6], a[7]);
}


IRAM_ATTR bool nerd_sha256d_baked(const uint32_t* digest, const uint8_t* dataIn, const uint32_t* bake, uint8_t* doubleHash)
{
    uint32_t temp1, temp2;
    //*********** Init 1rst SHA ***********

    //W0 W1 W2 is same !!
    uint32_t W[64] = { bake[0], bake[1], bake[2], GET_UINT32_BE(dataIn, 12),
                0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 640 };
    W[16] = bake[3];
    W[17] = bake[4];

    const uint32_t* a = bake + 5;
    uint32_t A[8] = { a[0], a[1], a[2], a[3],
                      a[4], a[5], a[6], a[7] };

    //P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[0], K[0]);
    //P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[1], K[1]);
    //P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[2], K[2]);

    //P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[3], K[3]);
    //P(a,    b,    c,    d,    e,    f,    g,    h,    x,    K)
    temp1 = bake[13] + W[3];
    temp2 = bake[14];
    A[0] += temp1;
    A[4] = temp1 + temp2;

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
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[16], K[16]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[17], K[17]);
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

    //*********** end SHA_finish ***********

     /* Calculate the second hash (double SHA-256) */

    W[0] = A[0] + digest[0];
    W[1] = A[1] + digest[1];
    W[2] = A[2] + digest[2];
    W[3] = A[3] + digest[3];
    W[4] = A[4] + digest[4];
    W[5] = A[5] + digest[5];
    W[6] = A[6] + digest[6];
    W[7] = A[7] + digest[7];
    W[8] = 0x80000000;
    W[9] = 0;
    W[10] = 0;
    W[11] = 0;
    W[12] = 0;
    W[13] = 0;
    W[14] = 0;
    W[15] = 256;


    A[0] = 0x6A09E667;
    A[1] = 0xBB67AE85;
    A[2] = 0x3C6EF372;
    A[3] = 0xA54FF53A;
    A[4] = 0x510E527F;
    A[5] = 0x9B05688C;
    A[6] = 0x1F83D9AB;
    A[7] = 0x5BE0CD19;

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
    
    //Unroll 56 - worse performace
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(56), K[56]);

    //Unroll 57
    //P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(57), K[57]);
    //P(a,    b,    c,    d,    e,    f,    g,    h,    x,     K)
    uint32_t m1 = A[6] + S3(A[3]) + F1(A[3], A[4], A[5]) + K[57] + R(57);
    //uint32_t m2 = S2(A[7]) + F0(A[7], A[0], A[1]);
    A[2] += m1;
    //A[6] = m1 + m2;
    uint32_t d57_a1 = A[1];

    //Unroll 58
    //P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(58), K[58]);
    //P(a,    b,    c,    d,    e,    f,    g,    h,    x,     K)
    uint32_t z1 = A[5] + S3(A[2]) + F1(A[2], A[3], A[4]) + K[58] + R(58);
    //uint32_t z2 = S2(A[6]) + F0(A[6], A[7], A[0]);
    uint32_t d58_a0 = A[0];
    A[1] += z1;
    //A[5] = z1 + z2;

    //Unroll 59
    //P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(59), K[59]);
    //P(a,    b,    c,    d,    e,    f,    g,    h,    x,     K)
    uint32_t t1 = A[4] + S3(A[1]) + F1(A[1], A[2], A[3]) + K[59] + R(59);
    //uint32_t t2 = S2(A[5]) + F0(A[5], A[6], A[7]);
    A[0] += t1;
    //A[4] = t1 + t2;

    //Unroll 60
    //P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(60), K[60]);
    //P(a,    b,    c,    d,    e,    f,    g,    h,    x,     K)
    temp1 = A[3] + S3(A[0]) + F1(A[0], A[1], A[2]) + K[60] + R(60);
    uint32_t a7 = A[7] + temp1;
    if ((uint32_t)(a7 & 0xFFFF) != 0x32E7)
        return false;

    //Post 57
    uint32_t m2 = S2(A[7]) + F0(A[7], d58_a0, d57_a1);
    A[6] = m1 + m2;

    //Post 58
    uint32_t z2 = S2(A[6]) + F0(A[6], A[7], d58_a0);
    A[5] = z1 + z2;

    //Post 59
    uint32_t t2 = S2(A[5]) + F0(A[5], A[6], A[7]);
    //uint32_t t2 = S2(A[5]) + F0(A[5], A[6], a7);
    A[4] = t1 + t2;

    //Post 60
    A[7] = a7;
    temp2 = S2(A[4]) + F0(A[4], A[5], A[6]);
    A[3] = temp1 + temp2;

    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(61), K[61]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(62), K[62]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(63), K[63]);

#if 1
    temp1 = 0x6A09E667 + A[0]; ((uint32_t*)doubleHash)[0] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0xBB67AE85 + A[1]; ((uint32_t*)doubleHash)[1] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x3C6EF372 + A[2]; ((uint32_t*)doubleHash)[2] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0xA54FF53A + A[3]; ((uint32_t*)doubleHash)[3] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x510E527F + A[4]; ((uint32_t*)doubleHash)[4] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x9B05688C + A[5]; ((uint32_t*)doubleHash)[5] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x1F83D9AB + A[6]; ((uint32_t*)doubleHash)[6] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
    temp1 = 0x5BE0CD19 + A[7]; ((uint32_t*)doubleHash)[7] = (temp1 << 24) | ((temp1 << 8) & 0x00FF0000) | ((temp1 >> 8) & 0x0000FF00) | (temp1 >> 24);
#else
    PUT_UINT32_BE(0x6A09E667 + A[0], doubleHash, 0);
    PUT_UINT32_BE(0xBB67AE85 + A[1], doubleHash, 4);
    PUT_UINT32_BE(0x3C6EF372 + A[2], doubleHash, 8);
    PUT_UINT32_BE(0xA54FF53A + A[3], doubleHash, 12);
    PUT_UINT32_BE(0x510E527F + A[4], doubleHash, 16);
    PUT_UINT32_BE(0x9B05688C + A[5], doubleHash, 20);
    PUT_UINT32_BE(0x1F83D9AB + A[6], doubleHash, 24);
    PUT_UINT32_BE(0x5BE0CD19 + A[7], doubleHash, 28);
#endif
    return true;
}
