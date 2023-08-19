#ifndef nerdSHA256_H_
#define nerdSHA256_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NERD_DIGEST_SIZE 32
#define NERD_BLOCK_SIZE  64
#define NERD_PAD_SIZE    56

struct nerd_sha256 {
    uint32_t  digest[NERD_DIGEST_SIZE / sizeof(uint32_t)];
    uint32_t  buffer[NERD_BLOCK_SIZE  / sizeof(uint32_t)];
    uint32_t  buffLen;   /* in bytes          */
    uint32_t  loLen;     /* length in bytes   */
    uint32_t  hiLen;     /* length in bytes   */
    void*   heap;
};

/* Calculate midstate */
IRAM_ATTR int nerd_midstate(nerd_sha256* sha256, uint8_t* data, uint32_t len);

//IRAM_ATTR int nerd_double_sha(nerd_sha256* midstate, uint8_t* data, uint8_t* doubleHash);

IRAM_ATTR int nerd_double_sha2(nerd_sha256* midstate, uint8_t* dataIn, uint8_t* doubleHash);

#endif /* nerdSHA256_H_ */