#ifndef DRIVERS_H
#define DRIVERS_H

#if defined(DEVKITV1)
    #define NO_DISPLAY
#elif defined(NERMINER_S3_AMOLED)
    #define AMOLED_DISPLAY
#elif defined(NERMINER_S3_DONGLE)    
    #define DONGLE_DISPLAY
#else
    #define T_DISPLAY
#endif

typedef void (*AlternateFunction)(void);
typedef void (*DriverInitFunction)(void);
typedef void (*ScreenFunction)(void);
typedef void (*CyclicScreenFunction)(unsigned long mElapsed);
typedef void (*AnimateCurrentScreenFunction)(unsigned long frame);

typedef struct {
    DriverInitFunction initDisplay;             // Initialize the display
    AlternateFunction alternateScreenState;     // Alternate screen state
    AlternateFunction alternateScreenRotation;  // Alternate screen rotation
    ScreenFunction loadingScreen;               // Explicit loading screen
    ScreenFunction setupScreen;                 // Explicit setup screen
    CyclicScreenFunction *cyclic_screens;       // Array of cyclic screens
    AnimateCurrentScreenFunction animateCurrentScreen; // Animate the current cyclic screen
    int num_cyclic_screens;                     // Number of cyclic screens
    int current_cyclic_screen;                  // Current cyclic screen being displayed
    int screenWidth;                            // Screen width
    int screenHeight;                           // Screen height
} DisplayDriver;

extern DisplayDriver *currentDisplayDriver;

extern DisplayDriver noDisplayDriver;
extern DisplayDriver tDisplayDriver;
extern DisplayDriver amoledDisplayDriver;
extern DisplayDriver dongleDisplayDriver;

#define SCREENS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif // DRIVERS_H
