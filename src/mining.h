
#ifndef MINING_API_H
#define MINING_API_H

// Mining
#define MAX_NONCE       5000000U
#define TARGET_NONCE    471136297U
#define DEFAULT_DIFFICULTY  "1e-9"

#define TARGET_BUFFER_SIZE 64

void runMonitor(void *name);
void runStratumWorker(void *name);
void runMiner(void *name);
String printLocalTime(void);

typedef struct{
  uint8_t bytearray_target[32];
  uint8_t bytearray_pooltarget[32];
  uint8_t merkle_result[32];
  uint8_t bytearray_blockheader[80];
  float poolDifficulty;
  bool inRun;
  bool newJob;
}miner_data;


#endif // UTILS_API_H