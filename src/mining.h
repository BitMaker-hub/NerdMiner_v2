
// Mining
#define THREADS 1
#define MAX_NONCE       5000000U
#define TARGET_NONCE    471136297U

void runMonitor(void *name);
void runWorker(void *name);
void runMiner(void);
String printLocalTime(void);