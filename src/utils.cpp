
#include <Arduino.h>
#include "utils.h"
#include "mining.h"
#include "stratum.h"
#include "mbedtls/sha256.h"

#include <string.h>
#include <stdio.h>

#ifndef bswap_16
#define bswap_16(a) ((((uint16_t) (a) << 8) & 0xff00) | (((uint16_t) (a) >> 8) & 0xff))
#endif

#ifndef bswap_32
#define bswap_32(a) ((((uint32_t) (a) << 24) & 0xff000000) | \
		     (((uint32_t) (a) << 8) & 0xff0000) | \
     		     (((uint32_t) (a) >> 8) & 0xff00) | \
     		     (((uint32_t) (a) >> 24) & 0xff))
#endif

uint32_t swab32(uint32_t v) {
    return bswap_32(v);
}

uint8_t hex(char ch) {
    uint8_t r = (ch > 57) ? (ch - 55) : (ch - 48);
    return r & 0x0F;
}

int to_byte_array(const char *in, size_t in_size, uint8_t *out) {
    int count = 0;
    if (in_size % 2) {
        while (*in && out) {
            *out = hex(*in++);
            if (!*in)
                return count;
            *out = (*out << 4) | hex(*in++);
            *out++;
            count++;
        }
        return count;
    } else {
        while (*in && out) {
            *out++ = (hex(*in++) << 4) | hex(*in++);
            count++;
        }
        return count;
    }
}

void swap_endian_words(const char * hex_words, uint8_t * output) {
    size_t hex_length = strlen(hex_words);
    if (hex_length % 8 != 0) {
        fprintf(stderr, "Must be 4-byte word aligned\n");
        exit(EXIT_FAILURE);
    }

    size_t binary_length = hex_length / 2;

    for (size_t i = 0; i < binary_length; i += 4) {
        for (int j = 0; j < 4; j++) {
            unsigned int byte_val;
            sscanf(hex_words + (i + j) * 2, "%2x", &byte_val);
            output[i + (3 - j)] = byte_val;
        }
    }
}

void reverse_bytes(uint8_t * data, size_t len) {
    for (int i = 0; i < len / 2; ++i) {
        uint8_t temp = data[i];
        data[i] = data[len - 1 - i];
        data[len - 1 - i] = temp;
    }
}

static const double truediffone = 26959535291011309493156476344723991336010898738574164086137773096960.0;
/* Converts a little endian 256 bit value to a double */
double le256todouble(const void *target)
{
	uint64_t *data64;
	double dcut64;

	data64 = (uint64_t *)(target + 24);
	dcut64 = *data64 * 6277101735386680763835789423207666416102355444464034512896.0;

	data64 = (uint64_t *)(target + 16);
	dcut64 += *data64 * 340282366920938463463374607431768211456.0;

	data64 = (uint64_t *)(target + 8);
	dcut64 += *data64 * 18446744073709551616.0;

	data64 = (uint64_t *)(target);
	dcut64 += *data64;

	return dcut64;
}

double diff_from_target(void *target)
{
	double d64, dcut64;

	d64 = truediffone;
	dcut64 = le256todouble(target);
	if (unlikely(!dcut64))
		dcut64 = 1;
	return d64 / dcut64;
}

/****************** PREMINING CALCULATIONS ********************/


bool checkValid(unsigned char* hash, unsigned char* target) {
  bool valid = true;
  unsigned char diff_target[32];
  memcpy(diff_target, &target, 32);
  //convert target to little endian for comparison
  reverse_bytes(diff_target, 32);

  for(uint8_t i=31; i>=0; i--) {
    if(hash[i] > diff_target[i]) {
      valid = false;
      break;
    }
  }

  #ifdef DEBUG_MINING
  if (valid) {
    Serial.print("\tvalid : ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x ", hash[i]);
    Serial.println();
  }
  #endif
  return valid;
}

