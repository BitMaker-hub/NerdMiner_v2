#ifndef DISPLAYDRIVER_H_
#define DISPLAYDRIVER_H_

#include "../devices/device.h"

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

extern DisplayDriver m5stackDisplayDriver;
extern DisplayDriver wt32DisplayDriver;
extern DisplayDriver noDisplayDriver;
extern DisplayDriver ledDisplayDriver;
extern DisplayDriver tDisplayDriver;
extern DisplayDriver amoledDisplayDriver;
extern DisplayDriver dongleDisplayDriver;
extern DisplayDriver esp32_2432S028RDriver;
extern DisplayDriver t_qtDisplayDriver;
extern DisplayDriver tDisplayV1Driver;
extern DisplayDriver m5stickCDriver;
extern DisplayDriver t_hmiDisplayDriver;
extern DisplayDriver sp_kcDisplayDriver;

#define SCREENS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif // DISPLAYDRIVER_H_
