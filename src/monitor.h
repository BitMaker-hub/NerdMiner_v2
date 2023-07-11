#ifndef MONITOR_API_H
#define MONITOR_API_H

#include <Arduino.h>

// Monitor states
#define SCREEN_MINING   0
#define SCREEN_CLOCK    1
#define SCREEN_GLOBAL   2

//Time update period
#define UPDATE_PERIOD_h   5

//API BTC price
#define getBTCAPI "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd"
#define UPDATE_BTC_min   5

//API Block height
#define getHeightAPI "https://mempool.space/api/blocks/tip/height"
#define UPDATE_Height_min 2

//APIs Global Stats
#define getGlobalHash "https://mempool.space/api/v1/mining/hashrate/3d"
#define getDifficulty "https://mempool.space/api/v1/difficulty-adjustment"
#define getFees "https://mempool.space/api/v1/fees/recommended"
#define UPDATE_Global_min 2

#define NEXT_HALVING_EVENT 840000
#define HALVING_BLOCKS 210000

typedef struct{
  uint8_t screen;
  bool rotation;
}monitor_data;

typedef struct{
  String globalHash; //hexahashes
  String currentBlock;
  String difficulty;
  String blocksHalving;
  float progressPercent;
  int remainingBlocks;
  int halfHourFee;
}global_data;

void setup_monitor(void);
void show_MinerScreen(unsigned long mElapsed);
void show_ClockScreen(unsigned long mElapsed);
void show_GlobalHashScreen(unsigned long mElapsed);
void changeScreen(void);

#endif //MONITOR_API_H