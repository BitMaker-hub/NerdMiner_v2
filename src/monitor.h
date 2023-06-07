#ifndef MONITOR_API_H
#define MONITOR_API_H

#include <Arduino.h>

// Monitor states
#define SCREEN_MINING   0
#define SCREEN_CLOCK    1
#define SCREEN_BLOCK    2

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

void show_MinerScreen(unsigned long mElapsed);

#endif //MONITOR_API_H