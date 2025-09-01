#ifdef HW_SHA256_TEST

#include <soc/soc.h>
#include "ShaTests/nerdSHA256plus.h"
#include "mbedtls/sha256.h"
#include <sha/sha_dma.h>
#include <sha/sha_parallel_engine.h>
#include <hal/sha_hal.h>
#include <hal/sha_ll.h>
#include <esp_heap_caps.h>
#include <hal/gdma_types.h>
#include <esp_crypto_shared_gdma.h>
#include <driver/periph_ctrl.h>

static const uint8_t s_test_buffer[128] = 
{
  0x00, 0x00, 0x00, 0x22, 0x99, 0x44, 0xbb, 0xff, 0xbb, 0x00, 0x00, 0x77, 0x44, 0xcc, 0x11, 0x77,
  0x88, 0x55, 0xbb, 0x44, 0x55, 0x00, 0x77, 0x88, 0x99, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xbb, 0xbb, 0x66, 0x11, 0x88, 0x33, 0x44, 0x99, 0xcc, 0x33, 0xff, 0x22,
  0x11, 0xaa, 0x77, 0xee, 0xbb, 0x66, 0xee, 0xcc, 0xee, 0x66, 0xee, 0xdd, 0x77, 0x55, 0x22, 0x22,
  0xcc, 0xcc, 0x66, 0xee, 0x22, 0xdd, 0x99, 0x66, 0x66, 0x88, 0x00, 0x11, 0x2e, 0x33, 0x41, 0x19,

  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80
};

static const uint8_t s_test_buffer_aligned[128] __attribute__((aligned(256))) = 
{
  0x00, 0x00, 0x00, 0x22, 0x99, 0x44, 0xbb, 0xff, 0xbb, 0x00, 0x00, 0x77, 0x44, 0xcc, 0x11, 0x77,
  0x88, 0x55, 0xbb, 0x44, 0x55, 0x00, 0x77, 0x88, 0x99, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xbb, 0xbb, 0x66, 0x11, 0x88, 0x33, 0x44, 0x99, 0xcc, 0x33, 0xff, 0x22,
  0x11, 0xaa, 0x77, 0xee, 0xbb, 0x66, 0xee, 0xcc, 0xee, 0x66, 0xee, 0xdd, 0x77, 0x55, 0x22, 0x22,
  0xcc, 0xcc, 0x66, 0xee, 0x22, 0xdd, 0x99, 0x66, 0x66, 0x88, 0x00, 0x11, 0x2e, 0x33, 0x41, 0x19,

  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80
};

static uint8_t interResult_aligned[64] __attribute__((aligned(256)));
static uint8_t midstate_aligned[32] __attribute__((aligned(256)));
static uint8_t hash_aligned[64] __attribute__((aligned(256)));

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
static inline void nerd_sha_hal_wait_idle()
{
    while (REG_READ(SHA_BUSY_REG))
    {}
}

static inline void nerd_sha_ll_fill_text_block_sha256(const void *input_text)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    REG_WRITE(&reg_addr_buf[0], data_words[0]);
    REG_WRITE(&reg_addr_buf[1], data_words[1]);
    REG_WRITE(&reg_addr_buf[2], data_words[2]);
    REG_WRITE(&reg_addr_buf[3], data_words[3]);
    REG_WRITE(&reg_addr_buf[4], data_words[4]);
    REG_WRITE(&reg_addr_buf[5], data_words[5]);
    REG_WRITE(&reg_addr_buf[6], data_words[6]);
    REG_WRITE(&reg_addr_buf[7], data_words[7]);
    REG_WRITE(&reg_addr_buf[8], data_words[8]);
    REG_WRITE(&reg_addr_buf[9], data_words[9]);
    REG_WRITE(&reg_addr_buf[10], data_words[10]);
    REG_WRITE(&reg_addr_buf[11], data_words[11]);
    REG_WRITE(&reg_addr_buf[12], data_words[12]);
    REG_WRITE(&reg_addr_buf[13], data_words[13]);
    REG_WRITE(&reg_addr_buf[14], data_words[14]);
    REG_WRITE(&reg_addr_buf[15], data_words[15]);
}

