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

#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h" // For mbedtls_platform_zeroize if needed

/*
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
*/

IRAM_ATTR void nerd_mids(nerdSHA256_context* midstate, uint8_t* dataIn)
{
    mbedtls_sha256_init(midstate);
    // Consider adding error checking for mbedtls_sha256_starts_ret and mbedtls_sha256_update_ret
    // For now, keeping it simple as per original structure that didn't return errors from nerd_mids
    if (mbedtls_sha256_starts_ret(midstate, 0) != 0) { // 0 for SHA-256
        // Handle error, maybe log it or assert, though original function didn't return status
        ESP_LOGE("nerd_mids", "mbedtls_sha256_starts_ret failed");
        return; // Or some other error indication if possible
    }
    if (mbedtls_sha256_update_ret(midstate, dataIn, 64) != 0) { // Process first 64 bytes
        // Handle error
        ESP_LOGE("nerd_mids", "mbedtls_sha256_update_ret failed");
        // midstate might be in an inconsistent state here
        return;
    }
}

IRAM_ATTR bool nerd_sha256d(nerdSHA256_context* midstate, uint8_t* dataIn, uint8_t* doubleHash)
{
    mbedtls_sha256_context temp_ctx;
    uint8_t first_hash_result[32];

    // First SHA256 round (continues from midstate)
    mbedtls_sha256_init(&temp_ctx);
    mbedtls_sha256_clone(&temp_ctx, midstate);

    if (mbedtls_sha256_update_ret(&temp_ctx, dataIn, 16) != 0) { // Process last 16 bytes of header
        ESP_LOGE("nerd_sha256d", "mbedtls_sha256_update_ret (1st hash) failed");
        mbedtls_sha256_free(&temp_ctx);
        return false;
    }
    if (mbedtls_sha256_finish_ret(&temp_ctx, first_hash_result) != 0) {
        ESP_LOGE("nerd_sha256d", "mbedtls_sha256_finish_ret (1st hash) failed");
        mbedtls_sha256_free(&temp_ctx);
        return false;
    }
    mbedtls_sha256_free(&temp_ctx); // Free after first hash

    // Second SHA256 round
    mbedtls_sha256_context second_hash_ctx; // Use a new context for the second round
    mbedtls_sha256_init(&second_hash_ctx);
    if (mbedtls_sha256_starts_ret(&second_hash_ctx, 0) != 0) { // 0 for SHA-256
        ESP_LOGE("nerd_sha256d", "mbedtls_sha256_starts_ret (2nd hash) failed");
        mbedtls_sha256_free(&second_hash_ctx);
        return false;
    }
    if (mbedtls_sha256_update_ret(&second_hash_ctx, first_hash_result, 32) != 0) {
        ESP_LOGE("nerd_sha256d", "mbedtls_sha256_update_ret (2nd hash) failed");
        mbedtls_sha256_free(&second_hash_ctx);
        return false;
    }
    if (mbedtls_sha256_finish_ret(&second_hash_ctx, doubleHash) != 0) {
        ESP_LOGE("nerd_sha256d", "mbedtls_sha256_finish_ret (2nd hash) failed");
        mbedtls_sha256_free(&second_hash_ctx);
        return false;
    }
    mbedtls_sha256_free(&second_hash_ctx);

    // Maintain original optimization check if needed
    if (doubleHash[31] != 0 || doubleHash[30] != 0) {
        // This check was originally an early exit.
        // Depending on how this function is used, this might still be a valid optimization
        // to signal that this hash result won't meet difficulty criteria.
        return false;
    }

    return true; // Success
}