#define NDEBUG
#include <stdio.h>
#include <string.h>
#include <Arduino.h>

//#include <wolfssl/wolfcrypt/sha256.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "nerdSHA256.h"
#include <math.h>
#include <string.h>

#define HASH_SIZE 32

//------------- JADE
#define SHR(x, n) ((x & 0xFFFFFFFF) >> n)

#define ROTR(x, n) ((x >> n) | (x << ((sizeof(x) << 3) - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define RJ(t) (W[t] = S1(W[t - 2]) + W[t - 7] + S0(W[t - 15]) + W[t - 16])

#define P(a, b, c, d, e, f, g, h, x, K)                                                                                \
    {                                                                                                                  \
        temp1 = h + S3(e) + F1(e, f, g) + K + x;                                                                       \
        temp2 = S2(a) + F0(a, b, c);                                                                                   \
        d += temp1;                                                                                                    \
        h = temp1 + temp2;                                                                                             \
    }

//--------------

IRAM_ATTR static inline uint32_t rotlFixed(uint32_t x, uint32_t y)
    {
        return (x << y) | (x >> (sizeof(y) * 8 - y));
    }
IRAM_ATTR static inline uint32_t rotrFixed(uint32_t x, uint32_t y)
    {
        return (x >> y) | (x << (sizeof(y) * 8 - y));
    }
/* SHA256 math based on specification */
#define Ch(x,y,z)       ((z) ^ ((x) & ((y) ^ (z))))
#define Maj(x,y,z)      ((((x) | (y)) & (z)) | ((x) & (y)))

#define R(x, n)         (((x) & 0xFFFFFFFFU) >> (n))

#define S(x, n)         rotrFixed(x, n)
#define Sigma0(x)       (S(x, 2)  ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6)  ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7)  ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))

#define a(i) S[(0-(i)) & 7]
#define b(i) S[(1-(i)) & 7]
#define c(i) S[(2-(i)) & 7]
#define d(i) S[(3-(i)) & 7]
#define e(i) S[(4-(i)) & 7]
#define f(i) S[(5-(i)) & 7]
#define g(i) S[(6-(i)) & 7]
#define h(i) S[(7-(i)) & 7]

#define XTRANSFORM(S, D)         Transform_Sha256((S),(D))
#define XMEMCPY(d,s,l)   memcpy((d),(s),(l))
#define XMEMSET(b,c,l)   memset((b),(c),(l))

/* SHA256 version that keeps all data in registers */
#define SCHED1(j) (W[j] = *((uint32_t*)&data[j*sizeof(uint32_t)]))
#define SCHED(j) (               \
                W[ j     & 15] += \
        Gamma1(W[(j-2)  & 15])+  \
                W[(j-7)  & 15] +  \
        Gamma0(W[(j-15) & 15])   \
    )

#define RND1(j) \
      t0 = h(j) + Sigma1(e(j)) + Ch(e(j), f(j), g(j)) + K[j] + SCHED1(j); \
      t1 = Sigma0(a(j)) + Maj(a(j), b(j), c(j)); \
      d(j) += t0; \
      h(j)  = t0 + t1
#define RND(j) \
      t0 = h(j) + Sigma1(e(j)) + Ch(e(j), f(j), g(j)) + K[j] + SCHED(j); \
      t1 = Sigma0(a(j)) + Maj(a(j), b(j), c(j)); \
      d(j) += t0; \
      h(j)  = t0 + t1
#define RNDN(j) \
      t0 = h(j) + Sigma1(e(j)) + Ch(e(j), f(j), g(j)) + K[i+j] + SCHED(j); \
      t1 = Sigma0(a(j)) + Maj(a(j), b(j), c(j)); \
      d(j) += t0; \
      h(j)  = t0 + t1

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

