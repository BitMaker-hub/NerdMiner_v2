#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <nvs.h>
//#include "ShaTests/nerdSHA256.h"
#include "ShaTests/nerdSHA256plus.h"
#include "stratum.h"
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "timeconst.h"
#include "drivers/displays/display.h"
#include "drivers/storage/storage.h"
#include <map>
#include <memory>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mbedtls/sha256.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"
#include "i2c_master.h"
#include "i2c_protocol.h"

#include "i2c_hash_config.h"
#if defined(I2C_HASH_SLAVE)
#include <driver/i2c.h>
#endif

#ifndef I2C_BOOT_SCAN_ROUNDS
#define I2C_BOOT_SCAN_ROUNDS 12
#endif
#ifndef I2C_BOOT_SCAN_DELAY_MS
#define I2C_BOOT_SCAN_DELAY_MS 300
#endif
#ifndef I2C_RUNTIME_RESCAN_MS
#define I2C_RUNTIME_RESCAN_MS 0
#endif
#ifndef I2C_MASTER_CYCLE_MS
#define I2C_MASTER_CYCLE_MS 120
#endif
#ifndef I2C_MASTER_IDLE_MS
#define I2C_MASTER_IDLE_MS 50
#endif
#ifndef I2C_MASTER_POST_HIT_DELAY_MS
#define I2C_MASTER_POST_HIT_DELAY_MS 2
#endif
#ifndef I2C_REBALANCE_ALPHA_PCT
#define I2C_REBALANCE_ALPHA_PCT 25
#endif
#ifndef I2C_REBALANCE_DEFAULT_RATE_HPS
#define I2C_REBALANCE_DEFAULT_RATE_HPS 200000.0f
#endif
#ifndef I2C_REBALANCE_MIN_RATE_HPS
#define I2C_REBALANCE_MIN_RATE_HPS 1000.0f
#endif
#ifndef I2C_REBALANCE_DEBUG
#define I2C_REBALANCE_DEBUG 0
#endif
#ifndef WIFI_RECONNECT_INTERVAL_MS
#define WIFI_RECONNECT_INTERVAL_MS 10000
#endif
#ifndef WIFI_FORCE_RECONNECT_INTERVAL_MS
#define WIFI_FORCE_RECONNECT_INTERVAL_MS 30000
#endif
#ifndef POOL_TCP_KEEPIDLE_S
#define POOL_TCP_KEEPIDLE_S 60
#endif
#ifndef POOL_TCP_KEEPINTVL_S
#define POOL_TCP_KEEPINTVL_S 15
#endif
#ifndef POOL_TCP_KEEPCNT
#define POOL_TCP_KEEPCNT 3
#endif
#ifndef POOL_STREAM_TIMEOUT_MS
#define POOL_STREAM_TIMEOUT_MS 2000
#endif
#ifndef MINER_POWER_SMOOTHING_SW_JOB_INTERVAL
#define MINER_POWER_SMOOTHING_SW_JOB_INTERVAL 0
#endif
#ifndef MINER_POWER_SMOOTHING_HW_JOB_INTERVAL
#define MINER_POWER_SMOOTHING_HW_JOB_INTERVAL 0
#endif
#ifndef MINER_POWER_SMOOTHING_DELAY_MS
#define MINER_POWER_SMOOTHING_DELAY_MS 1
#endif
#ifndef ADAPTIVE_SUGGEST_DIFFICULTY
#define ADAPTIVE_SUGGEST_DIFFICULTY 1
#endif
#ifndef SUGGEST_DIFF_FIXED_VALUE
#define SUGGEST_DIFF_FIXED_VALUE 16384.0
#endif
#ifndef SUGGEST_DIFF_TARGET_SHARE_TIME_S
#define SUGGEST_DIFF_TARGET_SHARE_TIME_S 30.0
#endif
#ifndef SUGGEST_DIFF_MIN
#define SUGGEST_DIFF_MIN 0.0001
#endif
#ifndef SUGGEST_DIFF_MAX
#define SUGGEST_DIFF_MAX 131072.0
#endif
#ifndef SUGGEST_DIFF_SMOOTH_ALPHA_PCT
#define SUGGEST_DIFF_SMOOTH_ALPHA_PCT 40
#endif

//10 Jobs per second
#if defined(CONFIG_IDF_TARGET_ESP32)
#define NONCE_PER_JOB_SW (32 * 1024)
#define NONCE_PER_JOB_HW (128 * 1024)
#else
#define NONCE_PER_JOB_SW (16 * 1024)
#define NONCE_PER_JOB_HW (64 * 1024)
#endif
static constexpr uint32_t kAdaptiveTargetMsDefault = 150;
static constexpr uint32_t kAdaptiveTargetMsMin = 80;
static constexpr uint32_t kAdaptiveTargetMsMax = 350;
static constexpr uint32_t kAdaptiveTargetStepDown = 20;
static constexpr uint32_t kAdaptiveTargetStepUp = 10;
static constexpr uint32_t kAdaptiveTargetWindowMs = 30000;
static volatile uint32_t s_adaptive_target_ms = kAdaptiveTargetMsDefault;
static constexpr uint32_t kAdaptiveMinSw = 4 * 1024;
static constexpr uint32_t kAdaptiveMaxSw = 128 * 1024;
static constexpr uint32_t kAdaptiveMinHw = 16 * 1024;
static constexpr uint32_t kAdaptiveMaxHw = 256 * 1024;

// Legacy alias kept for compatibility: I2C_SLAVE -> I2C_HASH_MASTER

//#define SHA256_VALIDATE
//#define RANDOM_NONCE
#define RANDOM_NONCE_MASK 0xFFFFC000

#ifdef HARDWARE_SHA265
#include <sha/sha_dma.h>
#include <hal/sha_hal.h>
#include <hal/sha_ll.h>

#if defined(CONFIG_IDF_TARGET_ESP32)
#include <sha/sha_parallel_engine.h>
#endif

#endif

nvs_handle_t stat_handle;

uint32_t templates = 0;
uint32_t hashes = 0;
uint32_t Mhashes = 0;
uint32_t totalKHashes = 0;
uint32_t elapsedKHs = 0;
uint64_t upTime = 0;

volatile uint32_t shares; // increase if blockhash has 32 bits of zeroes
volatile uint32_t valids; // increased if blockhash <= target
volatile uint32_t stales; // stale/late shares reported by pool

// Track best diff
double best_diff = 0.0;

// Variables to hold data from custom textboxes
//Track mining stats in non volatile memory
extern TSettings Settings;

IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres

//Global work data 
static WiFiClient client;
static miner_data mMiner; //Global miner data (Create a miner class TODO)
mining_subscribe mWorker;
mining_job mJob;
monitor_data mMonitor;
static bool volatile isMinerSuscribed = false;
unsigned long mLastTXtoPool = millis();
unsigned long mLastRXfromPool = millis();
static String s_last_subscribe_session_id;
static bool s_resume_subscribe_enabled = true;
static constexpr double kDiffOneHashes = 4294967296.0; // 2^32 hashes for difficulty 1
static double s_suggest_diff_smoothed = -1.0;
static uint64_t s_suggest_hashes_last = 0;
static uint32_t s_suggest_ms_last = 0;

int saveIntervals[7] = {5 * 60, 15 * 60, 30 * 60, 1 * 3600, 3 * 3600, 6 * 3600, 12 * 3600};
int saveIntervalsSize = sizeof(saveIntervals)/sizeof(saveIntervals[0]);
int currentIntervalIndex = 0;

