#ifndef MONITOR_API_H
#define MONITOR_API_H

#include <Arduino.h>

// Monitor states
#define SCREEN_MINING   0
#define SCREEN_CLOCK    1
#define SCREEN_BLOCK    2

//Time update period
#define UPDATE_PERIOD_h   5

//API BTC price
#define getBTCAPI "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd"
#define UPDATE_BTC_min   5

//API BTC price
#define getHeightAPI "https://mempool.space/api/blocks/tip/height"
#define UPDATE_Height_min 5

typedef struct{
  uint8_t screen;
  bool rotation;

  //Data gotten from minerWork
/*  unsigned long templates;
  unsigned long hashes;
  unsigned long Mhashes;
  unsigned long totalKHashes;
  unsigned long halfshares; // increase if blockhash has 16 bits of zeroes
  unsigned int shares; // increase if blockhash has 32 bits of zeroes
  unsigned int valids; // increased if blockhash <= target
  */
}monitor_data;

void setup_monitor(void);
void show_MinerScreen(unsigned long mElapsed);
void show_ClockScreen(unsigned long mElapsed);
void changeScreen(void);

#endif //MONITOR_API_H