static inline void nerd_sha_ll_write_digest_sha256(void *digest_state)
{
    uint32_t *digest_state_words = (uint32_t *)digest_state;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_H_BASE);

    REG_WRITE(&reg_addr_buf[0], digest_state_words[0]);
    REG_WRITE(&reg_addr_buf[1], digest_state_words[1]);
    REG_WRITE(&reg_addr_buf[2], digest_state_words[2]);
    REG_WRITE(&reg_addr_buf[3], digest_state_words[3]);
    REG_WRITE(&reg_addr_buf[4], digest_state_words[4]);
    REG_WRITE(&reg_addr_buf[5], digest_state_words[5]);
    REG_WRITE(&reg_addr_buf[6], digest_state_words[6]);
    REG_WRITE(&reg_addr_buf[7], digest_state_words[7]);
}

//void IRAM_ATTR esp_dport_access_read_buffer(uint32_t *buff_out, uint32_t address, uint32_t num_words)
static inline void nerd_sha_ll_read_digest(void* ptr)
{
    DPORT_INTERRUPT_DISABLE();
#if 0
    for (uint32_t i = 0;  i < 256 / 32; ++i)
    {
        ((uint32_t*)ptr)[i] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + i * 4);
    }
#else
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4);
  ((uint32_t*)ptr)[7] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4);
#endif
    DPORT_INTERRUPT_RESTORE();
}

static IRAM_ATTR uint8_t dma_buffer[128] __attribute__((aligned(32)));
static IRAM_ATTR uint8_t dma_inter[64] __attribute__((aligned(32)));
static IRAM_ATTR uint8_t dma_hash[32] __attribute__((aligned(32)));
static DRAM_ATTR lldesc_t s_dma_descr_input;
static DRAM_ATTR lldesc_t s_dma_descr_buf;
static DRAM_ATTR lldesc_t s_dma_descr_inter;

#endif

#if defined(CONFIG_IDF_TARGET_ESP32)
static inline void nerd_sha_ll_read_digest_swap(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  ((uint32_t*)ptr)[0] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4));
  ((uint32_t*)ptr)[1] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4));
  ((uint32_t*)ptr)[2] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4));
  ((uint32_t*)ptr)[3] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4));
  ((uint32_t*)ptr)[4] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4));
  ((uint32_t*)ptr)[5] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4));
  ((uint32_t*)ptr)[6] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4));
  ((uint32_t*)ptr)[7] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4));
  DPORT_INTERRUPT_RESTORE();
}

static inline void nerd_sha_ll_read_digest(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4);
  ((uint32_t*)ptr)[7] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4);
  DPORT_INTERRUPT_RESTORE();
}

static inline void nerd_sha_hal_wait_idle()
{
    while (DPORT_REG_READ(SHA_256_BUSY_REG))
    {}
}

static inline void nerd_sha_ll_fill_text_block_sha256(const void *input_text)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = data_words[0];
    reg_addr_buf[1]  = data_words[1];
    reg_addr_buf[2]  = data_words[2];
    reg_addr_buf[3]  = data_words[3];
    reg_addr_buf[4]  = data_words[4];
    reg_addr_buf[5]  = data_words[5];
    reg_addr_buf[6]  = data_words[6];
    reg_addr_buf[7]  = data_words[7];
    reg_addr_buf[8]  = data_words[8];
    reg_addr_buf[9]  = data_words[9];
    reg_addr_buf[10] = data_words[10];
    reg_addr_buf[11] = data_words[11];
    reg_addr_buf[12] = data_words[12];
    reg_addr_buf[13] = data_words[13];
    reg_addr_buf[14] = data_words[14];
    reg_addr_buf[15] = data_words[15];
}

static inline void nerd_sha_ll_fill_text_block_sha256_swap(const void *input_text)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = __builtin_bswap32(data_words[0]);
    reg_addr_buf[1]  = __builtin_bswap32(data_words[1]);
    reg_addr_buf[2]  = __builtin_bswap32(data_words[2]);
    reg_addr_buf[3]  = __builtin_bswap32(data_words[3]);
    reg_addr_buf[4]  = __builtin_bswap32(data_words[4]);
    reg_addr_buf[5]  = __builtin_bswap32(data_words[5]);
    reg_addr_buf[6]  = __builtin_bswap32(data_words[6]);
    reg_addr_buf[7]  = __builtin_bswap32(data_words[7]);
    reg_addr_buf[8]  = __builtin_bswap32(data_words[8]);
    reg_addr_buf[9]  = __builtin_bswap32(data_words[9]);
    reg_addr_buf[10] = __builtin_bswap32(data_words[10]);
    reg_addr_buf[11] = __builtin_bswap32(data_words[11]);
    reg_addr_buf[12] = __builtin_bswap32(data_words[12]);
    reg_addr_buf[13] = __builtin_bswap32(data_words[13]);
    reg_addr_buf[14] = __builtin_bswap32(data_words[14]);
    reg_addr_buf[15] = __builtin_bswap32(data_words[15]);
}