static void copyCString(char *dst, size_t dstSize, const char *src)
{
  if (dst == nullptr || dstSize == 0)
    return;
  if (src == nullptr)
  {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

static void configurePoolSocket(WiFiClient &poolClient)
{
  poolClient.setNoDelay(true);
  // Stream timeout is in milliseconds; keep it long enough to read full JSON lines.
  poolClient.setTimeout(POOL_STREAM_TIMEOUT_MS);

  int on = 1;
  poolClient.setSocketOption(SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

#if defined(TCP_KEEPIDLE)
  int keepidle = POOL_TCP_KEEPIDLE_S;
  poolClient.setSocketOption(IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
#endif
#if defined(TCP_KEEPINTVL)
  int keepintvl = POOL_TCP_KEEPINTVL_S;
  poolClient.setSocketOption(IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
#endif
#if defined(TCP_KEEPCNT)
  int keepcnt = POOL_TCP_KEEPCNT;
  poolClient.setSocketOption(IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
#endif
}

static inline double clamp_double(double v, double lo, double hi)
{
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint64_t get_total_hashes_snapshot()
{
  return (uint64_t)Mhashes * 1000000ULL + (uint64_t)hashes;
}

static double calculateSuggestedDifficulty()
{
#if ADAPTIVE_SUGGEST_DIFFICULTY
  const uint32_t now = millis();
  const uint64_t total_hashes = get_total_hashes_snapshot();

  if (s_suggest_ms_last == 0 || now <= s_suggest_ms_last || total_hashes < s_suggest_hashes_last)
  {
    s_suggest_ms_last = now;
    s_suggest_hashes_last = total_hashes;
    if (s_suggest_diff_smoothed < 0.0)
      return SUGGEST_DIFF_MIN;
    return clamp_double(s_suggest_diff_smoothed, SUGGEST_DIFF_MIN, SUGGEST_DIFF_MAX);
  }

  const uint32_t elapsed_ms = now - s_suggest_ms_last;
  const uint64_t hash_delta = total_hashes - s_suggest_hashes_last;
  if (elapsed_ms >= 1000 && hash_delta > 0)
  {
    const double hps = ((double)hash_delta * 1000.0) / (double)elapsed_ms;
    double sample_diff = (hps * SUGGEST_DIFF_TARGET_SHARE_TIME_S) / kDiffOneHashes;
    sample_diff = clamp_double(sample_diff, SUGGEST_DIFF_MIN, SUGGEST_DIFF_MAX);

    if (s_suggest_diff_smoothed < 0.0)
    {
      s_suggest_diff_smoothed = sample_diff;
    }
    else
    {
      double alpha = (double)SUGGEST_DIFF_SMOOTH_ALPHA_PCT / 100.0;
      alpha = clamp_double(alpha, 0.0, 1.0);
      s_suggest_diff_smoothed = (1.0 - alpha) * s_suggest_diff_smoothed + alpha * sample_diff;
    }

    s_suggest_ms_last = now;
    s_suggest_hashes_last = total_hashes;
  }

  if (s_suggest_diff_smoothed < 0.0)
    return SUGGEST_DIFF_MIN;
  return clamp_double(s_suggest_diff_smoothed, SUGGEST_DIFF_MIN, SUGGEST_DIFF_MAX);
#else
  return SUGGEST_DIFF_FIXED_VALUE;
#endif
}

bool checkPoolConnection(void) {
  
  if (client.connected()) {
    return true;
  }
  
  isMinerSuscribed = false;

  Serial.println("Client not connected, trying to connect..."); 
  
  //Resolve first time pool DNS and save IP
  if(serverIP == IPAddress(1,1,1,1)) {
    if (!WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP))
    {
      Serial.printf("Failed to resolve pool DNS for: %s\n", Settings.PoolAddress.c_str());
      return false;
    }
    Serial.printf("Resolved DNS and saved IP (first time): %s\n", serverIP.toString().c_str());
  }

  //Try connecting pool IP
  if (!client.connect(serverIP, Settings.PoolPort)) {
    Serial.println("Imposible to connect to : " + Settings.PoolAddress);
    if (WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP))
      Serial.printf("Resolved DNS retry got: %s\n", serverIP.toString().c_str());
    else
      Serial.printf("DNS retry failed for: %s\n", Settings.PoolAddress.c_str());
    return false;
  }

  configurePoolSocket(client);
  uint32_t now = millis();
  mLastTXtoPool = now;
  mLastRXfromPool = now;
  Serial.printf("Connected to pool %s:%d\n", serverIP.toString().c_str(), Settings.PoolPort);
  return true;
}

//Implements a socketKeepAlive function and 
//checks if pool is not sending any data to reconnect again.
//Even connection could be alive, pool could stop sending new job NOTIFY
unsigned long mStart0Hashrate = 0;
bool checkPoolInactivity(unsigned int keepAliveTime, unsigned long inactivityTime, double currentDifficulty){ 

    unsigned long currentKHashes = (Mhashes*1000) + hashes/1000;
    unsigned long elapsedKHs = currentKHashes - totalKHashes;

    uint32_t time_now = millis();
    if (!isMinerSuscribed)
      return false;

    // Heartbeat by TX idle time:
    // if we spend too long without sending anything (e.g. no shares), push a light keepalive.
    if (time_now < mLastTXtoPool) //32bit wrap
      mLastTXtoPool = time_now;
    if (time_now < mLastRXfromPool) //32bit wrap
      mLastRXfromPool = time_now;

    bool tx_idle = (time_now > mLastTXtoPool + keepAliveTime);
    if (tx_idle)
    {
      double suggest_diff = calculateSuggestedDifficulty();
      Serial.println("  Sending  : Heartbeat (suggest_difficulty)");
      if (!tx_suggest_difficulty(client, suggest_diff)) {
        Serial.println("  Sending heartbeat to pool -> Detected client disconnected");
        return true;
      }
      mLastTXtoPool = time_now;
    }

    (void)currentDifficulty;
    (void)elapsedKHs;
    (void)inactivityTime;
    mStart0Hashrate = 0;
    return false;
}

struct JobData
{
  uint32_t id;
  double difficulty;
  uint8_t sha_buffer_sw[128];
  uint8_t sha_buffer_hw[128];
  uint32_t midstate[8];
  uint32_t bake[16];
  uint32_t hw_midstate[8];
};

struct JobRequest
{
  JobData *data;
  uint32_t nonce_start;
  uint32_t nonce_stride;
  uint32_t nonce_count;
};

struct JobResult
{
  uint32_t id;
  uint32_t nonce;
  uint32_t nonce_count;
  double difficulty;
  uint8_t hash[32];
};

#if defined(CONFIG_IDF_TARGET_ESP32)
static constexpr UBaseType_t kJobQueueDepth = 12;
static constexpr UBaseType_t kJobQueueTargetDepth = 6;
static constexpr UBaseType_t kResultQueueDepth = 24;
#else
static constexpr UBaseType_t kJobQueueDepth = 8;
static constexpr UBaseType_t kJobQueueTargetDepth = 4;
static constexpr UBaseType_t kResultQueueDepth = 16;
#endif
static constexpr size_t kJobResultsMax = 32;
static constexpr size_t kMaxPendingSubmissions = 64;

static QueueHandle_t s_job_queue_sw = nullptr;
#ifdef HARDWARE_SHA265
static QueueHandle_t s_job_queue_hw = nullptr;
#endif
static QueueHandle_t s_result_queue = nullptr;

static StaticQueue_t s_job_queue_sw_struct;
static uint8_t s_job_queue_sw_storage[kJobQueueDepth * sizeof(JobRequest)];
#ifdef HARDWARE_SHA265
static StaticQueue_t s_job_queue_hw_struct;
static uint8_t s_job_queue_hw_storage[kJobQueueDepth * sizeof(JobRequest)];
#endif
static StaticQueue_t s_result_queue_struct;
static uint8_t s_result_queue_storage[kResultQueueDepth * sizeof(JobResult)];
static volatile uint8_t s_working_current_job_id = 0xFF;
static volatile uint32_t s_sw_hashes_per_ms = 0;
static volatile uint32_t s_hw_hashes_per_ms = 0;
static constexpr uint8_t kJobDataPoolSize = 4;
static JobData s_job_data_pool[kJobDataPoolSize];
static uint8_t s_job_data_index = 0;
static JobData *s_current_job_data = nullptr;

static inline uint32_t clamp_u32(uint32_t v, uint32_t lo, uint32_t hi)
{
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint32_t calc_nonce_per_job_sw()
{
  uint32_t hpm = s_sw_hashes_per_ms;
  if (hpm == 0)
    return NONCE_PER_JOB_SW;
  uint32_t n = hpm * s_adaptive_target_ms;
  return clamp_u32(n, kAdaptiveMinSw, kAdaptiveMaxSw);
}

static inline uint32_t calc_nonce_per_job_hw()
{
  uint32_t hpm = s_hw_hashes_per_ms;
  if (hpm == 0)
    return NONCE_PER_JOB_HW;
  uint32_t n = hpm * s_adaptive_target_ms;
  return clamp_u32(n, kAdaptiveMinHw, kAdaptiveMaxHw);
}

static inline void update_hashrate(volatile uint32_t &rate, uint32_t hashes, uint32_t elapsed_us)
{
  if (elapsed_us == 0)
    return;
  uint32_t sample = (uint32_t)((uint64_t)hashes * 1000ULL / (uint64_t)elapsed_us);
  if (sample == 0)
    return;
  uint32_t prev = rate;
  if (prev == 0)
    rate = sample;
  else
    rate = (prev * 7 + sample) / 8;
}

static inline void apply_power_smoothing(uint32_t interval, uint32_t &counter)
{
  if (interval == 0)
    return;

  counter++;
  if (counter < interval)
    return;

  counter = 0;
  const TickType_t delay_ticks = MINER_POWER_SMOOTHING_DELAY_MS / portTICK_PERIOD_MS;
  if (delay_ticks > 0)
    vTaskDelay(delay_ticks);
  else
    taskYIELD();
}
static uint32_t mac_nonce_offset()
{
  static uint32_t cached = 0;
  if (cached != 0)
    return cached;
  uint64_t mac = ESP.getEfuseMac();
  uint32_t lo = (uint32_t)mac;
  uint32_t hi = (uint32_t)(mac >> 32);
  uint32_t mix = lo ^ hi;
  mix ^= (mix >> 16);
  mix *= 0x7feb352dU;
  mix ^= (mix >> 15);
  mix *= 0x846ca68bU;
  mix ^= (mix >> 16);
  if (mix == 0)
    mix = 1;
  cached = mix;
  return cached;
}

static bool is_stale_error(const String &line)
{
  static StaticJsonDocument<256> doc;
  static StaticJsonDocument<64> filter;

  filter.clear();
  filter["error"] = true;
  doc.clear();
  DeserializationError error = deserializeJson(doc, line, DeserializationOption::Filter(filter));
  if (error)
    return false;
  if (!doc.containsKey("error"))
    return false;
  if (doc["error"].isNull())
    return false;
  int code = doc["error"][0] | 0;
  const char *msg = doc["error"][1] | "";
  if (code == 21)
    return true;
  if (msg && (strstr(msg, "stale") || strstr(msg, "Stale") || strstr(msg, "job not found") || strstr(msg, "Job not found")))
    return true;
  return false;
}




void initMiningQueues()
{
  if (s_job_queue_sw != nullptr)
    return;

  s_job_queue_sw = xQueueCreateStatic(kJobQueueDepth, sizeof(JobRequest),
                                      s_job_queue_sw_storage, &s_job_queue_sw_struct);
#ifdef HARDWARE_SHA265
  s_job_queue_hw = xQueueCreateStatic(kJobQueueDepth, sizeof(JobRequest),
                                      s_job_queue_hw_storage, &s_job_queue_hw_struct);
#endif
  s_result_queue = xQueueCreateStatic(kResultQueueDepth, sizeof(JobResult),
                                      s_result_queue_storage, &s_result_queue_struct);
}

static bool JobPush(QueueHandle_t queue, JobData *data, uint32_t nonce_start, uint32_t nonce_count, uint32_t nonce_stride = 1)
{
  if (queue == nullptr)
    return false;

  JobRequest job{};
  job.data = data;
  job.nonce_start = nonce_start;
  job.nonce_stride = nonce_stride == 0 ? 1 : nonce_stride;
  job.nonce_count = nonce_count;
  return xQueueSend(queue, &job, 0) == pdTRUE;
}

struct Submition
{
  double diff;
  bool is32bit;
  bool isValid;
};

static void resetMiningQueuesOnly()
{
  if (s_result_queue)
    xQueueReset(s_result_queue);
  if (s_job_queue_sw)
    xQueueReset(s_job_queue_sw);
  #ifdef HARDWARE_SHA265
  if (s_job_queue_hw)
    xQueueReset(s_job_queue_hw);
  #endif
  s_working_current_job_id = 0xFF;
}

#ifdef I2C_HASH_MASTER
static void i2c_master_clear_active_job();
#endif

static void MiningJobStop(uint32_t &job_pool, std::map<uint32_t, std::shared_ptr<Submition>> & submition_map)
{
  resetMiningQueuesOnly();
#ifdef I2C_HASH_MASTER
  i2c_master_clear_active_job();
#endif
  job_pool = 0xFFFFFFFF;
  submition_map.clear();
}

#ifdef RANDOM_NONCE
uint64_t s_random_state = 1;
static uint32_t RandomGet()
{
    s_random_state += 0x9E3779B97F4A7C15ull;
    uint64_t z = s_random_state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

#endif

#if defined(I2C_HASH_SLAVE)

#ifndef I2C_HASH_BUS_PORT
#define I2C_HASH_BUS_PORT 0
#endif
#ifndef I2C_HASH_SDA_PIN
#define I2C_HASH_SDA_PIN 21
#endif
#ifndef I2C_HASH_SCL_PIN
#define I2C_HASH_SCL_PIN 22
#endif
#ifndef I2C_HASH_SLAVE_ADDRESS
#define I2C_HASH_SLAVE_ADDRESS 0x21
#endif
#ifndef I2C_HASH_CLOCK_HZ
#define I2C_HASH_CLOCK_HZ 100000
#endif
#ifndef I2C_SLAVE_DEBUG
#define I2C_SLAVE_DEBUG 0
#endif

static constexpr int kI2cSlaveRxBufLen = 256;
static constexpr int kI2cSlaveTxBufLen = 256;

struct I2cNonceStrideState
{
  uint32_t cursor;
  uint32_t stride;
};

static inline void i2c_stride_advance(I2cNonceStrideState &state, uint32_t nonce_count)
{
  if (nonce_count == 0)
    return;
  uint32_t step = state.stride == 0 ? 1 : state.stride;
  uint64_t adv = (uint64_t)step * (uint64_t)nonce_count;
  state.cursor = (uint32_t)(state.cursor + adv);
}

static int i2c_slave_start_worker()
{
  i2c_config_t config{};
  config.mode = I2C_MODE_SLAVE;
  config.sda_io_num = I2C_HASH_SDA_PIN;
  config.scl_io_num = I2C_HASH_SCL_PIN;
  config.sda_pullup_en = GPIO_PULLUP_ENABLE;
  config.scl_pullup_en = GPIO_PULLUP_ENABLE;
  config.slave.addr_10bit_en = 0;
  config.slave.slave_addr = I2C_HASH_SLAVE_ADDRESS;

  esp_err_t err = i2c_param_config((i2c_port_t)I2C_HASH_BUS_PORT, &config);
  if (err != ESP_OK)
    return err;
  return i2c_driver_install((i2c_port_t)I2C_HASH_BUS_PORT, config.mode, kI2cSlaveRxBufLen, kI2cSlaveTxBufLen, 0);
}

static void i2c_slave_refill_job_queues(JobData *job_data, I2cNonceStrideState &nonce_state)
{
  if (job_data == nullptr)
    return;

  const uint32_t sw_nonce_count = calc_nonce_per_job_sw();
  while (s_job_queue_sw && uxQueueMessagesWaiting(s_job_queue_sw) < kJobQueueTargetDepth)
  {
    if (!JobPush(s_job_queue_sw, job_data, nonce_state.cursor, sw_nonce_count, nonce_state.stride))
      break;
    i2c_stride_advance(nonce_state, sw_nonce_count);
  }

  #ifdef HARDWARE_SHA265
  const uint32_t hw_nonce_count = calc_nonce_per_job_hw();
  while (s_job_queue_hw && uxQueueMessagesWaiting(s_job_queue_hw) < kJobQueueTargetDepth)
  {
    if (!JobPush(s_job_queue_hw, job_data, nonce_state.cursor, hw_nonce_count, nonce_state.stride))
      break;
    i2c_stride_advance(nonce_state, hw_nonce_count);
  }
  #endif
}

static void i2c_slave_prepare_job(const JobI2cRequest &request, JobData *job_data, I2cNonceStrideState &nonce_state)
{
  if (job_data == nullptr)
    return;

  resetMiningQueuesOnly();

  memset(job_data, 0, sizeof(*job_data));
  job_data->id = request.id;
  job_data->difficulty = request.difficulty;

  memcpy(job_data->sha_buffer_sw, request.buffer, sizeof(request.buffer));
  memset(job_data->sha_buffer_sw + 80, 0, sizeof(job_data->sha_buffer_sw) - 80);
  job_data->sha_buffer_sw[80] = 0x80;
  job_data->sha_buffer_sw[126] = 0x02;
  job_data->sha_buffer_sw[127] = 0x80;

  nerd_mids(job_data->midstate, job_data->sha_buffer_sw);
  nerd_sha256_bake(job_data->midstate, job_data->sha_buffer_sw + 64, job_data->bake);

  #if defined(CONFIG_IDF_TARGET_ESP32)
  for (int i = 0; i < 32; ++i)
    ((uint32_t *)job_data->sha_buffer_hw)[i] = __builtin_bswap32(((const uint32_t *)job_data->sha_buffer_sw)[i]);
  #else
  memcpy(job_data->sha_buffer_hw, job_data->sha_buffer_sw, sizeof(job_data->sha_buffer_hw));
  #endif

  #ifdef HARDWARE_SHA265
  #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
  esp_sha_acquire_hardware();
  sha_hal_hash_block(SHA2_256, job_data->sha_buffer_sw, 64 / 4, true);
  sha_hal_read_digest(SHA2_256, job_data->hw_midstate);
  esp_sha_release_hardware();
  #else
  memset(job_data->hw_midstate, 0, sizeof(job_data->hw_midstate));
  #endif
  #else
  memset(job_data->hw_midstate, 0, sizeof(job_data->hw_midstate));
  #endif

  s_current_job_data = job_data;
  s_working_current_job_id = request.id;

  nonce_state.cursor = request.nonce_start;
  nonce_state.stride = request.nonce_stride == 0 ? 1 : request.nonce_stride;
  i2c_slave_refill_job_queues(job_data, nonce_state);
}

static JobI2cResult i2c_slave_collect_result(uint8_t active_job_id, JobData *job_data, I2cNonceStrideState &nonce_state)
{
  JobI2cResult out{};
  out.cmd = I2C_CMD_SLAVE_RESULT;
  out.id = active_job_id;
  out.nonce = kI2cInvalidNonce;
  out.processed_nonce = 0;

  double best_found_diff = -1.0;
  if (s_result_queue && job_data)
  {
    JobResult res{};
    while (xQueueReceive(s_result_queue, &res, 0) == pdTRUE)
    {
      if ((uint8_t)(res.id & 0xFF) != active_job_id)
        continue;
      out.processed_nonce += res.nonce_count;
      hashes += res.nonce_count;
      if (res.nonce != kI2cInvalidNonce && res.difficulty > best_found_diff)
      {
        best_found_diff = res.difficulty;
        out.nonce = res.nonce;
      }
    }
  }

  i2c_slave_refill_job_queues(job_data, nonce_state);
  out.crc = i2cCommandCrc8(&out, sizeof(out));
  return out;
}

static void i2c_slave_publish_result(const JobI2cResult &result)
{
  JobI2cResult msg = result;
  msg.crc = i2cCommandCrc8(&msg, sizeof(msg));
  i2c_slave_write_buffer((i2c_port_t)I2C_HASH_BUS_PORT, (uint8_t *)&msg, sizeof(msg), 10 / portTICK_PERIOD_MS);
}

void runI2cSlaveWorker(void *name)
{
  Serial.println("");
  Serial.printf("\n[I2C-SLAVE] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());

  initMiningQueues();
  mMonitor.NerdStatus = NM_hashing;

  int err = i2c_slave_start_worker();
  if (err != ESP_OK)
  {
    Serial.printf("[I2C-SLAVE] Failed to start bus, error=%d\n", err);
    while (1)
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  Serial.printf("[I2C-SLAVE] Ready on address 0x%02X (SDA=%d, SCL=%d)\n", I2C_HASH_SLAVE_ADDRESS, I2C_HASH_SDA_PIN, I2C_HASH_SCL_PIN);

  JobData *job_data = nullptr;
  I2cNonceStrideState nonce_state{};
  nonce_state.cursor = 0;
  nonce_state.stride = 1;
  uint8_t active_job_id = kI2cNoJobId;

  JobI2cResult idle{};
  idle.cmd = I2C_CMD_SLAVE_RESULT;
  idle.id = kI2cNoJobId;
  idle.nonce = kI2cInvalidNonce;
  idle.processed_nonce = 0;
  i2c_slave_publish_result(idle);

  uint8_t rx_chunk[96] = {0};
  uint8_t rx_buffer[256] = {0};
  size_t rx_buffer_len = 0;
  uint32_t wdt_counter = 0;
  uint32_t feed_count = 0;
  uint32_t req_count = 0;
  uint32_t crc_fail_count = 0;
  uint32_t invalid_cmd_count = 0;
  uint32_t rx_overflow_count = 0;
  uint32_t last_status_ms = millis();
  while (1)
  {
    int rx_len = i2c_slave_read_buffer((i2c_port_t)I2C_HASH_BUS_PORT, rx_chunk, sizeof(rx_chunk), 20 / portTICK_PERIOD_MS);
    if (rx_len > 0)
    {
      if (rx_buffer_len + (size_t)rx_len > sizeof(rx_buffer))
      {
        rx_overflow_count++;
        rx_buffer_len = 0;
        #if I2C_SLAVE_DEBUG
        Serial.printf("[I2C-SLAVE] RX overflow len=%d overflow=%u\n", rx_len, (unsigned)rx_overflow_count);
        #endif
      }
      else
      {
        memcpy(rx_buffer + rx_buffer_len, rx_chunk, (size_t)rx_len);
        rx_buffer_len += (size_t)rx_len;
      }

      size_t offset = 0;
      while (rx_buffer_len - offset >= 2)
      {
        const uint8_t cmd = rx_buffer[offset];
        size_t frame_len = 0;
        if (cmd == I2C_CMD_REQUEST_RESULT)
          frame_len = 2;
        else if (cmd == I2C_CMD_FEED)
          frame_len = sizeof(JobI2cRequest);
        else
        {
          invalid_cmd_count++;
          #if I2C_SLAVE_DEBUG
          Serial.printf("[I2C-SLAVE] Ignored cmd=0x%02X at offset=%u invalid=%u\n",
                        (unsigned)cmd, (unsigned)offset, (unsigned)invalid_cmd_count);
          #endif
          offset += 1;
          continue;
        }

        if (rx_buffer_len - offset < frame_len)
          break; // wait more bytes

        const uint8_t *frame = rx_buffer + offset;
        if (i2cCommandCrc8(frame, frame_len) != frame[1])
        {
          crc_fail_count++;
          #if I2C_SLAVE_DEBUG
          Serial.printf("[I2C-SLAVE] CRC error cmd=0x%02X frame_len=%u crc_fail=%u\n",
                        (unsigned)cmd, (unsigned)frame_len, (unsigned)crc_fail_count);
          #endif
          offset += 1; // resync stream
          continue;
        }

        if (cmd == I2C_CMD_FEED)
        {
          JobI2cRequest request{};
          memcpy(&request, frame, sizeof(request));
          JobData *slot = &s_job_data_pool[s_job_data_index++ % kJobDataPoolSize];
          i2c_slave_prepare_job(request, slot, nonce_state);
          job_data = slot;
          active_job_id = request.id;
          JobI2cResult result = i2c_slave_collect_result(active_job_id, job_data, nonce_state);
          i2c_slave_publish_result(result);
          feed_count++;
          #if I2C_SLAVE_DEBUG
          Serial.printf("[I2C-SLAVE] FEED #%u id=%u nonce_start=0x%08lX stride=%lu diff=%.9f\n",
                        (unsigned)feed_count, (unsigned)request.id,
                        (unsigned long)request.nonce_start,
                        (unsigned long)request.nonce_stride,
                        request.difficulty);
          #endif
        }
        else
        {
          req_count++;
          if (job_data && active_job_id != kI2cNoJobId)
          {
            JobI2cResult result = i2c_slave_collect_result(active_job_id, job_data, nonce_state);
            i2c_slave_publish_result(result);
            #if I2C_SLAVE_DEBUG
            if (result.processed_nonce > 0 || result.nonce != kI2cInvalidNonce)
            {
              Serial.printf("[I2C-SLAVE] RESULT req=%u id=%u processed=%u nonce=0x%08lX\n",
                            (unsigned)req_count, (unsigned)result.id, (unsigned)result.processed_nonce, (unsigned long)result.nonce);
            }
            #endif
          }
          else
          {
            i2c_slave_publish_result(idle);
          }
        }

        offset += frame_len;
      }

      if (offset > 0)
      {
        if (offset < rx_buffer_len)
          memmove(rx_buffer, rx_buffer + offset, rx_buffer_len - offset);
        rx_buffer_len -= offset;
      }
    }
    else if (job_data && active_job_id != kI2cNoJobId)
    {
      i2c_slave_refill_job_queues(job_data, nonce_state);
      vTaskDelay(2 / portTICK_PERIOD_MS);
    }
    else
    {
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }

    wdt_counter++;
    if (wdt_counter >= 8)
    {
      wdt_counter = 0;
      esp_task_wdt_reset();
    }

    #if I2C_SLAVE_DEBUG
    uint32_t now_ms = millis();
    if (now_ms - last_status_ms >= 5000)
    {
      last_status_ms = now_ms;
      uint32_t swq = s_job_queue_sw ? uxQueueMessagesWaiting(s_job_queue_sw) : 0;
      uint32_t hwq = 0;
      #ifdef HARDWARE_SHA265
      hwq = s_job_queue_hw ? uxQueueMessagesWaiting(s_job_queue_hw) : 0;
      #endif
      Serial.printf("[I2C-SLAVE] status id=%u feeds=%u req=%u crc_fail=%u ovf=%u swq=%u hwq=%u nonce_pool=0x%08lX\n",
                    (unsigned)active_job_id,
                    (unsigned)feed_count,
                    (unsigned)req_count,
                    (unsigned)crc_fail_count,
                    (unsigned)rx_overflow_count,
                    (unsigned)swq,
                    (unsigned)hwq,
                    (unsigned long)nonce_state.cursor);
    }
    #endif
  }
}

#else

void runI2cSlaveWorker(void *name)
{
  (void)name;
  vTaskDelete(nullptr);
}

#endif

#if defined(I2C_HASH_MASTER)

struct I2cMasterJobShared
{
  bool active;
  uint8_t job_id;
  uint32_t full_job_id;
  double difficulty;
  uint8_t header76[76];
  uint8_t sha_tail[64];
  uint32_t midstate[8];
  uint32_t bake[16];
  uint32_t generation;
};

struct I2cMasterJobSnapshot
{
  uint8_t job_id;
  uint32_t full_job_id;
  double difficulty;
  uint8_t header76[76];
  uint8_t sha_tail[64];
  uint32_t midstate[8];
  uint32_t bake[16];
  uint32_t generation;
};

static portMUX_TYPE s_i2c_master_lock = portMUX_INITIALIZER_UNLOCKED;
static I2cMasterJobShared s_i2c_master_job{};
static volatile uint32_t s_i2c_master_hashes_pending = 0;
static volatile uint8_t s_i2c_master_slave_count = 0;
static volatile bool s_i2c_master_boot_scan_done = false;

static bool i2c_master_has_workers()
{
  return s_i2c_master_slave_count > 0;
}

static void i2c_master_add_processed_hashes(uint32_t hashes_done)
{
  if (hashes_done == 0)
    return;
  taskENTER_CRITICAL(&s_i2c_master_lock);
  s_i2c_master_hashes_pending += hashes_done;
  taskEXIT_CRITICAL(&s_i2c_master_lock);
}

static uint32_t i2c_master_take_processed_hashes()
{
  taskENTER_CRITICAL(&s_i2c_master_lock);
  uint32_t out = s_i2c_master_hashes_pending;
  s_i2c_master_hashes_pending = 0;
  taskEXIT_CRITICAL(&s_i2c_master_lock);
  return out;
}

static void i2c_master_set_active_job(uint32_t full_job_id,
                                      double difficulty,
                                      const uint8_t *blockheader,
                                      const uint32_t *midstate,
                                      const uint32_t *bake)
{
  if (blockheader == nullptr || midstate == nullptr || bake == nullptr)
    return;

  taskENTER_CRITICAL(&s_i2c_master_lock);
  uint32_t next_generation = s_i2c_master_job.generation + 1;
  if (next_generation == 0)
    next_generation = 1;
  s_i2c_master_job.active = true;
  s_i2c_master_job.job_id = (uint8_t)(full_job_id & 0xFF);
  s_i2c_master_job.full_job_id = full_job_id;
  s_i2c_master_job.difficulty = difficulty;
  memcpy(s_i2c_master_job.header76, blockheader, sizeof(s_i2c_master_job.header76));
  memcpy(s_i2c_master_job.sha_tail, blockheader + 64, sizeof(s_i2c_master_job.sha_tail));
  memcpy(s_i2c_master_job.midstate, midstate, sizeof(s_i2c_master_job.midstate));
  memcpy(s_i2c_master_job.bake, bake, sizeof(s_i2c_master_job.bake));
  s_i2c_master_job.generation = next_generation;
  s_i2c_master_hashes_pending = 0;
  taskEXIT_CRITICAL(&s_i2c_master_lock);
}

static void i2c_master_clear_active_job()
{
  taskENTER_CRITICAL(&s_i2c_master_lock);
  uint32_t next_generation = s_i2c_master_job.generation + 1;
  if (next_generation == 0)
    next_generation = 1;
  s_i2c_master_job.active = false;
  s_i2c_master_job.generation = next_generation;
  s_i2c_master_hashes_pending = 0;
  taskEXIT_CRITICAL(&s_i2c_master_lock);
}

static bool i2c_master_read_job_snapshot(I2cMasterJobSnapshot &snapshot)
{
  bool active = false;
  taskENTER_CRITICAL(&s_i2c_master_lock);
  active = s_i2c_master_job.active;
  if (active)
  {
    snapshot.job_id = s_i2c_master_job.job_id;
    snapshot.full_job_id = s_i2c_master_job.full_job_id;
    snapshot.difficulty = s_i2c_master_job.difficulty;
    memcpy(snapshot.header76, s_i2c_master_job.header76, sizeof(snapshot.header76));
    memcpy(snapshot.sha_tail, s_i2c_master_job.sha_tail, sizeof(snapshot.sha_tail));
    memcpy(snapshot.midstate, s_i2c_master_job.midstate, sizeof(snapshot.midstate));
    memcpy(snapshot.bake, s_i2c_master_job.bake, sizeof(snapshot.bake));
    snapshot.generation = s_i2c_master_job.generation;
  }
  taskEXIT_CRITICAL(&s_i2c_master_lock);
  return active;
}

static uint32_t i2c_master_slaves_signature(const std::vector<uint8_t> &slaves)
{
  uint32_t sig = 2166136261u;
  for (size_t i = 0; i < slaves.size(); ++i)
  {
    sig ^= (uint32_t)slaves[i];
    sig *= 16777619u;
  }
  return sig ^ (uint32_t)slaves.size();
}

static void i2c_master_assign_nonce_starts(const std::vector<uint8_t> &slaves,
                                           std::vector<uint32_t> &nonce_starts,
                                           uint32_t &nonce_stride,
                                           uint32_t seed)
{
  nonce_starts.clear();
  if (slaves.empty())
  {
    nonce_stride = 1;
    return;
  }

  nonce_stride = (uint32_t)slaves.size();
  if (nonce_stride == 0)
    nonce_stride = 1;

  nonce_starts.resize(slaves.size(), 0);
  for (size_t i = 0; i < slaves.size(); ++i)
  {
    nonce_starts[i] = seed + (uint32_t)i;
  }
}

void runI2cMasterWorker(void *name)
{
  Serial.println("");
  Serial.printf("\n[I2C-MASTER] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());
  s_i2c_master_boot_scan_done = false;

  std::vector<uint8_t> i2c_slave_vector;
  std::vector<uint32_t> i2c_slave_nonce_starts;
  uint32_t i2c_slave_nonce_stride = 1;
  std::map<uint8_t, float> i2c_slave_rates_hps;
  bool i2c_master_ready = false;
  uint32_t i2c_last_scan = 0;
  uint32_t last_feed_generation = 0;
  uint32_t last_feed_signature = 0;
  uint32_t wdt_counter = 0;

  auto scan_i2c_workers = [&](bool verbose)
  {
    if (!i2c_master_ready)
    {
      int err = i2c_master_start();
      if (err != ESP_OK)
      {
        s_i2c_master_slave_count = 0;
        if (verbose)
          Serial.printf("[I2C-MASTER] bus init failed, err=%d\n", err);
        return;
      }
      i2c_master_ready = true;
      verbose = true;
    }

    std::vector<uint8_t> found = i2c_master_scan(0x08, 0x78);
    bool changed = (found != i2c_slave_vector);

    i2c_slave_vector = std::move(found);
    s_i2c_master_slave_count = (uint8_t)i2c_slave_vector.size();
    i2c_last_scan = millis();

    for (auto it = i2c_slave_rates_hps.begin(); it != i2c_slave_rates_hps.end();)
    {
      bool present = false;
      for (size_t i = 0; i < i2c_slave_vector.size(); ++i)
      {
        if (i2c_slave_vector[i] == it->first)
        {
          present = true;
          break;
        }
      }
      if (!present)
        it = i2c_slave_rates_hps.erase(it);
      else
        ++it;
    }
    for (size_t i = 0; i < i2c_slave_vector.size(); ++i)
    {
      uint8_t addr = i2c_slave_vector[i];
      if (i2c_slave_rates_hps.find(addr) == i2c_slave_rates_hps.end())
        i2c_slave_rates_hps[addr] = I2C_REBALANCE_DEFAULT_RATE_HPS;
    }

    if (verbose || changed)
    {
      Serial.printf("[I2C-MASTER] Found %d slave workers\n", i2c_slave_vector.size());
      if (!i2c_slave_vector.empty())
      {
        Serial.print("[I2C-MASTER] Workers: ");
        for (size_t n = 0; n < i2c_slave_vector.size(); ++n)
          Serial.printf("0x%02X,", (uint32_t)i2c_slave_vector[n]);
        Serial.println("");
      }
    }
    if (changed)
    {
      last_feed_signature = 0;
      i2c_slave_nonce_starts.clear();
      i2c_slave_nonce_stride = 1;
    }
  };

  uint32_t boot_scan_rounds = I2C_BOOT_SCAN_ROUNDS;
  if (boot_scan_rounds == 0)
    boot_scan_rounds = 1;
  for (uint32_t round = 0; round < boot_scan_rounds; ++round)
  {
    Serial.printf("[I2C-MASTER] Boot scan %u/%u\n", (unsigned)(round + 1), (unsigned)boot_scan_rounds);
    scan_i2c_workers(round == 0 || round + 1 == boot_scan_rounds);
    if (round + 1 < boot_scan_rounds && I2C_BOOT_SCAN_DELAY_MS > 0)
      vTaskDelay(I2C_BOOT_SCAN_DELAY_MS / portTICK_PERIOD_MS);
  }
  s_i2c_master_boot_scan_done = true;
  Serial.println("[I2C-MASTER] Boot scan complete.");

  while (1)
  {
    #if (I2C_RUNTIME_RESCAN_MS > 0)
    {
      uint32_t now_scan = millis();
      if (now_scan < i2c_last_scan || (now_scan - i2c_last_scan >= I2C_RUNTIME_RESCAN_MS))
      {
        scan_i2c_workers(i2c_slave_vector.empty());
      }
    }
    #endif

    I2cMasterJobSnapshot job{};
    bool has_job = i2c_master_read_job_snapshot(job);

    if (has_job && !i2c_slave_vector.empty())
    {
      uint32_t slave_signature = i2c_master_slaves_signature(i2c_slave_vector);
      if (job.generation != last_feed_generation || slave_signature != last_feed_signature)
      {
        uint32_t nonce_seed = job.full_job_id ^ (job.generation * 0x9E3779B9u) ^ slave_signature;
        i2c_master_assign_nonce_starts(i2c_slave_vector, i2c_slave_nonce_starts, i2c_slave_nonce_stride, nonce_seed);
        i2c_feed_slaves(i2c_slave_vector, job.job_id, i2c_slave_nonce_starts, i2c_slave_nonce_stride, (float)job.difficulty, job.header76);
        #if I2C_REBALANCE_DEBUG
        Serial.printf("[I2C-MASTER] Stride starts (stride=%lu): ", (unsigned long)i2c_slave_nonce_stride);
        for (size_t i = 0; i < i2c_slave_vector.size(); ++i)
        {
          uint32_t start = (i < i2c_slave_nonce_starts.size()) ? i2c_slave_nonce_starts[i] : 0;
          float rate = I2C_REBALANCE_DEFAULT_RATE_HPS;
          auto it = i2c_slave_rates_hps.find(i2c_slave_vector[i]);
          if (it != i2c_slave_rates_hps.end())
            rate = it->second;
          Serial.printf("0x%02X=>0x%08lX(%.0f) ", (uint32_t)i2c_slave_vector[i], (unsigned long)start, (double)rate);
        }
        Serial.println("");
        #endif
        last_feed_generation = job.generation;
        last_feed_signature = slave_signature;
      }

      uint32_t time_start = millis();
      i2c_hit_slaves(i2c_slave_vector);
      if (I2C_MASTER_POST_HIT_DELAY_MS > 0)
        vTaskDelay(I2C_MASTER_POST_HIT_DELAY_MS / portTICK_PERIOD_MS);
      std::vector<I2cSlaveHarvest> harvest_records = i2c_harvest_slaves(i2c_slave_vector, job.job_id);
      uint32_t time_end = millis();
      uint32_t elapsed_for_rate = (time_end > time_start) ? (time_end - time_start) : 0;
      if (elapsed_for_rate == 0)
        elapsed_for_rate = (I2C_MASTER_CYCLE_MS > 0) ? I2C_MASTER_CYCLE_MS : 1;

      uint32_t nonces_done = 0;
      std::vector<uint32_t> nonce_vector;
      std::map<uint8_t, uint32_t> processed_by_slave;
      for (size_t i = 0; i < harvest_records.size(); ++i)
      {
        const I2cSlaveHarvest &item = harvest_records[i];
        nonces_done += item.processed_nonce;
        processed_by_slave[item.address] += item.processed_nonce;
        if (item.has_nonce)
          nonce_vector.push_back(item.nonce);
      }
      i2c_master_add_processed_hashes(nonces_done);

      float alpha = (float)I2C_REBALANCE_ALPHA_PCT / 100.0f;
      if (alpha < 0.0f)
        alpha = 0.0f;
      if (alpha > 1.0f)
        alpha = 1.0f;
      for (size_t i = 0; i < i2c_slave_vector.size(); ++i)
      {
        uint8_t addr = i2c_slave_vector[i];
        uint32_t processed = 0;
        auto pit = processed_by_slave.find(addr);
        if (pit != processed_by_slave.end())
          processed = pit->second;

        float sample_rate = ((float)processed * 1000.0f) / (float)elapsed_for_rate;
        auto rit = i2c_slave_rates_hps.find(addr);
        if (rit == i2c_slave_rates_hps.end())
          i2c_slave_rates_hps[addr] = I2C_REBALANCE_DEFAULT_RATE_HPS;
        float prev_rate = i2c_slave_rates_hps[addr];
        float next_rate = sample_rate;
        if (prev_rate > 0.0f)
          next_rate = prev_rate + ((sample_rate - prev_rate) * alpha);
        if (next_rate < I2C_REBALANCE_MIN_RATE_HPS)
          next_rate = I2C_REBALANCE_MIN_RATE_HPS;
        i2c_slave_rates_hps[addr] = next_rate;
      }

      if (s_result_queue && !nonce_vector.empty())
      {
        for (size_t n = 0; n < nonce_vector.size(); ++n)
        {
          JobResult result{};
          if (nerd_sha256d_baked_nonce(job.midstate, nonce_vector[n], job.bake, result.hash))
          {
            result.id = job.full_job_id;
            result.nonce = nonce_vector[n];
            result.nonce_count = 0;
            result.difficulty = diff_from_target(result.hash);
            xQueueSend(s_result_queue, &result, 0);
          }
        }
      }

      if (time_end > time_start)
      {
        uint32_t elapsed = time_end - time_start;
        if (I2C_MASTER_CYCLE_MS > 0 && elapsed < I2C_MASTER_CYCLE_MS)
          vTaskDelay((I2C_MASTER_CYCLE_MS - elapsed) / portTICK_PERIOD_MS);
      }
      else if (I2C_MASTER_CYCLE_MS > 0)
      {
        vTaskDelay(I2C_MASTER_CYCLE_MS / portTICK_PERIOD_MS);
      }
    }
    else
    {
      if (I2C_MASTER_IDLE_MS > 0)
        vTaskDelay(I2C_MASTER_IDLE_MS / portTICK_PERIOD_MS);
      else
        vTaskDelay(1);
    }

    wdt_counter++;
    if (wdt_counter >= 8)
    {
      wdt_counter = 0;
      esp_task_wdt_reset();
    }
  }
}

#endif

void runStratumWorker(void *name) {

// TEST: https://bitcoin.stackexchange.com/questions/22929/full-example-data-for-scrypt-stratum-client

  Serial.println("");
  Serial.printf("\n[WORKER] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());

  initMiningQueues();

  #ifdef DEBUG_MEMORY
  Serial.printf("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
  #endif

  std::map<uint32_t, std::shared_ptr<Submition>> s_submition_map;

#ifdef I2C_HASH_MASTER
  static bool s_i2c_master_task_started = false;
  if (!s_i2c_master_task_started)
  {
    BaseType_t res = xTaskCreatePinnedToCore(runI2cMasterWorker, "I2C-Master", 7000, (void *)"(I2C-Master)", 3, nullptr, 0);
    if (res == pdPASS)
    {
      s_i2c_master_task_started = true;
    }
    else
    {
      Serial.println("[I2C-MASTER] Failed to start worker task");
    }
  }
  if (s_i2c_master_task_started)
  {
    Serial.println("[I2C-MASTER] Waiting boot scan before starting stratum...");
    while (!s_i2c_master_boot_scan_done)
      vTaskDelay(20 / portTICK_PERIOD_MS);
    Serial.println("[I2C-MASTER] Boot scan finished. Continuing stratum startup.");
  }
#endif

  // connect to pool  
  double currentPoolDifficulty = DEFAULT_DIFFICULTY;
  uint32_t nonce_pool = 0;
  uint32_t job_pool = 0xFFFFFFFF;
  uint32_t last_job_time = millis();
  uint32_t hw_midstate[8] = {0};
  uint32_t diget_mid[8] = {0};
  uint32_t bake[16] = {0};
  #if defined(CONFIG_IDF_TARGET_ESP32)
  uint8_t sha_buffer_swap[128] = {0};
  #endif

  while(true) {
      
    wl_status_t wifi_status = WiFi.status();
    if(wifi_status != WL_CONNECTED){
      // WiFi is disconnected.
      mMonitor.NerdStatus = NM_Connecting;
      MiningJobStop(job_pool, s_submition_map);
      static uint32_t last_wifi_reconnect = 0;
      uint32_t now = millis();
      if (now < last_wifi_reconnect) // wrap
        last_wifi_reconnect = 0;
      // Keep auto reconnect in control; force reconnect only if stuck for a while.
      bool allow_force_reconnect =
          (wifi_status == WL_DISCONNECTED) ||
          (wifi_status == WL_CONNECTION_LOST) ||
          (wifi_status == WL_CONNECT_FAILED) ||
          (wifi_status == WL_NO_SSID_AVAIL);
      if (allow_force_reconnect && now - last_wifi_reconnect >= WIFI_FORCE_RECONNECT_INTERVAL_MS)
      {
        WiFi.reconnect();
        last_wifi_reconnect = now;
      }
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    } 

    if(!checkPoolConnection()){
      //If server is not reachable add random delay for connection retries
      //Generate value between 1 and 60 secs
      MiningJobStop(job_pool, s_submition_map);
      vTaskDelay(((1 + rand() % 60) * 1000) / portTICK_PERIOD_MS);
      continue;
    }

    if(!isMinerSuscribed)
    {
      //Stop miner current jobs
      mWorker = init_mining_subscribe();

      // STEP 1: Pool server connection (SUBSCRIBE)
      bool resume_attempt = s_resume_subscribe_enabled && s_last_subscribe_session_id.length() > 0;
      if(!tx_mining_subscribe(client, mWorker, resume_attempt ? s_last_subscribe_session_id.c_str() : nullptr)) {
        if (resume_attempt)
        {
          Serial.println("[WORKER] Resume subscribe failed, trying fresh subscribe");
          s_resume_subscribe_enabled = false;
          if (!tx_mining_subscribe(client, mWorker, nullptr))
          {
            client.stop();
            MiningJobStop(job_pool, s_submition_map);
            continue;
          }
        }
        else
        {
          client.stop();
          MiningJobStop(job_pool, s_submition_map);
          continue;
        }
      }
      s_last_subscribe_session_id = mWorker.sub_details;
      mLastRXfromPool = millis();
      
      copyCString(mWorker.wName, sizeof(mWorker.wName), Settings.BtcWallet);
      copyCString(mWorker.wPass, sizeof(mWorker.wPass), Settings.PoolPassword);
      if (strlen(Settings.PoolPassword) >= sizeof(mWorker.wPass))
      {
        Serial.printf("[WORKER] Pool password truncated to %u chars\n", (unsigned)(sizeof(mWorker.wPass) - 1));
      }
      // STEP 2: Pool authorize work (Block Info)
      tx_mining_auth(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO
      //tx_mining_auth2(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO

      // STEP 3: Suggest pool difficulty
      tx_suggest_difficulty(client, calculateSuggestedDifficulty());

      isMinerSuscribed=true;
      uint32_t time_now = millis();
      mLastTXtoPool = time_now;
      last_job_time = time_now;
    }

    //Check if pool is down for almost 5minutes and then restart connection with pool (1min=600000ms)
    if(checkPoolInactivity(KEEPALIVE_TIME_ms, POOLINACTIVITY_TIME_ms, currentPoolDifficulty)){
      //Restart connection
      Serial.println("  Detected more than 2 min without data form stratum server. Closing socket and reopening...");
      client.stop();
      isMinerSuscribed=false;
      MiningJobStop(job_pool, s_submition_map);
      continue; 
    }

    {
      uint32_t time_now = millis();
      if (time_now < last_job_time) //32bit wrap
        last_job_time = time_now;
      if (time_now >= last_job_time + 10*60*1000)  //10minutes without job
      {
        client.stop();
        isMinerSuscribed=false;
        MiningJobStop(job_pool, s_submition_map);
        continue;
      }
    }

    //Read pending messages from pool
    while(client.connected() && client.available())
    {
      String line = client.readStringUntil('\n');
      if (line.length() == 0)
        break;
      mLastRXfromPool = millis();
      //Serial.println("  Received message from pool");      
      stratum_method result = parse_mining_method(line);
      switch (result)
      {
          case MINING_NOTIFY:         if(parse_mining_notify(line, mJob))
                                      {
                                          if (s_result_queue)
                                            xQueueReset(s_result_queue);
                                          if (s_job_queue_sw)
                                            xQueueReset(s_job_queue_sw);
                                          #ifdef HARDWARE_SHA265
                                          if (s_job_queue_hw)
                                            xQueueReset(s_job_queue_hw);
                                          #endif
                                          //Increse templates readed
                                          templates++;
                                          job_pool++;
                                          s_working_current_job_id = job_pool & 0xFF;

                                          last_job_time = millis();
                                          mLastTXtoPool = last_job_time;

                                          uint32_t mh = hashes/1000000;
                                          Mhashes += mh;
                                          hashes -= mh*1000000;

                                          //Prepare data for new jobs
                                          mMiner=calculateMiningData(mWorker, mJob);

                                          memset(mMiner.bytearray_blockheader+80, 0, 128-80);
                                          mMiner.bytearray_blockheader[80] = 0x80;
                                          mMiner.bytearray_blockheader[126] = 0x02;
                                          mMiner.bytearray_blockheader[127] = 0x80;

                                          nerd_mids(diget_mid, mMiner.bytearray_blockheader);
                                          nerd_sha256_bake(diget_mid, mMiner.bytearray_blockheader+64, bake);

                                          #ifdef HARDWARE_SHA265
                                          #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
                                            esp_sha_acquire_hardware();
                                            sha_hal_hash_block(SHA2_256,  mMiner.bytearray_blockheader, 64/4, true);
                                            sha_hal_read_digest(SHA2_256, hw_midstate);
                                            esp_sha_release_hardware();
                                          #endif
                                          #endif

                                          #if defined(CONFIG_IDF_TARGET_ESP32)
                                          for (int i = 0; i < 32; ++i)
                                            ((uint32_t*)sha_buffer_swap)[i] = __builtin_bswap32(((const uint32_t*)(mMiner.bytearray_blockheader))[i]);
                                          #endif

                                          #ifdef RANDOM_NONCE
                                          nonce_pool = RandomGet() & RANDOM_NONCE_MASK;
                                          #else
                                            const uint32_t mac_offset = mac_nonce_offset() & 0x0FFFFFFF;
                                            #ifdef I2C_HASH_MASTER
                                            if (i2c_master_has_workers())
                                              nonce_pool = 0x10000000 + mac_offset;
                                            else
                                            #endif
                                              nonce_pool = 0xDA54E700 + mac_offset;  //nonce 0x00000000 is not possible, start from some random nonce
                                          #endif
                                          

                                          JobData *job_data = &s_job_data_pool[s_job_data_index++ % kJobDataPoolSize];
                                          job_data->id = job_pool;
                                          job_data->difficulty = currentPoolDifficulty;
                                          memcpy(job_data->sha_buffer_sw, mMiner.bytearray_blockheader, sizeof(job_data->sha_buffer_sw));
                                          #if defined(CONFIG_IDF_TARGET_ESP32)
                                          memcpy(job_data->sha_buffer_hw, sha_buffer_swap, sizeof(job_data->sha_buffer_hw));
                                          #else
                                          memcpy(job_data->sha_buffer_hw, mMiner.bytearray_blockheader, sizeof(job_data->sha_buffer_hw));
                                          #endif
                                          memcpy(job_data->midstate, diget_mid, sizeof(job_data->midstate));
                                          memcpy(job_data->bake, bake, sizeof(job_data->bake));
                                          #ifdef HARDWARE_SHA265
                                          memcpy(job_data->hw_midstate, hw_midstate, sizeof(job_data->hw_midstate));
                                          #else
                                          memset(job_data->hw_midstate, 0, sizeof(job_data->hw_midstate));
                                          #endif
                                          s_current_job_data = job_data;

                                          const uint32_t sw_nonce_count = calc_nonce_per_job_sw();
                                          #ifdef HARDWARE_SHA265
                                          const uint32_t hw_nonce_count = calc_nonce_per_job_hw();
                                          #endif
                                          for (uint32_t i = 0; i < kJobQueueTargetDepth; ++i)
                                          {
                                            JobPush(s_job_queue_sw, job_data, nonce_pool, sw_nonce_count);
                                            nonce_pool += sw_nonce_count;
                                            #ifdef HARDWARE_SHA265
                                            JobPush(s_job_queue_hw, job_data, nonce_pool, hw_nonce_count);
                                            nonce_pool += hw_nonce_count;
                                            #endif
                                          }
                                          #ifdef I2C_HASH_MASTER
                                          i2c_master_set_active_job(job_pool, currentPoolDifficulty, mMiner.bytearray_blockheader, diget_mid, bake);
                                          #endif
                                      } else
                                      {
                                        Serial.println("Parsing error, need restart");
                                        client.stop();
                                        isMinerSuscribed=false;
                                        MiningJobStop(job_pool, s_submition_map);
                                      }
                                      break;
          case MINING_SET_DIFFICULTY: parse_mining_set_difficulty(line, currentPoolDifficulty);
                                      break;
          case STRATUM_SUCCESS:       {
                                        unsigned long id = parse_extract_id(line);
                                        auto it = s_submition_map.find(id);
                                        if (it != s_submition_map.end())
                                        {
                                          Submition &sub = *it->second;
                                          if (sub.diff > best_diff)
                                            best_diff = sub.diff;
                                          if (sub.is32bit)
                                            shares++;
                                          if (sub.isValid)
                                          {
                                            Serial.println("CONGRATULATIONS! Valid block found");
                                            valids++;
                                          }
                                          s_submition_map.erase(it);
                                        }
                                      }
                                      break;
          case STRATUM_PARSE_ERROR:   {
                                        unsigned long id = parse_extract_id(line);
                                        if (is_stale_error(line))
                                          stales++;
                                        auto it = s_submition_map.find(id);
                                        if (it != s_submition_map.end())
                                        {
                                          Serial.printf("Refuse submition %d\n", id);
                                          s_submition_map.erase(it);
                                        }
                                      }
                                      break;
          default:                    Serial.println("  Parsed JSON: unknown"); break;

      }
    }

    JobResult job_results[kJobResultsMax];
    size_t job_results_count = 0;
    vTaskDelay(50 / portTICK_PERIOD_MS); //Small delay

    
    if (job_pool != 0xFFFFFFFF)
    {
      #ifdef I2C_HASH_MASTER
      hashes += i2c_master_take_processed_hashes();
      #endif
      if (s_result_queue)
      {
        JobResult res{};
        while (job_results_count < kJobResultsMax && xQueueReceive(s_result_queue, &res, 0) == pdTRUE)
        {
          job_results[job_results_count++] = res;
        }
      }

      const uint32_t sw_nonce_count = calc_nonce_per_job_sw();
      JobData *job_data = s_current_job_data;
      if (job_data)
      {
        while (s_job_queue_sw && uxQueueMessagesWaiting(s_job_queue_sw) < kJobQueueTargetDepth)
        {
          if (!JobPush(s_job_queue_sw, job_data, nonce_pool, sw_nonce_count))
            break;
          nonce_pool += sw_nonce_count;
        }
        #ifdef HARDWARE_SHA265
        const uint32_t hw_nonce_count = calc_nonce_per_job_hw();
        while (s_job_queue_hw && uxQueueMessagesWaiting(s_job_queue_hw) < kJobQueueTargetDepth)
        {
          if (!JobPush(s_job_queue_hw, job_data, nonce_pool, hw_nonce_count))
            break;
          nonce_pool += hw_nonce_count;
        }
        #endif
      }
    }

    for (size_t r = 0; r < job_results_count; ++r)
    {
      JobResult *res = &job_results[r];

      hashes += res->nonce_count;
      if (res->difficulty > currentPoolDifficulty && job_pool == res->id && res->nonce != 0xFFFFFFFF)
      {
        if (!client.connected())
          break;
        unsigned long sumbit_id = 0;
        if (!tx_mining_submit(client, mWorker, mJob, res->nonce, sumbit_id))
        {
          client.stop();
          isMinerSuscribed = false;
          MiningJobStop(job_pool, s_submition_map);
          break;
        }
        Serial.print("   - Current diff share: "); Serial.println(res->difficulty,12);
        Serial.print("   - Current pool diff : "); Serial.println(currentPoolDifficulty,12);
        Serial.print("   - TX SHARE: ");
        for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", res->hash[i]);
        Serial.println("");
        mLastTXtoPool = millis();

        auto sub_ptr = std::make_shared<Submition>();
        sub_ptr->diff = res->difficulty;
        sub_ptr->is32bit = (res->hash[29] == 0 && res->hash[28] == 0);
        if (sub_ptr->is32bit)
        {
          sub_ptr->isValid = checkValid(res->hash, mMiner.bytearray_target);
        } else
          sub_ptr->isValid = false;

        if (s_submition_map.size() >= kMaxPendingSubmissions)
          s_submition_map.erase(s_submition_map.begin());
        s_submition_map[sumbit_id] = sub_ptr;
      }
    }
  }
}

//////////////////THREAD CALLS///////////////////

void minerWorkerSw(void * task_id)
{
  unsigned int miner_id = (uint32_t)task_id;
  Serial.printf("[MINER] %d Started minerWorkerSw Task!\n", miner_id);

  JobRequest job{};
  JobResult result{};
  uint8_t hash[32];
  uint32_t wdt_counter = 0;
  uint32_t smooth_counter = 0;
  while (1)
  {
    if (s_job_queue_sw && xQueueReceive(s_job_queue_sw, &job, 0) == pdTRUE)
    {
      result = {};
      result.difficulty = job.data->difficulty;
      result.nonce = 0xFFFFFFFF;
      result.id = job.data->id;
      result.nonce_count = job.nonce_count;

      const uint32_t start_us = micros();
      const uint32_t *midstate = job.data->midstate;
      const uint32_t *bake = job.data->bake;
      const uint32_t nonce_base = job.nonce_start;
      const uint32_t nonce_step = job.nonce_stride == 0 ? 1 : job.nonce_stride;
      const uint8_t job_in_work = job.data->id & 0xFF;
      uint32_t nonce = nonce_base;

      for (uint32_t n = 0; n < job.nonce_count; ++n)
      {
        if (nerd_sha256d_baked_nonce(midstate, nonce, bake, hash))
        {
          double diff_hash = diff_from_target(hash);
          if (diff_hash > result.difficulty)
          {
            result.difficulty = diff_hash;
            result.nonce = nonce;
            memcpy(result.hash, hash, 32);
          }
        }

        if ((uint16_t)(n & 0xFF) == 0 && s_working_current_job_id != job_in_work)
        {
          result.nonce_count = n + 1;
          break;
        }
        nonce += nonce_step;
      }

      update_hashrate(s_sw_hashes_per_ms, result.nonce_count, micros() - start_us);
      if (s_result_queue)
        xQueueSend(s_result_queue, &result, 0);
      apply_power_smoothing(MINER_POWER_SMOOTHING_SW_JOB_INTERVAL, smooth_counter);
    }
    else
    {
      vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    wdt_counter++;
    if (wdt_counter >= 8)
    {
      wdt_counter = 0;
      esp_task_wdt_reset();
    }
  }
}

#ifdef HARDWARE_SHA265

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)

static inline void nerd_sha_ll_fill_text_block_sha256(const void *input_text, uint32_t nonce)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    REG_WRITE(&reg_addr_buf[0], data_words[0]);
    REG_WRITE(&reg_addr_buf[1], data_words[1]);
    REG_WRITE(&reg_addr_buf[2], data_words[2]);
#if 0
    REG_WRITE(&reg_addr_buf[3], nonce);
    //REG_WRITE(&reg_addr_buf[3], data_words[3]);    
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
#else
    REG_WRITE(&reg_addr_buf[3], nonce);
    REG_WRITE(&reg_addr_buf[4], 0x00000080);
    REG_WRITE(&reg_addr_buf[5], 0x00000000);
    REG_WRITE(&reg_addr_buf[6], 0x00000000);
    REG_WRITE(&reg_addr_buf[7], 0x00000000);
    REG_WRITE(&reg_addr_buf[8], 0x00000000);
    REG_WRITE(&reg_addr_buf[9], 0x00000000);
    REG_WRITE(&reg_addr_buf[10], 0x00000000);
    REG_WRITE(&reg_addr_buf[11], 0x00000000);
    REG_WRITE(&reg_addr_buf[12], 0x00000000);
    REG_WRITE(&reg_addr_buf[13], 0x00000000);
    REG_WRITE(&reg_addr_buf[14], 0x00000000);
    REG_WRITE(&reg_addr_buf[15], 0x80020000);
#endif
}

static inline void nerd_sha_ll_fill_text_block_sha256_inter()
{
  uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

  DPORT_INTERRUPT_DISABLE();
  REG_WRITE(&reg_addr_buf[0], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4));
  REG_WRITE(&reg_addr_buf[1], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4));
  REG_WRITE(&reg_addr_buf[2], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4));
  REG_WRITE(&reg_addr_buf[3], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4));
  REG_WRITE(&reg_addr_buf[4], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4));
  REG_WRITE(&reg_addr_buf[5], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4));
  REG_WRITE(&reg_addr_buf[6], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4));
  REG_WRITE(&reg_addr_buf[7], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4));
  DPORT_INTERRUPT_RESTORE();

  REG_WRITE(&reg_addr_buf[8], 0x00000080);
  REG_WRITE(&reg_addr_buf[9], 0x00000000);
  REG_WRITE(&reg_addr_buf[10], 0x00000000);
  REG_WRITE(&reg_addr_buf[11], 0x00000000);
  REG_WRITE(&reg_addr_buf[12], 0x00000000);
  REG_WRITE(&reg_addr_buf[13], 0x00000000);
  REG_WRITE(&reg_addr_buf[14], 0x00000000);
  REG_WRITE(&reg_addr_buf[15], 0x00010000);
}

static inline void nerd_sha_ll_read_digest(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4);  
  ((uint32_t*)ptr)[7] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4);
  DPORT_INTERRUPT_RESTORE();
}


static inline bool nerd_sha_ll_read_digest_if(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  uint32_t last = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4);
  #if 1
  if ( (uint16_t)(last >> 16) != 0)
  {
    DPORT_INTERRUPT_RESTORE();
    return false;
  }
  #endif

  ((uint32_t*)ptr)[7] = last;
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4);  
  DPORT_INTERRUPT_RESTORE();
  return true;
}