IRAM_ATTR inline static int Transform_Sha256(nerd_sha256* sha256, const uint8_t* data)
{
    uint32_t S[8], t0, t1;
    int i;
    IRAM_DATA_ATTR uint32_t W[NERD_BLOCK_SIZE/sizeof(uint32_t)];

    /* Copy digest to working vars */
    S[0] = sha256->digest[0];
    S[1] = sha256->digest[1];
    S[2] = sha256->digest[2];
    S[3] = sha256->digest[3];
    S[4] = sha256->digest[4];
    S[5] = sha256->digest[5];
    S[6] = sha256->digest[6];
    S[7] = sha256->digest[7];

    i = 0;
    RND1( 0); RND1( 1); RND1( 2); RND1( 3);
    RND1( 4); RND1( 5); RND1( 6); RND1( 7);
    RND1( 8); RND1( 9); RND1(10); RND1(11);
    RND1(12); RND1(13); RND1(14); RND1(15);
    for (i = 16; i < 64; i += 16) {
        RNDN( 0); RNDN( 1); RNDN( 2); RNDN( 3);
        RNDN( 4); RNDN( 5); RNDN( 6); RNDN( 7);
        RNDN( 8); RNDN( 9); RNDN(10); RNDN(11);
        RNDN(12); RNDN(13); RNDN(14); RNDN(15);
    }

    /* Add the working vars back into digest */
    sha256->digest[0] += S[0];
    sha256->digest[1] += S[1];
    sha256->digest[2] += S[2];
    sha256->digest[3] += S[3];
    sha256->digest[4] += S[4];
    sha256->digest[5] += S[5];
    sha256->digest[6] += S[6];
    sha256->digest[7] += S[7];

    return 0;
}

IRAM_ATTR static uint32_t ByteReverseWord32(uint32_t value){
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return rotlFixed(value, 16U);
}

IRAM_ATTR static void ByteReverseWords(uint32_t* out, const uint32_t* in, uint32_t byteCount)
{
    uint32_t count, i;
    count = byteCount/(uint32_t)sizeof(uint32_t);
    for (i = 0; i < count; i++)  out[i] = ByteReverseWord32(in[i]);
}


static int nerd_update(nerd_sha256* sha256, uint8_t* data, uint32_t len)
{
    int ret = 0;
    uint32_t blocksLen;
    uint8_t* local;

    //ShaUpdate
    uint32_t tmp = sha256->loLen;
    if ((sha256->loLen += len) < tmp) {
        sha256->hiLen++;                       /* carry low to high */
    }

    local = (uint8_t*)sha256->buffer;

    /* process any remainder from previous operation */
    if (sha256->buffLen > 0) {
        blocksLen = min(len, NERD_BLOCK_SIZE - sha256->buffLen);
        XMEMCPY(&local[sha256->buffLen], data, blocksLen);

        sha256->buffLen += blocksLen;
        data            += blocksLen;
        len             -= blocksLen;

        if (sha256->buffLen == NERD_BLOCK_SIZE) {
        
            ByteReverseWords(sha256->buffer, sha256->buffer, NERD_BLOCK_SIZE);

            ret = XTRANSFORM(sha256, (const uint8_t*)local);

            if (ret == 0)
                sha256->buffLen = 0;
            else
                len = 0; /* error */
        }
    }

    /* process blocks */
    while (len >= NERD_BLOCK_SIZE) {
        uint32_t* local32 = sha256->buffer;
        XMEMCPY(local32, data, NERD_BLOCK_SIZE);

        data += NERD_BLOCK_SIZE;
        len  -= NERD_BLOCK_SIZE;

        ByteReverseWords(local32, local32, NERD_BLOCK_SIZE);
    
        ret = XTRANSFORM(sha256, (const uint8_t*)local32);

        if (ret != 0)
            break;
    }
    /* save remainder */
    if (ret == 0 && len > 0) {
        XMEMCPY(local, data, len);
        sha256->buffLen = len;
    }

    return ret;
}