void getNextExtranonce2(int extranonce2_size, char *extranonce2) {
  
  unsigned long extranonce2_number = strtoul(extranonce2, NULL, 10);
  extranonce2_number++;
  
  memset(extranonce2, '0', 2 * extranonce2_size);
  if (extranonce2_number > long(pow(10, 2 * extranonce2_size))) {
    return;
  }
  
  char next_extranounce2[2 * extranonce2_size + 1];
  memset(extranonce2, '0', 2 * extranonce2_size);
  ultoa(extranonce2_number, next_extranounce2, 10);
  memcpy(extranonce2 + (2 * extranonce2_size) - long(log10(extranonce2_number)) - 1 , next_extranounce2, strlen(next_extranounce2));
  extranonce2[2 * extranonce2_size] = 0;
}

miner_data init_miner_data(void){
  
  miner_data newMinerData;

  newMinerData.poolDifficulty = DEFAULT_DIFFICULTY;
  newMinerData.inRun = false;
  newMinerData.newJob = false;
  
  return newMinerData;
}

miner_data calculateMiningData(mining_subscribe& mWorker, mining_job mJob){

  miner_data mMiner = init_miner_data();

  // calculate target - target = (nbits[2:]+'00'*(int(nbits[:2],16) - 3)).zfill(64)
    
    char target[TARGET_BUFFER_SIZE+1];
    memset(target, '0', TARGET_BUFFER_SIZE);
    int zeros = (int) strtol(mJob.nbits.substring(0, 2).c_str(), 0, 16) - 3;
    memcpy(target + zeros - 2, mJob.nbits.substring(2).c_str(), mJob.nbits.length() - 2);
    target[TARGET_BUFFER_SIZE] = 0;
    Serial.print("    target: "); Serial.println(target);
    
    // bytearray target
    size_t size_target = to_byte_array(target, 32, mMiner.bytearray_target);

    for (size_t j = 0; j < 8; j++) {
      mMiner.bytearray_target[j] ^= mMiner.bytearray_target[size_target - 1 - j];
      mMiner.bytearray_target[size_target - 1 - j] ^= mMiner.bytearray_target[j];
      mMiner.bytearray_target[j] ^= mMiner.bytearray_target[size_target - 1 - j];
    }

    // get extranonce2 - extranonce2 = hex(random.randint(0,2**32-1))[2:].zfill(2*extranonce2_size)
    //To review
    char extranonce2_char[2 * mWorker.extranonce2_size+1];	
	  mWorker.extranonce2.toCharArray(extranonce2_char, 2 * mWorker.extranonce2_size + 1);
    getNextExtranonce2(mWorker.extranonce2_size, extranonce2_char);
    mWorker.extranonce2 = String(extranonce2_char);
    //mWorker.extranonce2 = "00000002";
    
    //get coinbase - coinbase_hash_bin = hashlib.sha256(hashlib.sha256(binascii.unhexlify(coinbase)).digest()).digest()
    String coinbase = mJob.coinb1 + mWorker.extranonce1 + mWorker.extranonce2 + mJob.coinb2;
    Serial.print("    coinbase: "); Serial.println(coinbase);
    size_t str_len = coinbase.length()/2;
    uint8_t bytearray[str_len];

    size_t res = to_byte_array(coinbase.c_str(), str_len*2, bytearray);

    #ifdef DEBUG_MINING
    Serial.print("    extranonce2: "); Serial.println(mWorker.extranonce2);
    Serial.print("    coinbase: "); Serial.println(coinbase);
    Serial.print("    coinbase bytes - size: "); Serial.println(res);
    for (size_t i = 0; i < res; i++)
        Serial.printf("%02x", bytearray[i]);
    Serial.println("---");
    #endif

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
  
    byte interResult[32]; // 256 bit
    byte shaResult[32]; // 256 bit
  
    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, bytearray, str_len);
    mbedtls_sha256_finish_ret(&ctx, interResult);

    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, interResult, 32);
    mbedtls_sha256_finish_ret(&ctx, shaResult);
    mbedtls_sha256_free(&ctx);

    #ifdef DEBUG_MINING
    Serial.print("    coinbase double sha: ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x", shaResult[i]);
    Serial.println("");
    #endif

    
    // copy coinbase hash
    memcpy(mMiner.merkle_result, shaResult, sizeof(shaResult));
    
    byte merkle_concatenated[32 * 2];
    for (size_t k=0; k < mJob.merkle_branch.size(); k++) {
        const char* merkle_element = (const char*) mJob.merkle_branch[k];
        uint8_t bytearray[32];
        size_t res = to_byte_array(merkle_element, 64, bytearray);

        #ifdef DEBUG_MINING
        Serial.print("    merkle element    "); Serial.print(k); Serial.print(": "); Serial.println(merkle_element);
        #endif
        for (size_t i = 0; i < 32; i++) {
          merkle_concatenated[i] = mMiner.merkle_result[i];
          merkle_concatenated[32 + i] = bytearray[i];
        }

        #ifdef DEBUG_MINING
        Serial.print("    merkle element    "); Serial.print(k); Serial.print(": "); Serial.println(merkle_element);
        Serial.print("    merkle concatenated: ");
        for (size_t i = 0; i < 64; i++)
            Serial.printf("%02x", merkle_concatenated[i]);
        Serial.println("");
        #endif

        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts_ret(&ctx,0);
        mbedtls_sha256_update_ret(&ctx, merkle_concatenated, 64);
        mbedtls_sha256_finish_ret(&ctx, interResult);

        mbedtls_sha256_starts_ret(&ctx,0);
        mbedtls_sha256_update_ret(&ctx, interResult, 32);
        mbedtls_sha256_finish_ret(&ctx, mMiner.merkle_result);
        mbedtls_sha256_free(&ctx);

        #ifdef DEBUG_MINING
        Serial.print("    merkle sha         : ");
        for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", mMiner.merkle_result[i]);
        Serial.println("");
        #endif
    }
    // merkle root from merkle_result
    
    Serial.print("    merkle sha         : ");
    char merkle_root[65];
    for (int i = 0; i < 32; i++) {
      Serial.printf("%02x", mMiner.merkle_result[i]);
      snprintf(&merkle_root[i*2], 3, "%02x", mMiner.merkle_result[i]);
    }
    merkle_root[65] = 0;
    Serial.println("");

    // calculate blockheader
    // j.block_header = ''.join([j.version, j.prevhash, merkle_root, j.ntime, j.nbits])
    String blockheader = mJob.version + mJob.prev_block_hash + String(merkle_root) + mJob.ntime + mJob.nbits + "00000000";
    str_len = blockheader.length()/2;
    
    //uint8_t bytearray_blockheader[str_len];
    res = to_byte_array(blockheader.c_str(), str_len*2, mMiner.bytearray_blockheader);

    #ifdef DEBUG_MINING
    Serial.println("    blockheader: "); Serial.print(blockheader);
    Serial.println("    blockheader bytes "); Serial.print(str_len); Serial.print(" -> ");
    #endif

    // reverse version
    uint8_t buff;
    size_t bword, bsize, boffset;
    boffset = 0;
    bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = mMiner.bytearray_blockheader[j];
        mMiner.bytearray_blockheader[j] = mMiner.bytearray_blockheader[2 * boffset + bsize - 1 - j];
        mMiner.bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }

    // reverse prev hash (4-byte word swap)
    boffset = 4;
    bword = 4;
    bsize = 32;
    for (size_t i = 1; i <= bsize / bword; i++) {
        for (size_t j = boffset; j < boffset + bword / 2; j++) {
            buff = mMiner.bytearray_blockheader[j];
            mMiner.bytearray_blockheader[j] = mMiner.bytearray_blockheader[2 * boffset + bword - 1 - j];
            mMiner.bytearray_blockheader[2 * boffset + bword - 1 - j] = buff;
        }
        boffset += bword;
    }

