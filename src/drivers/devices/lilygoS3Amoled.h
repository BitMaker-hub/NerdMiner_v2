#ifndef _LILYGO_S3_AMOLED_H
#define _LILYGO_S3_AMOLED_H

// The T-Display S3 AMOLED (RM67162 1.91") has a single physical button on GPIO0.
// GPIO21 is the CST816T touch IRQ, so it must NOT be configured as a button.
#define PIN_BUTTON_1 0

#define AMOLED_DISPLAY

#if TOUCH
#define TOUCH_ENABLE (1)
#endif

#endif