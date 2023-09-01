#ifndef DRIVERS_H
#define DRIVERS_H

#if defined(NERDMINERV2)
#include "devices/nerdMinerV2.h"
#elif defined(DEVKITV1)
#include "devices/esp32DevKit.h"
#elif defined(TDISPLAY)
#include "devices/lilygoS3TDisplay.h"
#elif defined(NERMINER_S3_AMOLED)
#include "devices/lilygoS3Amoled.h"
#elif defined(NERMINER_S3_DONGLE)
#include "devices/lilygoS3Dongle.h"
#else
#error "No device defined"
#endif

typedef void (*AlternateFunction)(void);
typedef void (*DriverInitFunction)(void);
typedef void (*ScreenFunction)(void);
typedef void (*CyclicScreenFunction)(unsigned long mElapsed);
typedef void (*AnimateCurrentScreenFunction)(unsigned long frame);
typedef void (*DoLedStuff)(unsigned long frame);

typedef struct
{
  DriverInitFunction initDisplay;                    // Initialize the display
  AlternateFunction alternateScreenState;            // Alternate screen state
  AlternateFunction alternateScreenRotation;         // Alternate screen rotation
  ScreenFunction loadingScreen;                      // Explicit loading screen
  ScreenFunction setupScreen;                        // Explicit setup screen
  CyclicScreenFunction *cyclic_screens;              // Array of cyclic screens
  AnimateCurrentScreenFunction animateCurrentScreen; // Animate the current cyclic screen
  DoLedStuff doLedStuff;                             // Do LED stuff
  int num_cyclic_screens;                            // Number of cyclic screens
  int current_cyclic_screen;                         // Current cyclic screen being displayed
  int screenWidth;                                   // Screen width
  int screenHeight;                                  // Screen height
} DisplayDriver;

extern DisplayDriver *currentDisplayDriver;

extern DisplayDriver noDisplayDriver;
extern DisplayDriver tDisplayDriver;
extern DisplayDriver amoledDisplayDriver;
extern DisplayDriver dongleDisplayDriver;

#define SCREENS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif // DRIVERS_H
