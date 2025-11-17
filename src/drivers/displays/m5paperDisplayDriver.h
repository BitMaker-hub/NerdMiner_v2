#ifndef M5PAPER_DISPLAY_DRIVER_H
#define M5PAPER_DISPLAY_DRIVER_H

#ifdef M5PAPER_DISPLAY

// Touch state structure for M5Paper GT911 touch controller
struct TouchState_M5Paper {
    bool isPressed;      // Currently touching
    bool justReleased;   // Just lifted finger
    int buttonNumber;    // Which button (0=none, 1=first, 2=second, etc.)
    int x, y;           // Touch coordinates
};

// M5Paper E-ink display driver functions
void m5paper_Init(void);
void m5paper_AlternateScreenState(void);
void m5paper_AlternateRotation(void);
void m5paper_LoadingScreen(void);
void m5paper_SetupScreen(void);
void m5paper_MinerScreen(unsigned long mElapsed);
void m5paper_ClockScreen(unsigned long mElapsed);
void m5paper_GlobalHashScreen(unsigned long mElapsed);
void m5paper_AnimateCurrentScreen(unsigned long frame);
void m5paper_DoLedStuff(unsigned long frame);

// Touch handling
TouchState_M5Paper m5paper_checkTouch(int currentScreen);

#endif // M5PAPER_DISPLAY

#endif // M5PAPER_DISPLAY_DRIVER_H