static inline void nerd_sha_ll_write_digest(const void *digest_state)
{
    const uint32_t *digest_state_words = (const uint32_t *)digest_state;
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

static inline void nerd_sha_hal_wait_idle()
{
    while (REG_READ(SHA_BUSY_REG))
    {}
}

//#define VALIDATION
void minerWorkerHw(void * task_id)
{
  unsigned int miner_id = (uint32_t)task_id;
  Serial.printf("[MINER] %d Started minerWorkerHw Task!\n", miner_id);

  JobRequest job{};
  JobResult result{};
  uint8_t hash[32];
  uint32_t wdt_counter = 0;
  uint32_t smooth_counter = 0;

#ifdef VALIDATION
  uint8_t doubleHash[32];
  uint32_t diget_mid[8];
  uint32_t bake[16];
#endif

  while (1)
  {
    if (s_job_queue_hw && xQueueReceive(s_job_queue_hw, &job, 0) == pdTRUE)
    {
      result = {};
      result.id = job.data->id;
      result.nonce = 0xFFFFFFFF;
      result.nonce_count = job.nonce_count;
      result.difficulty = job.data->difficulty;

      const uint8_t *sha_buffer = job.data->sha_buffer_hw + 64;
      const uint32_t *hw_midstate = job.data->hw_midstate;
      const uint32_t nonce_base = job.nonce_start;
      const uint32_t nonce_step = job.nonce_stride == 0 ? 1 : job.nonce_stride;
      const uint8_t job_in_work = job.data->id & 0xFF;

      const uint32_t start_us = micros();
      esp_sha_acquire_hardware();
      REG_WRITE(SHA_MODE_REG, SHA2_256);
      uint32_t nonce = nonce_base;
      for (uint32_t i = 0; i < job.nonce_count; ++i)
      {
        //nerd_sha_hal_wait_idle();
        nerd_sha_ll_write_digest(hw_midstate);
        //nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256(sha_buffer, nonce);
        //sha_ll_continue_block(SHA2_256);
        REG_WRITE(SHA_CONTINUE_REG, 1);
        
        sha_ll_load(SHA2_256);
        nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256_inter();
        //sha_ll_start_block(SHA2_256);
        REG_WRITE(SHA_START_REG, 1);
        sha_ll_load(SHA2_256);
        nerd_sha_hal_wait_idle();
        if (nerd_sha_ll_read_digest_if(hash))
        {
          //Serial.printf("Hw 16bit Share, nonce=0x%X\n", nonce);
#ifdef VALIDATION
          //Validation
          nerd_sha256d_baked_nonce(job.data->midstate, nonce, job.data->bake, doubleHash);
          for (int i = 0; i < 32; ++i)
          {
            if (hash[i] != doubleHash[i])
            {
              Serial.println("***HW sha256 esp32s3 bug detected***");
              break;
            }
          }
#endif
          //~5 per second
          double diff_hash = diff_from_target(hash);
          if (diff_hash > result.difficulty)
          {
            if (isSha256Valid(hash))
            {
              result.difficulty = diff_hash;
              result.nonce = nonce;
              memcpy(result.hash, hash, sizeof(hash));
            }
          }
        }
        if (
             (uint8_t)(i & 0xFF) == 0 &&
             s_working_current_job_id != job_in_work)
        {
          result.nonce_count = i + 1;
          break;
        }
        nonce += nonce_step;
      }
      esp_sha_release_hardware();
      update_hashrate(s_hw_hashes_per_ms, result.nonce_count, micros() - start_us);
      if (s_result_queue)
        xQueueSend(s_result_queue, &result, 0);
      apply_power_smoothing(MINER_POWER_SMOOTHING_HW_JOB_INTERVAL, smooth_counter);
    }
    else
    {
      vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    wdt_counter++;
    if (wdt_counter >= 8)
    {
      wdt_counter = 0;
      esp_task_wdt_reset();
    }
  }
}

#endif  //#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)

#if defined(CONFIG_IDF_TARGET_ESP32)

static inline bool nerd_sha_ll_read_digest_swap_if(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  uint32_t fin = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4);
  if ( (uint32_t)(fin & 0xFFFF) != 0)
  {
    DPORT_INTERRUPT_RESTORE();
    return false;
  }
  ((uint32_t*)ptr)[7] = __builtin_bswap32(fin);
  ((uint32_t*)ptr)[0] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4));
  ((uint32_t*)ptr)[1] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4));
  ((uint32_t*)ptr)[2] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4));
  ((uint32_t*)ptr)[3] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4));
  ((uint32_t*)ptr)[4] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4));
  ((uint32_t*)ptr)[5] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4));
  ((uint32_t*)ptr)[6] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4));
  DPORT_INTERRUPT_RESTORE();
  return true;
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

