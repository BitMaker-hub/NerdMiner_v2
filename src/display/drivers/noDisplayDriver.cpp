#include "../drivers.h"

#ifdef NO_DISPLAY

#include <Arduino.h>
#include "monitor.h"

void noDisplay_Init(void) {
    Serial.println("No display driver initialized");
}

void noDisplay_AlternateScreenState(void) {
}

void noDisplay_AlternateRotation(void) {
}

void noDisplay_NoScreen(unsigned long mElapsed) {
    mining_data data = getMiningData(mElapsed);

    //Print hashrate to serial
    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
    data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

    //Print extended data to serial for no display devices
    // Serial.printf(">>> Valid blocks: %s\n", data.valids.c_str());
    // Serial.printf(">>> Block templates: %s\n", data.templates.c_str());
    // Serial.printf(">>> Best difficulty: %s\n", data.bestDiff.c_str());
    // Serial.printf(">>> 32Bit shares: %s\n", data.completedShares.c_str());    
    // Serial.printf(">>> Temperature: %s\n", data.temp.c_str());
    // Serial.printf(">>> Total MHashes: %s\n", data.totalMHashes.c_str());
    // Serial.printf(">>> Time mining: %s\n", data.timeMining.c_str());
}
void noDisplay_LoadingScreen(void) {
    Serial.println("Initializing...");
 }
void noDisplay_SetupScreen(void) { 
    Serial.println("Setup...");
 }

CyclicScreenFunction noDisplayCyclicScreens[] = { noDisplay_NoScreen };

DisplayDriver noDisplayDriver = {
    noDisplay_Init,
    noDisplay_AlternateScreenState,
    noDisplay_AlternateRotation,
    noDisplay_LoadingScreen, 
    noDisplay_SetupScreen,
    noDisplayCyclicScreens, 
    SCREENS_ARRAY_SIZE(noDisplayCyclicScreens), 
    0, 
    0,
    0,
};
#endif