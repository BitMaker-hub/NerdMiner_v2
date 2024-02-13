#include "display.h"
#include "../storage/storage.h"
#include "../storage/nvMemory.h"

// Variables to hold data from custom textboxes
//Track mining stats in non volatile memory
extern TSettings Settings;
extern nvMemory nvMem;

#ifdef NO_DISPLAY
DisplayDriver *currentDisplayDriver = &noDisplayDriver;
#endif

#ifdef LED_DISPLAY
DisplayDriver *currentDisplayDriver = &ledDisplayDriver;
#endif

#ifdef T_DISPLAY
DisplayDriver *currentDisplayDriver = &tDisplayDriver;
#endif

#ifdef AMOLED_DISPLAY
DisplayDriver *currentDisplayDriver = &amoledDisplayDriver;
#endif

#ifdef DONGLE_DISPLAY
DisplayDriver *currentDisplayDriver = &dongleDisplayDriver;
#endif

#ifdef ESP32_2432S028R
DisplayDriver *currentDisplayDriver = &esp32_2432S028RDriver;
#endif

#ifdef ESP32_2432S028_2USB
DisplayDriver *currentDisplayDriver = &esp32_2432S028RDriver;
#endif

#ifdef T_QT_DISPLAY
DisplayDriver *currentDisplayDriver = &t_qtDisplayDriver;
#endif

#ifdef V1_DISPLAY
DisplayDriver *currentDisplayDriver = &tDisplayV1Driver;
#endif

#ifdef M5STICKC_DISPLAY
DisplayDriver *currentDisplayDriver = &m5stickCDriver;
#endif


// Initialize the display
void initDisplay()
{
  Serial.println("Starting display.");
  currentDisplayDriver->initDisplay();
  if (!nvMem.loadConfig(&Settings)) {
    return;
  }

  if (Settings.screenOrientation>=0) {
    Serial.println("Setting stored screen orientation.");
    Serial.println(Settings.screenOrientation);
    currentDisplayDriver->setRotation(Settings.screenOrientation);
  } else {
    Serial.println("No stored screen orientation");
    Serial.println(Settings.screenOrientation);
    currentDisplayDriver->setRotation(0);
  }

    if (Settings.currentCyclicScreen>=0) {
    Serial.println("Setting stored screen.");
    Serial.println(Settings.currentCyclicScreen);
    currentDisplayDriver->current_cyclic_screen = Settings.currentCyclicScreen;
  } else {
    Serial.println("No stored screen.");
    Serial.println(Settings.currentCyclicScreen);  
  }

}

// Alternate screen state
void alternateScreenState()
{
  currentDisplayDriver->alternateScreenState();
}

// Alternate screen rotation
void alternateScreenRotation()
{
  int screen_rotation = currentDisplayDriver->alternateScreenRotation();
  Settings.screenOrientation = screen_rotation;
  nvMem.saveConfig(&Settings);
}

// Draw the loading screen
void drawLoadingScreen()
{
  currentDisplayDriver->loadingScreen();
}

// Draw the setup screen
void drawSetupScreen()
{
  currentDisplayDriver->setupScreen();
}

// Reset the current cyclic screen to the first one
void resetToFirstScreen()
{
  currentDisplayDriver->current_cyclic_screen = 0;
}

// Switches to the next cyclic screen without drawing it
void switchToNextScreen()
{
  currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
  Settings.currentCyclicScreen = currentDisplayDriver->current_cyclic_screen;
  nvMem.saveConfig(&Settings);
}

// Draw the current cyclic screen
void drawCurrentScreen(unsigned long mElapsed)
{
  currentDisplayDriver->cyclic_screens[currentDisplayDriver->current_cyclic_screen](mElapsed);
}

// Animate the current cyclic screen
void animateCurrentScreen(unsigned long frame)
{
  currentDisplayDriver->animateCurrentScreen(frame);
}

// Do LED stuff
void doLedStuff(unsigned long frame)
{
  currentDisplayDriver->doLedStuff(frame);
}
