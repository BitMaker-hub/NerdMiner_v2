#ifndef _M5_PAPER_H
#define _M5_PAPER_H

// M5Paper v1.1 device configuration
// With E-ink display support

// Physical buttons on M5Paper
#ifndef PIN_BUTTON_1
#define PIN_BUTTON_1 38  // Center wheel button (click)
#endif

#ifndef PIN_BUTTON_2
#define PIN_BUTTON_2 39  // Wheel down - used for next screen and long press reset
#endif

#ifndef PIN_BUTTON_3
#define PIN_BUTTON_3 37  // Wheel up
#endif

// No LED on M5Paper, but define safe dummy GPIO to prevent errors
#ifndef LED_PIN
#define LED_PIN 2  // Safe unused GPIO pin (won't affect anything)
#endif

#ifndef ACTIVE_LED
#define ACTIVE_LED HIGH
#endif

#ifndef INACTIVE_LED
#define INACTIVE_LED LOW
#endif

// Enable E-ink display
#define M5PAPER_DISPLAY

#endif // _M5_PAPER_H