static inline void nerd_sha_ll_fill_text_block_sha256_upper(const void *input_text, uint32_t nonce)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = data_words[0];
    reg_addr_buf[1]  = data_words[1];
    reg_addr_buf[2]  = data_words[2];
    reg_addr_buf[3]  = __builtin_bswap32(nonce);
#if 1
    reg_addr_buf[4]  = 0x80000000;
    reg_addr_buf[5]  = 0x00000000;
    reg_addr_buf[6]  = 0x00000000;
    reg_addr_buf[7]  = 0x00000000;
    reg_addr_buf[8]  = 0x00000000;
    reg_addr_buf[9]  = 0x00000000;
    reg_addr_buf[10] = 0x00000000;
    reg_addr_buf[11] = 0x00000000;
    reg_addr_buf[12] = 0x00000000;
    reg_addr_buf[13] = 0x00000000;
    reg_addr_buf[14] = 0x00000000;
    reg_addr_buf[15] = 0x00000280;
#else
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
#endif
}

static inline void nerd_sha_ll_fill_text_block_sha256_double()
{
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

void minerWorkerHw(void * task_id)
{
  unsigned int miner_id = (uint32_t)task_id;
  Serial.printf("[MINER] %d Started minerWorkerHwEsp32D Task!\n", miner_id);

  JobRequest job{};
  JobResult result{};
  uint8_t hash[32];
  uint32_t smooth_counter = 0;

  while (1)
  {
    if (s_job_queue_hw && xQueueReceive(s_job_queue_hw, &job, 0) == pdTRUE)
    {
      result = {};
      result.id = job.data->id;
      result.nonce = 0xFFFFFFFF;
      result.nonce_count = job.nonce_count;
      result.difficulty = job.data->difficulty;
      const uint8_t *sha_buffer = job.data->sha_buffer_hw;
      const uint8_t *sha_upper = sha_buffer + 64;
      uint32_t nonce = job.nonce_start;
      const uint32_t nonce_step = job.nonce_stride == 0 ? 1 : job.nonce_stride;
      const uint8_t job_in_work = job.data->id & 0xFF;

      const uint32_t start_us = micros();
      esp_sha_lock_engine(SHA2_256);
      for (uint32_t n = 0; n < job.nonce_count; ++n)
      {
        //((uint32_t*)(sha_buffer+64+12))[0] = __builtin_bswap32(nonce);

        //sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
        //nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256(sha_buffer);
        sha_ll_start_block(SHA2_256);

        //sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
        nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256_upper(sha_upper, nonce);
        sha_ll_continue_block(SHA2_256);

        nerd_sha_hal_wait_idle();
        sha_ll_load(SHA2_256);

        //sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
        nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256_double();
        sha_ll_start_block(SHA2_256);

        nerd_sha_hal_wait_idle();
        sha_ll_load(SHA2_256);
        if (nerd_sha_ll_read_digest_swap_if(hash))
        {
          //~5 per second
          double diff_hash = diff_from_target(hash);
          if (diff_hash > result.difficulty)
          {
            if (isSha256Valid(hash))
            {
              result.difficulty = diff_hash;
              result.nonce = nonce;
              memcpy(result.hash, hash, sizeof(hash));
            }
          }
        }
        if (
             (uint8_t)(n & 0xFF) == 0 &&
             s_working_current_job_id != job_in_work)
        {
          result.nonce_count = n + 1;
          break;
        }
        nonce += nonce_step;
      }
      esp_sha_unlock_engine(SHA2_256);
      update_hashrate(s_hw_hashes_per_ms, result.nonce_count, micros() - start_us);
      if (s_result_queue)
        xQueueSend(s_result_queue, &result, 0);
      apply_power_smoothing(MINER_POWER_SMOOTHING_HW_JOB_INTERVAL, smooth_counter);
    }
    else
    {
      vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    esp_task_wdt_reset();
  }
}

#endif  //CONFIG_IDF_TARGET_ESP32

#endif  //HARDWARE_SHA265


#define DELAY 100
#define REDRAW_EVERY 10

void restoreStat() {
  if(!Settings.saveStats) return;
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.printf("[MONITOR] NVS partition is full or has invalid version, erasing...\n");
    nvs_flash_init();
  }

  ret = nvs_open("state", NVS_READWRITE, &stat_handle);

  size_t required_size = sizeof(double);
  nvs_get_blob(stat_handle, "best_diff", &best_diff, &required_size);
  nvs_get_u32(stat_handle, "Mhashes", &Mhashes);
  uint32_t nv_shares, nv_valids;
  nvs_get_u32(stat_handle, "shares", &nv_shares);
  nvs_get_u32(stat_handle, "valids", &nv_valids);
  shares = nv_shares;
  valids = nv_valids;
  nvs_get_u32(stat_handle, "templates", &templates);
  nvs_get_u64(stat_handle, "upTime", &upTime);

  uint32_t crc = crc32_reset();
  crc = crc32_add(crc, &best_diff, sizeof(best_diff));
  crc = crc32_add(crc, &Mhashes, sizeof(Mhashes));
  crc = crc32_add(crc, &nv_shares, sizeof(nv_shares));
  crc = crc32_add(crc, &nv_valids, sizeof(nv_valids));
  crc = crc32_add(crc, &templates, sizeof(templates));
  crc = crc32_add(crc, &upTime, sizeof(upTime));
  crc = crc32_finish(crc);

  uint32_t nv_crc;
  nvs_get_u32(stat_handle, "crc32", &nv_crc);
  if (nv_crc != crc)
  {
    best_diff = 0.0;
    Mhashes = 0;
    shares = 0;
    valids = 0;
    templates = 0;
    upTime = 0;
  }
}

void saveStat() {
  if(!Settings.saveStats) return;
  Serial.printf("[MONITOR] Saving stats\n");
  nvs_set_blob(stat_handle, "best_diff", &best_diff, sizeof(best_diff));
  nvs_set_u32(stat_handle, "Mhashes", Mhashes);
  nvs_set_u32(stat_handle, "shares", shares);
  nvs_set_u32(stat_handle, "valids", valids);
  nvs_set_u32(stat_handle, "templates", templates);
  nvs_set_u64(stat_handle, "upTime", upTime);

  uint32_t crc = crc32_reset();
  crc = crc32_add(crc, &best_diff, sizeof(best_diff));
  crc = crc32_add(crc, &Mhashes, sizeof(Mhashes));
  uint32_t nv_shares = shares;
  uint32_t nv_valids = valids;
  crc = crc32_add(crc, &nv_shares, sizeof(nv_shares));
  crc = crc32_add(crc, &nv_valids, sizeof(nv_valids));
  crc = crc32_add(crc, &templates, sizeof(templates));
  crc = crc32_add(crc, &upTime, sizeof(upTime));
  crc = crc32_finish(crc);
  nvs_set_u32(stat_handle, "crc32", crc);
}

void resetStat() {
    Serial.printf("[MONITOR] Resetting NVS stats\n");
    templates = hashes = Mhashes = totalKHashes = elapsedKHs = upTime = shares = valids = stales = 0;
    best_diff = 0.0;
    saveStat();
}

void runMonitor(void *name)
{

  Serial.println("[MONITOR] started");
  restoreStat();

  unsigned long mLastCheck = 0;

  resetToFirstScreen();

  unsigned long frame = 0;

  uint32_t seconds_elapsed = 0;

  totalKHashes = (Mhashes * 1000) + hashes / 1000;
  uint32_t last_update_millis = millis();
  uint32_t uptime_frac = 0;

  while (1)
  {
    uint32_t now_millis = millis();
    if (now_millis < last_update_millis)
      now_millis = last_update_millis;
    
    uint32_t mElapsed = now_millis - mLastCheck;
    if (mElapsed >= 1000)
    { 
      mLastCheck = now_millis;
      last_update_millis = now_millis;
      unsigned long currentKHashes = (Mhashes * 1000) + hashes / 1000;
      elapsedKHs = currentKHashes - totalKHashes;
      totalKHashes = currentKHashes;

      uptime_frac += mElapsed;
      while (uptime_frac >= 1000)
      {
        uptime_frac -= 1000;
        upTime ++;
      }

      drawCurrentScreen(mElapsed);

      // Monitor state when hashrate is 0.0
      if (elapsedKHs == 0)
      {
        Serial.printf(">>> [i] Miner: newJob>%s / inRun>%s) - Client: connected>%s / subscribed>%s / wificonnected>%s\n",
            (s_working_current_job_id != 0xFF) ? "true" : "false",
            isMinerSuscribed ? "true" : "false",
            client.connected() ? "true" : "false", isMinerSuscribed ? "true" : "false", WiFi.status() == WL_CONNECTED ? "true" : "false");
      }

      #ifdef DEBUG_MEMORY
      Serial.printf("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
      Serial.printf("### Max stack usage: %d\n", uxTaskGetStackHighWaterMark(NULL));
      #endif

      seconds_elapsed++;

      if(seconds_elapsed % (saveIntervals[currentIntervalIndex]) == 0){
        saveStat();
        seconds_elapsed = 0;
        if(currentIntervalIndex < saveIntervalsSize - 1)
          currentIntervalIndex++;
      }    
    }
    animateCurrentScreen(frame);
    doLedStuff(frame);

    // Adapt target batch time based on stale shares
    static uint32_t last_stales = 0;
    static uint32_t last_adjust_ms = 0;
    static uint8_t stable_windows = 0;
    if (now_millis - last_adjust_ms >= kAdaptiveTargetWindowMs)
    {
      uint32_t current_stales = stales;
      uint32_t delta = current_stales - last_stales;
      last_stales = current_stales;
      last_adjust_ms = now_millis;

      if (delta > 0)
      {
        stable_windows = 0;
        if (s_adaptive_target_ms > kAdaptiveTargetMsMin)
        {
          uint32_t next = s_adaptive_target_ms - kAdaptiveTargetStepDown;
          if (next < kAdaptiveTargetMsMin)
            next = kAdaptiveTargetMsMin;
          s_adaptive_target_ms = next;
        }
      }
      else
      {
        stable_windows++;
        if (stable_windows >= 3 && s_adaptive_target_ms < kAdaptiveTargetMsMax)
        {
          uint32_t next = s_adaptive_target_ms + kAdaptiveTargetStepUp;
          if (next > kAdaptiveTargetMsMax)
            next = kAdaptiveTargetMsMax;
          s_adaptive_target_ms = next;
          stable_windows = 0;
        }
      }
    }
    vTaskDelay(DELAY / portTICK_PERIOD_MS);
    frame++;
  }
}
