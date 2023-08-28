#ifndef DISPLAY_H
#define DISPLAY_H

#include "drivers.h"

extern DisplayDriver *currentDisplayDriver;

void initDisplay();
void alternateScreenState();
void alternateScreenRotation();
void switchToNextScreen();
void resetToFirstScreen();
void drawLoadingScreen();
void drawSetupScreen();
void drawCurrentScreen(unsigned long mElapsed);

#endif // DISPLAY_H