static inline void nerd_sha_ll_fill_text_block_sha256_double(const void *input_text)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

#if 0
    //No change
    reg_addr_buf[0]  = data_words[0];
    reg_addr_buf[1]  = data_words[1];
    reg_addr_buf[2]  = data_words[2];
    reg_addr_buf[3]  = data_words[3];
    reg_addr_buf[4]  = data_words[4];
    reg_addr_buf[5]  = data_words[5];
    reg_addr_buf[6]  = data_words[6];
    reg_addr_buf[7]  = data_words[7];
#endif
    reg_addr_buf[8]  = 0x80000000;
    reg_addr_buf[9]  = 0x00000000;
    reg_addr_buf[10] = 0x00000000;
    reg_addr_buf[11] = 0x00000000;
    reg_addr_buf[12] = 0x00000000;
    reg_addr_buf[13] = 0x00000000;
    reg_addr_buf[14] = 0x00000000;
    reg_addr_buf[15] = 0x00000100;
}
#endif

IRAM_ATTR void HwShaTest()
{
  uint8_t interResult[64];
  uint8_t midstate[32];
  uint8_t hash[64];
  memset(interResult, 0, sizeof(interResult));
  interResult[32] = 0x80;
  interResult[62] = 0x01;
  interResult[63] = 0x00;

  memset(interResult_aligned, 0, sizeof(interResult_aligned));
  interResult_aligned[32] = 0x80;
  interResult_aligned[62] = 0x01;
  interResult_aligned[63] = 0x00;
  
  uint32_t bake[16];

  uint32_t time_start = micros();
  int test_count = 1000000;

#if 0
  //Generic software
  //esp32s3 16KH/s
  //esp32D  9.5KH/s
  test_count = 20000;
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  for (int i = 0; i < test_count; ++i)
  { 
    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, s_test_buffer, 80);
    mbedtls_sha256_finish_ret(&ctx, interResult);

    mbedtls_sha256_starts_ret(&ctx,0);
    mbedtls_sha256_update_ret(&ctx, interResult, 32);
    mbedtls_sha256_finish_ret(&ctx, hash);
  }
  mbedtls_sha256_free(&ctx);
#endif

#if 1
  //nerdSha256
  //ESP32   39KH/s
  //ESP32S3 39.01KH/s
  test_count = 100000;
  nerdSHA256_context ctx;
  nerd_mids(ctx.digest, s_test_buffer);
  for (int i = 0; i < test_count; ++i)
  {
    nerd_sha256d(&ctx, s_test_buffer+64, hash);
  }
#endif

#if 0
  //nerdSha256 bake
  //ESP32   : 41KH/s
  //ESP32S3 : 42.32KH/s
  test_count = 100000;
  nerdSHA256_context ctx;
  nerd_mids(&ctx, s_test_buffer);
  nerd_sha256_bake(ctx.digest, s_test_buffer+64, bake);  //15 words
  for (int i = 0; i < test_count; ++i)
  {
    nerd_sha256d_baked(ctx.digest, s_test_buffer+64, bake, hash);
  }
#endif

#if 0
  //Hardware high level 62KH/s
  esp_sha_acquire_hardware();
  for (int i = 0; i < test_count; ++i)
  {
      esp_sha_dma(SHA2_256, s_test_buffer+64, 64, s_test_buffer, 64, true);
      esp_sha_read_digest_state(SHA2_256, interResult);
      esp_sha_dma(SHA2_256, 0, 0, interResult, 64, true);
      esp_sha_read_digest_state(SHA2_256, hash);
  }
  esp_sha_release_hardware();
#endif

#if 0
  //ESP32D 5.50KH/s
  test_count = 40000;
  //esp_sha_lock_engine(SHA2_256);
  for (int i = 0; i < test_count; ++i)
  {
      esp_sha(SHA2_256, s_test_buffer, 80, interResult);
      esp_sha(SHA2_256, interResult, 32, hash);
  }
  //esp_sha_unlock_engine(SHA2_256);
