#include "displayDriver.h"

#ifdef NO_DISPLAY

#include <Arduino.h>
#include "monitor.h"
#include "wManager.h"

extern monitor_data mMonitor;

void noDisplay_Init(void)
{
  Serial.println("No display driver initialized");
  pinMode(LED_PIN, OUTPUT);
}

void noDisplay_AlternateScreenState(void)
{
}

void noDisplay_AlternateRotation(void)
{
}

void noDisplay_NoScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  // Print hashrate to serial
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print extended data to serial for no display devices
  Serial.printf(">>> Valid blocks: %s\n", data.valids.c_str());
  Serial.printf(">>> Block templates: %s\n", data.templates.c_str());
  Serial.printf(">>> Best difficulty: %s\n", data.bestDiff.c_str());
  Serial.printf(">>> 32Bit shares: %s\n", data.completedShares.c_str());
  Serial.printf(">>> Temperature: %s\n", data.temp.c_str());
  Serial.printf(">>> Total MHashes: %s\n", data.totalMHashes.c_str());
  Serial.printf(">>> Time mining: %s\n", data.timeMining.c_str());
}
void noDisplay_LoadingScreen(void)
{
  Serial.println("Initializing...");
}

void noDisplay_SetupScreen(void)
{
  Serial.println("Setup...");
}

// Variables para controlar el parpadeo con millis()
unsigned long previousMillis = 0;

void noDisplay_DoLedStuff(unsigned long frame)
{
  unsigned long currentMillis = millis();

  switch (mMonitor.NerdStatus)
  {

  case NM_waitingConfig:
    digitalWrite(LED_PIN, HIGH); // LED encendido de forma continua
    break;

  case NM_Connecting:
    if (currentMillis - previousMillis >= 500)
    { // 0.5sec blink
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;

  case NM_hashing:
    if (currentMillis - previousMillis >= 100)
    { // 0.1sec blink
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Cambia el estado del LED
    }
    break;
  }
}

void noDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction noDisplayCyclicScreens[] = {noDisplay_NoScreen};

DisplayDriver noDisplayDriver = {
    noDisplay_Init,
    noDisplay_AlternateScreenState,
    noDisplay_AlternateRotation,
    noDisplay_LoadingScreen,
    noDisplay_SetupScreen,
    noDisplayCyclicScreens,
    noDisplay_AnimateCurrentScreen,
    noDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(noDisplayCyclicScreens),
    0,
    0,
    0,
};
#endif
