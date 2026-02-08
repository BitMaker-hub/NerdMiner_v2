#include "displayDriver.h"

#ifdef NO_DISPLAY

#include <Arduino.h>
#include "monitor.h"
#include "wManager.h"

extern monitor_data mMonitor;
bool ledOn = false;

void noDisplay_Init(void)
{
  Serial.println("No display driver initialized");
  pinMode(LED_PIN, OUTPUT);
}

void noDisplay_AlternateScreenState(void)
{
  Serial.println("Switching display state");
  ledOn = !ledOn;
}

void noDisplay_AlternateRotation(void)
{
}

void noDisplay_NoScreen(unsigned long mElapsed)
{
  static unsigned long lastPrint = 0;
  static unsigned long elapsedAccum = 0;
  static String lastShares;
  static String lastKHashes;
  static String lastHashrate;
  static String lastTemplates;
  static String lastBestDiff;
  static String lastStales;
  elapsedAccum += mElapsed;

  const unsigned long now = millis();
  if (now - lastPrint < 5000)
    return;
  lastPrint = now;

  mining_data data = getMiningData(elapsedAccum);
  elapsedAccum = 0;

  bool changed = false;
  if (data.completedShares != lastShares) changed = true;
  if (data.totalKHashes != lastKHashes) changed = true;
  if (data.currentHashRate != lastHashrate) changed = true;
  if (data.templates != lastTemplates) changed = true;
  if (data.bestDiff != lastBestDiff) changed = true;
  if (data.stales != lastStales) changed = true;

  // Always print at least every 60s, otherwise only on changes.
  if (!changed && (now - lastPrint < 60000))
    return;

  lastShares = data.completedShares;
  lastKHashes = data.totalKHashes;
  lastHashrate = data.currentHashRate;
  lastTemplates = data.templates;
  lastBestDiff = data.bestDiff;
  lastStales = data.stales;

  // Print hashrate to serial
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Print extended data to serial for no display devices
  Serial.printf(">>> Valid blocks: %s\n", data.valids.c_str());
  Serial.printf(">>> Block templates: %s\n", data.templates.c_str());
  Serial.printf(">>> Best difficulty: %s\n", data.bestDiff.c_str());
  Serial.printf(">>> 32Bit shares: %s\n", data.completedShares.c_str());
  Serial.printf(">>> Stales: %s\n", data.stales.c_str());
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

  if (!ledOn)
  {
    digitalWrite(LED_PIN, INACTIVE_LED);
    return;
  }

  switch (mMonitor.NerdStatus)
  {

  case NM_waitingConfig:
    digitalWrite(LED_PIN, ACTIVE_LED); // LED encendido de forma continua
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