int nerd_midstate(nerd_sha256* sha256, uint8_t* data, uint32_t len)
{
    int ret = 0;
    uint32_t blocksLen;
    uint8_t* local;

    //Init SHA context 
    XMEMSET(sha256->digest, 0, sizeof(sha256->digest));
    sha256->digest[0] = 0x6A09E667L;
    sha256->digest[1] = 0xBB67AE85L;
    sha256->digest[2] = 0x3C6EF372L;
    sha256->digest[3] = 0xA54FF53AL;
    sha256->digest[4] = 0x510E527FL;
    sha256->digest[5] = 0x9B05688CL;
    sha256->digest[6] = 0x1F83D9ABL;
    sha256->digest[7] = 0x5BE0CD19L;

    sha256->buffLen = 0;
    sha256->loLen   = 0;
    sha256->hiLen   = 0;
    //endINIT Sha contexxt

    nerd_update(sha256,data,len);
    
    return 0;
}


IRAM_ATTR int nerd_double_sha2(nerd_sha256* midstate, uint8_t* dataIn, uint8_t* doubleHash)
{
    uint32_t S[8], t0, t1;
    uint32_t W[16];
    int i;
    uint8_t* data;

    //*********** Init 1rst SHA ***********
    uint8_t data1rstSHA[64] = { dataIn[3],dataIn[2],dataIn[1],dataIn[0],dataIn[7],dataIn[6],dataIn[5],dataIn[4],
                       dataIn[11],dataIn[10],dataIn[9],dataIn[8],dataIn[15],dataIn[14],dataIn[13],dataIn[12],
                       0,0,0,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x80,0x02,0,0};

    data = data1rstSHA;
    // Copy digest to working vars 
    S[0] = midstate->digest[0];
    S[1] = midstate->digest[1];
    S[2] = midstate->digest[2];
    S[3] = midstate->digest[3];
    S[4] = midstate->digest[4];
    S[5] = midstate->digest[5];
    S[6] = midstate->digest[6];
    S[7] = midstate->digest[7];

    RND1( 0); RND1( 1); RND1( 2); RND1( 3);
    RND1( 4); RND1( 5); RND1( 6); RND1( 7);
    RND1( 8); RND1( 9); RND1(10); RND1(11);
    RND1(12); RND1(13); RND1(14); RND1(15);
    // 64 operations, partially loop unrolled 
    for (i = 16; i < 64; i += 16) {
        RNDN( 0); RNDN( 1); RNDN( 2); RNDN( 3);
        RNDN( 4); RNDN( 5); RNDN( 6); RNDN( 7);
        RNDN( 8); RNDN( 9); RNDN(10); RNDN(11);
        RNDN(12); RNDN(13); RNDN(14); RNDN(15);
    }

    // Add the working vars back into digest 
    S[0] += midstate->digest[0];
    S[1] += midstate->digest[1];
    S[2] += midstate->digest[2];
    S[3] += midstate->digest[3];
    S[4] += midstate->digest[4];
    S[5] += midstate->digest[5];
    S[6] += midstate->digest[6];
    S[7] += midstate->digest[7];
    //*********** end SHA_finish ***********

    // ----- 2nd SHA ------------
    uint32_t data2nSha[64] = {S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],
                              0x80000000,0,0,0,0,0,0,256};    

    data = (uint8_t*)data2nSha;

    S[0] = 0x6A09E667L;
    S[1] = 0xBB67AE85L;
    S[2] = 0x3C6EF372L;
    S[3] = 0xA54FF53AL;
    S[4] = 0x510E527FL;
    S[5] = 0x9B05688CL;
    S[6] = 0x1F83D9ABL;
    S[7] = 0x5BE0CD19L;

    RND1( 0); RND1( 1); RND1( 2); RND1( 3);
    RND1( 4); RND1( 5); RND1( 6); RND1( 7);
    RND1( 8); RND1( 9); RND1(10); RND1(11);
    RND1(12); RND1(13); RND1(14); RND1(15);
    // 64 operations, partially loop unrolled 
    for (i = 16; i < 64; i += 16) {
        RNDN( 0); RNDN( 1); RNDN( 2); RNDN( 3);
        RNDN( 4); RNDN( 5); RNDN( 6); RNDN( 7);
        RNDN( 8); RNDN( 9); RNDN(10); RNDN(11);
        RNDN(12); RNDN(13); RNDN(14); RNDN(15);
    }

    // Add the working vars back into digest 
    S[0] += 0x6A09E667L;
    S[1] += 0xBB67AE85L;
    S[2] += 0x3C6EF372L;
    S[3] += 0xA54FF53AL;
    S[4] += 0x510E527FL;
    S[5] += 0x9B05688CL;
    S[6] += 0x1F83D9ABL;
    S[7] += 0x5BE0CD19L;

    ByteReverseWords((uint32_t*)doubleHash, S, NERD_DIGEST_SIZE);
    
    return 0;
}



