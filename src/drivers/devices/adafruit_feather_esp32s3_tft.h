#ifndef _ADAFRUIT_FEATHER_ESP32S3_TFT_H
#define _ADAFRUIT_FEATHER_ESP32S3_TFT_H

#define PIN_BUTTON_1 0

/* Red LED
This little red LED, labeled #13 on the board,
is on or blinks during certain operations (such as pulsing when in the bootloader),
and is controllable in code. It is available in CircuitPython as board.LED,
and in Arduino as LED_BUILTIN or 13.
*/
// #define LED_PIN 13

#define V1_DISPLAY

#define PIN_ENABLE5V 21

/* NeoPixel LED
This addressable RGB NeoPixel LED, labeled Neo on the board,
works both as a status LED (in CircuitPython and the bootloader),
and can be controlled with code. It is available in CircuitPython as board.NEOPIXEL,
and in Arduino as PIN_NEOPIXEL or 33
*/
// #define RGB_LED_PIN 33
/*
There is a NeoPixel power pin that needs to be pulled high for the NeoPixel to work.
This is done automatically by CircuitPython and Arduino.
It is available in CircuitPython and Arduino as NEOPIXEL_POWER or 34
*/
// #define RGB_PWR_PIN 34

#endif