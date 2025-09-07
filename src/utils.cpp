
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

	const uint64_t *data64;
	double dcut64;

	data64 = (const uint64_t *)((const uint8_t*)target + 24);
	dcut64 = *data64 * 6277101735386680763835789423207666416102355444464034512896.0;

	data64 = (const uint64_t *)((const uint8_t*)target + 16);
	dcut64 += *data64 * 340282366920938463463374607431768211456.0;

	data64 = (const uint64_t *)((const uint8_t*)target + 8);
	dcut64 += *data64 * 18446744073709551616.0;

	data64 = (const uint64_t *)(target);
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

bool isSha256Valid(const void* sha256)
{
    for(uint8_t i=0; i < 8; ++i)
    {
        if ( ((const uint32_t*)sha256)[i] != 0 ) 
            return true;
    }
    return false;
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

/**
 * get random extranonce2
*/
void getRandomExtranonce2(int extranonce2_size, char *extranonce2) {
  uint8_t b0, b1, b2, b3;

  b0 = rand() % 256;
  b1 = rand() % 256;
  b2 = rand() % 256;
  b3 = rand() % 256;

  unsigned long extranonce2_number = b3 << 24 | b2 <<  16 | b1 << 8 | b0;

  char format[] = "%00x";

  sprintf(&format[1], "%02dx", extranonce2_size * 2);
  sprintf(extranonce2, format, extranonce2_number);
}

/**
 * get linear extranonce2
*/
void getNextExtranonce2(int extranonce2_size, char *extranonce2) {
  unsigned long extranonce2_number = strtoul(extranonce2, NULL, 10);

  extranonce2_number++;

  char format[] = "%00x";

  sprintf(&format[1], "%02dx", extranonce2_size * 2);
  sprintf(extranonce2, format, extranonce2_number);
}

miner_data init_miner_data(void){
  
  miner_data newMinerData; 
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
    //char extranonce2_char[2 * mWorker.extranonce2_size+1];	
	//mWorker.extranonce2.toCharArray(extranonce2_char, 2 * mWorker.extranonce2_size + 1);
    //getNextExtranonce2(mWorker.extranonce2_size, extranonce2_char);
    if (mWorker.extranonce2_size == 2)
        mWorker.extranonce2 = "0001";
    else if (mWorker.extranonce2_size == 4)
        mWorker.extranonce2 = "00000001";
    else if (mWorker.extranonce2_size == 8)
        mWorker.extranonce2 = "0000000000000001";
    else
    {
        Serial.println("Unknown extranonce2");
        mWorker.extranonce2 = "00000001";
    }
    //mWorker.extranonce2 = "00000002";
    
    //get coinbase - coinbase_hash_bin = hashlib.sha256(hashlib.sha256(binascii.unhexlify(coinbase)).digest()).digest()
    // Use char buffer instead of String concatenation to avoid memory leaks
    static char coinbase_buffer[512]; // Static buffer to avoid repeated allocation
    snprintf(coinbase_buffer, sizeof(coinbase_buffer), "%s%s%s%s", 
             mJob.coinb1.c_str(), mWorker.extranonce1.c_str(), 
             mWorker.extranonce2.c_str(), mJob.coinb2.c_str());
    Serial.print("    coinbase: "); Serial.println(coinbase_buffer);
    size_t str_len = strlen(coinbase_buffer)/2;
    uint8_t bytearray[str_len];

    size_t res = to_byte_array(coinbase_buffer, str_len*2, bytearray);

    #ifdef DEBUG_MINING
    Serial.print("    extranonce2: "); Serial.println(mWorker.extranonce2);
    Serial.print("    coinbase: "); Serial.println(coinbase_buffer);
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
	char suffix[2] = {0,0};
	bool decimal = true;
	double dval;

	if (val >= exa) {
		val /= peta;
		dval = val / kilo;
        suffix[0] = 'E';
        if (dval > 999.99)
            dval = 999.99;
	} else if (val >= peta) {
		val /= tera;
		dval = val / kilo;
		suffix[0] = 'P';
	} else if (val >= tera) {
		val /= giga;
		dval = val / kilo;
		suffix[0] = 'T';
	} else if (val >= giga) {
		val /= mega;
		dval = val / kilo;
		suffix[0] = 'G';
	} else if (val >= mega) {
		val /= kilo;
		dval = val / kilo;
		suffix[0] = 'M';
	} else if (val >= kilo) {
		dval = val / kilo;
		suffix[0] = 'K';
	} else {
		dval = val;
		if (dval < min_diff)
			dval = 0.0;
	}
    
    int frac = 3;
    if (suffix[0] != 0)
    {
        if (dval > 99.999)
            frac = 1;
        else if (dval > 9.999)
            frac = 2;
    } else
    {
        if (dval > 99.999)
            frac = 2;
        else if (dval > 9.999)
            frac = 3;
        else
            frac = 4;
    }

	if (!sigdigits) {
		if (decimal)
        {
            if (frac == 4)
			    snprintf(buf, bufsiz, "%.4f%s", dval, suffix);
            else if (frac == 3)
			    snprintf(buf, bufsiz, "%.3f%s", dval, suffix);
            else if (frac == 2)
			    snprintf(buf, bufsiz, "%.2f%s", dval, suffix);
            else
			    snprintf(buf, bufsiz, "%.1f%s", dval, suffix);
        } else
			snprintf(buf, bufsiz, "%d%s", (unsigned int)dval, suffix);
	} else {
		/* Always show sigdigits + 1, padded on right with zeroes
		 * followed by suffix */
		int ndigits = sigdigits - 1 - (dval > 0.0 ? floor(log10(dval)) : 0);

		snprintf(buf, bufsiz, "%*.*f%s", sigdigits + 1, ndigits, dval, suffix);
	}
}



static const uint32_t s_crc32_table[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t crc32_reset()
{
    return 0xFFFFFFFF;
}

uint32_t crc32_add(uint32_t crc32, const void* data, size_t size)
{
    for (size_t n = 0; n < size; ++n)
        crc32 = (crc32 >> 8) ^ s_crc32_table[(crc32 ^ ((const uint8_t*)data)[n]) & 0xFF];
    return crc32;
}

uint32_t crc32_finish(uint32_t crc32)
{
    return crc32 ^ 0xFFFFFFFF;
}