/*
    // reverse merkle (4-byte word swap)
    boffset = 36;
    bword = 4;
    bsize = 32;
    for (size_t i = 1; i <= bsize / bword; i++) {
        for (size_t j = boffset; j < boffset + bword / 2; j++) {
            buff = mMiner.bytearray_blockheader[j];
            mMiner.bytearray_blockheader[j] = mMiner.bytearray_blockheader[2 * boffset + bword - 1 - j];
            mMiner.bytearray_blockheader[2 * boffset + bword - 1 - j] = buff;
        }
        boffset += bword;
    }
*/
    // reverse ntime
    boffset = 68;
    bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = mMiner.bytearray_blockheader[j];
        mMiner.bytearray_blockheader[j] = mMiner.bytearray_blockheader[2 * boffset + bsize - 1 - j];
        mMiner.bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }

    // reverse difficulty
    boffset = 72;
    bsize = 4;
    for (size_t j = boffset; j < boffset + (bsize/2); j++) {
        buff = mMiner.bytearray_blockheader[j];
        mMiner.bytearray_blockheader[j] = mMiner.bytearray_blockheader[2 * boffset + bsize - 1 - j];
        mMiner.bytearray_blockheader[2 * boffset + bsize - 1 - j] = buff;
    }


    #ifdef DEBUG_MINING
    Serial.print(" >>> bytearray_blockheader     : "); 
    for (size_t i = 0; i < 4; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("version     ");
    for (size_t i = 0; i < 4; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("prev hash   ");
    for (size_t i = 4; i < 4+32; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("merkle root ");
    for (size_t i = 36; i < 36+32; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("ntime       ");
    for (size_t i = 68; i < 68+4; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("nbits       ");
    for (size_t i = 72; i < 72+4; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.print("nonce       ");
    for (size_t i = 76; i < 76+4; i++)
        Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    Serial.println("");
    Serial.println("bytearray_blockheader: ");
    for (size_t i = 0; i < str_len; i++) {
      Serial.printf("%02x", mMiner.bytearray_blockheader[i]);
    }
    Serial.println("");
    #endif
  return mMiner;
}

/* Convert a double value into a truncated string for displaying with its
 * associated suitable for Mega, Giga etc. Buf array needs to be long enough */
void suffix_string(double val, char *buf, size_t bufsiz, int sigdigits)
{
	const double kilo = 1000;
	const double mega = 1000000;
	const double giga = 1000000000;
	const double tera = 1000000000000;
	const double peta = 1000000000000000;
	const double exa  = 1000000000000000000;
	// minimum diff value to display
	const double min_diff = 0.001;
    const byte maxNdigits = 2;
	char suffix[2] = "";
	bool decimal = true;
	double dval;

	if (val >= exa) {
		val /= peta;
		dval = val / kilo;
		strcpy(suffix, "E");
	} else if (val >= peta) {
		val /= tera;
		dval = val / kilo;
		strcpy(suffix, "P");
	} else if (val >= tera) {
		val /= giga;
		dval = val / kilo;
		strcpy(suffix, "T");
	} else if (val >= giga) {
		val /= mega;
		dval = val / kilo;
		strcpy(suffix, "G");
	} else if (val >= mega) {
		val /= kilo;
		dval = val / kilo;
		strcpy(suffix, "M");
	} else if (val >= kilo) {
		dval = val / kilo;
		strcpy(suffix, "K");
	} else {
		dval = val;
		if (dval < min_diff)
			dval = 0.0;
	}

	if (!sigdigits) {
		if (decimal)
			snprintf(buf, bufsiz, "%.3f%s", dval, suffix);
		else
			snprintf(buf, bufsiz, "%d%s", (unsigned int)dval, suffix);
	} else {
		/* Always show sigdigits + 1, padded on right with zeroes
		 * followed by suffix */
		int ndigits = sigdigits - 1 - (dval > 0.0 ? floor(log10(dval)) : 0);

		snprintf(buf, bufsiz, "%*.*f%s", sigdigits + 1, ndigits, dval, suffix);
	}
}