#endif

#if 0
  //ESP32D
  //Invalid result!!
  test_count = 100000;
  esp_sha_lock_engine(SHA2_256);
  for (int i = 0; i < test_count; ++i)
  {
      esp_sha_block(SHA2_256, s_test_buffer, true);
      esp_sha_block(SHA2_256, s_test_buffer+64, false);
      esp_sha_read_digest_state(SHA2_256, interResult);
      esp_sha_block(SHA2_256, interResult, true);
      esp_sha_read_digest_state(SHA2_256, hash);
  }
  esp_sha_unlock_engine(SHA2_256);
#endif

#if 0
  //ESP32D Hardware SHA ~200KH/s  
  test_count = 50000;
  periph_module_enable(PERIPH_SHA_MODULE);
  uint8_t buffer_swap[128];
  for (int i = 0; i < 32; ++i)
    ((uint32_t*)buffer_swap)[i] = __builtin_bswap32(((const uint32_t*)s_test_buffer)[i]);

  uint8_t inter_swap[64];
  for (int i = 0; i < 16; ++i)
    ((uint32_t*)inter_swap)[i] = __builtin_bswap32(((const uint32_t*)interResult)[i]);

  for (int i = 0; i < test_count; ++i)
  {
      //sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
      nerd_sha_hal_wait_idle();
      nerd_sha_ll_fill_text_block_sha256(buffer_swap);
      sha_ll_start_block(SHA2_256);

      //sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
      nerd_sha_hal_wait_idle();
      nerd_sha_ll_fill_text_block_sha256(buffer_swap+64);
      sha_ll_continue_block(SHA2_256);

      nerd_sha_hal_wait_idle();
      sha_ll_load(SHA2_256);
      //nerd_sha_ll_read_digest_swap(interResult);

      //sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
      nerd_sha_hal_wait_idle();
      nerd_sha_ll_fill_text_block_sha256_double(inter_swap);
      sha_ll_start_block(SHA2_256);

      nerd_sha_hal_wait_idle();
      sha_ll_load(SHA2_256);
      nerd_sha_ll_read_digest_swap(hash);
  }
#endif

#if 0
  //Hardware low level + midstate 156KH/s
  esp_sha_acquire_hardware();
  sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
  sha_hal_read_digest(SHA2_256, midstate);
  for (int i = 0; i < test_count; ++i)
  {
      sha_hal_write_digest(SHA2_256, midstate);
      sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
      sha_hal_read_digest(SHA2_256, interResult);
      sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
      sha_hal_read_digest(SHA2_256, hash);
  }
  esp_sha_release_hardware();
#endif

#if 0
  //Hardware low level + midstate + aligned 156KH/s (No sense)
  esp_sha_acquire_hardware();
  sha_hal_hash_block(SHA2_256, s_test_buffer_aligned, 64/4, true);
  sha_hal_read_digest(SHA2_256, midstate_aligned);
  for (int i = 0; i < test_count; ++i)
  {
      sha_hal_write_digest(SHA2_256, midstate_aligned);
      sha_hal_hash_block(SHA2_256, s_test_buffer_aligned+64, 64/4, false);
      sha_hal_read_digest(SHA2_256, interResult_aligned);
      sha_hal_hash_block(SHA2_256, interResult_aligned, 64/4, true);
      sha_hal_read_digest(SHA2_256, hash_aligned);
  }
  esp_sha_release_hardware();
  memcpy(hash, hash_aligned, sizeof(hash_aligned));
#endif

