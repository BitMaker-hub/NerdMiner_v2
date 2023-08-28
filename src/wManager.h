
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
#define LED_PIN      2
#elif defined(NERMINER_S3_AMOLED)
#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 21
#define PIN_ENABLE5V 15

#endif

void init_WifiManager();
void wifiManagerProcess();
void reset_configurations();