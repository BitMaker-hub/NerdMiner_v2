#ifndef jadeSHA256_H_
#define jadeSHA256_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct _sha256_context {
    uint8_t buffer[64];
    uint32_t state[8];
} _sha256_context;

/* Calculate midstate */
IRAM_ATTR void calc_midstate(uint8_t* buf_ptr, _sha256_context* midstate);

IRAM_ATTR bool make_double_sha(_sha256_context* midstate);

/* We need a way to tell the miner to us that there is a solution */
typedef void (*solution_cb)(void* ctx, const uint8_t*, uint32_t);


#endif /* jadeSHA256_H_ */