#if 0
  //Hardware LL 162.43KH/s
  esp_sha_acquire_hardware();
  //sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
  sha_hal_wait_idle();
  sha_ll_fill_text_block(s_test_buffer, 64/4);
  sha_ll_start_block(SHA2_256);

  //sha_hal_read_digest(SHA2_256, midstate);
  sha_ll_load(SHA2_256);
  sha_hal_wait_idle();
  sha_ll_read_digest(SHA2_256, midstate, 256 / 32);

  for (int i = 0; i < test_count; ++i)
  {
      //sha_hal_write_digest(SHA2_256, midstate);
      sha_ll_write_digest(SHA2_256, midstate, 256 / 32);
      //nerd_sha_ll_write_digest_sha256(midstate);
      
      //sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
      //sha_hal_wait_idle();
      nerd_sha_hal_wait_idle();
      //sha_ll_fill_text_block(s_test_buffer+64, 64/4);
      nerd_sha_ll_fill_text_block_sha256(s_test_buffer+64);
      sha_ll_continue_block(SHA2_256);
      
      //sha_hal_read_digest(SHA2_256, interResult);
      sha_ll_load(SHA2_256);
      //sha_hal_wait_idle();
      nerd_sha_hal_wait_idle();
      //sha_ll_read_digest(SHA2_256, interResult, 256 / 32);
      nerd_sha_ll_read_digest(interResult);
      
      //sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
      //sha_hal_wait_idle();
      nerd_sha_hal_wait_idle();
      //sha_ll_fill_text_block(interResult, 64/4);
      nerd_sha_ll_fill_text_block_sha256(interResult);
      sha_ll_start_block(SHA2_256);

      //sha_hal_read_digest(SHA2_256, hash);
      sha_ll_load(SHA2_256);
      //sha_hal_wait_idle();
      nerd_sha_hal_wait_idle();
      //sha_ll_read_digest(SHA2_256, hash, 256 / 32);
      nerd_sha_ll_read_digest(hash);
  }
  esp_sha_release_hardware();
#endif

#if 0
  //DMA hash
  uint8_t* dma_cap_buf = (uint8_t*)heap_caps_malloc(128, MALLOC_CAP_8BIT|MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
  memcpy(dma_cap_buf, s_test_buffer, 128);

  uint8_t* dma_cap_inter = (uint8_t*)heap_caps_malloc(64, MALLOC_CAP_8BIT|MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
  memcpy(dma_cap_inter, interResult, 64);

  uint8_t* dma_cap_hash = (uint8_t*)heap_caps_malloc(32, MALLOC_CAP_8BIT|MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);

  memset(&s_dma_descr_input, 0, sizeof(lldesc_t));
  memset(&s_dma_descr_buf, 0, sizeof(lldesc_t));
  memset(&s_dma_descr_inter, 0, sizeof(lldesc_t));
  

  s_dma_descr_input.length = 64;
  s_dma_descr_input.size = 64;
  s_dma_descr_input.owner = 1;
  s_dma_descr_input.eof = 1;
  s_dma_descr_input.buf = dma_cap_buf+64;

  s_dma_descr_buf.length = 64;
  s_dma_descr_buf.size = 64;
  s_dma_descr_buf.owner = 1;
  s_dma_descr_buf.buf = dma_cap_buf;
  s_dma_descr_buf.eof = 0;
  s_dma_descr_buf.empty = (uint32_t)(&s_dma_descr_input);

  s_dma_descr_inter.length = 64;
  s_dma_descr_inter.size = 64;
  s_dma_descr_inter.owner = 1;
  s_dma_descr_inter.buf = dma_cap_inter;
  s_dma_descr_inter.eof = 1;

  //49.83KH/s
  esp_sha_acquire_hardware();
  for (int i = 0; i < test_count; ++i)
  {
    esp_crypto_shared_gdma_start(&s_dma_descr_buf, NULL, GDMA_TRIG_PERIPH_SHA);
    sha_hal_hash_dma(SHA2_256, 2, true);
    sha_hal_wait_idle();
    esp_sha_read_digest_state(SHA2_256, dma_cap_inter);
  
    esp_crypto_shared_gdma_start(&s_dma_descr_inter, NULL, GDMA_TRIG_PERIPH_SHA);
    sha_hal_hash_dma(SHA2_256, 1, true);
    sha_hal_wait_idle();
    esp_sha_read_digest_state(SHA2_256, hash);
  }
  esp_sha_release_hardware();
#endif

  uint32_t time_end = micros();
  double hash_rate = ((double)test_count * 1000000) / (double)(time_end - time_start);
  Serial.print("Hashrate=");
  Serial.print(hash_rate/1000);
  Serial.println("KH/s");

  Serial.print("interResult: ");
  for (size_t i = 0; i < 32; i++)
    Serial.printf("%02x", interResult[i]);
  Serial.println("");

    Serial.print("hash: ");
  for (size_t i = 0; i < 32; i++)
    Serial.printf("%02x", hash[i]);
  Serial.println("");
  
  //should be
  //54cd9f1ebc3db9a626688e5bb91d808abbd4079b2cba7f43fa08bfced300ef19
  //6fa464b007f2d577edfa5dfe9dfc3f9209f36d1a6711d314ea68ccdd03000000
}

#endif