IRAM_ATTR int nerd_double_sha(nerd_sha256* midstate, uint8_t* data, uint8_t* doubleHash)
{
    IRAM_DATA_ATTR nerd_sha256 sha256;
    //nerd_sha256 sha256_2;
    int ret = 0;
    uint32_t blocksLen;
    uint8_t* local;

    //Copy current context
    sha256 = *midstate;

    // ----- 1rst SHA ------------
    //*********** ShaUpdate ***********
    local = (uint8_t*)sha256.buffer;
    XMEMCPY(local, data, 16); //Pending bytes to make the sha256
    //*********** end update ***********

    //*********** Init SHA_finish ***********

    local[16] = 0x80; // add 1 
    //ADD final zeros
    XMEMSET(&local[17], 0, 39); //NERD_PAD_SIZE - sha256.buffLen);

    // put lengths in bits 
    sha256.hiLen = 0;
    sha256.loLen = 640;

    ByteReverseWords(sha256.buffer, sha256.buffer, NERD_BLOCK_SIZE);

    // ! length ordering dependent on digest endian type ! 
    XMEMCPY(&local[NERD_PAD_SIZE], &sha256.hiLen, sizeof(uint32_t));
    XMEMCPY(&local[NERD_PAD_SIZE + sizeof(uint32_t)], &sha256.loLen, sizeof(uint32_t));

    XTRANSFORM(&sha256, (const uint8_t*)local);

    //*********** end SHA_finish ***********

    // ----- 2nd SHA ------------
    //Init SHA context again
    IRAM_DATA_ATTR nerd_sha256 secondSha256 = {
        // Init with initial sha data
        .digest = {0x6A09E667L, 0xBB67AE85L, 0x3C6EF372L, 0xA54FF53AL, 0x510E527FL, 0x9B05688CL, 0x1F83D9ABL, 0x5BE0CD19L},    
        // Init with past SHA done
        .buffer = {sha256.digest[0],sha256.digest[1],sha256.digest[2],sha256.digest[3],sha256.digest[4],sha256.digest[5],sha256.digest[6],sha256.digest[7],
                   0x80000000,0,0,0,0,0,0,0},    // Init with past hash and 0x80
        .buffLen = 32,     // Bytes to hash
        .loLen = 256,      // Init to 256 bits
        .hiLen = 0,       // Inicializar a cero
        .heap = NULL      // Inicializar a NULL
    };
    
    local = (uint8_t*)secondSha256.buffer;

    // ! length ordering dependent on digest endian type ! 
    XMEMCPY(&local[NERD_PAD_SIZE + sizeof(uint32_t)], &secondSha256.loLen, sizeof(uint32_t));

    XTRANSFORM(&secondSha256, (const uint8_t*)local);

    ByteReverseWords((uint32_t*)doubleHash, secondSha256.digest, NERD_DIGEST_SIZE);
    
    return 0;
}