/************************************************************************************
*   written by: Bitmaker
*   based on: Blockstream Jade shaLib
*   thanks to @LarryBitcoin

*   Description:

*   NerdSha256plus is a custom C implementation of sha256d based on Blockstream Jade 
    code https://github.com/Blockstream/Jade

    The folowing file can be used on any ESP32 implementation using both cores

*************************************************************************************/
#ifndef nerdSHA256plus_H_
#define nerdSHA256plus_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


struct nerdSHA256_context {
    uint8_t buffer[64];
    uint32_t digest[8];
};

/* Calculate midstate */
IRAM_ATTR void nerd_mids(nerdSHA256_context* midstate, uint8_t* dataIn);

IRAM_ATTR bool nerd_sha256d(nerdSHA256_context* midstate, uint8_t* dataIn, uint8_t* doubleHash);

void ByteReverseWords(uint32_t* out, const uint32_t* in, uint32_t byteCount);

#endif /* nerdSHA256plus_H_ */