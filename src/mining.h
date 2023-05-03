
// Mining
#define THREADS 1
#define MAX_NONCE 3000000
// #define MAX_NONCE    1.215.752.192

// Pool
//#define POOL_URL "solo.ckpool.org" //"btc.zsolo.bid" "eu.stratum.slushpool.com"
//#define POOL_PORT 3333  //6057 //3333


void runMonitor(void *name);
void runWorker(void *name);
void runMiner(void);