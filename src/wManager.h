
#ifdef NERDMINERV2
//Define config buttons for TTGO-TDisplay-s3
#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 14
#define PIN_ENABLE5V 15
#elif defined(DEVKITV1)
//Standard ESP32-devKit
#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 19  //Not used
#define PIN_ENABLE5V 21  //Not used
#endif

void init_WifiManager();
void wifiManagerProcess();
void reset_configurations();