#ifndef UTILS_API_H
#define UTILS_API_H

#include <stddef.h>
#include <stdint.h>
#include "mining.h"
#include "stratum.h"

/*
 * General byte order swapping functions.
 */
#define	bswap16(x)	__bswap16(x)
#define	bswap32(x)	__bswap32(x)
#define	bswap64(x)	__bswap64(x)



uint8_t hex(char ch);

int to_byte_array(const char *in, size_t in_size, uint8_t *out);
double le256todouble(const void *target);
double diff_from_target(void *target);
miner_data calculateMiningData(mining_subscribe& mWorker, mining_job mJob);
bool checkValid(unsigned char* hash, unsigned char* target);
void suffix_string(double val, char *buf, size_t bufsiz, int sigdigits);



#endif // UTILS_API_H