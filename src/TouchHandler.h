#ifndef _TOUCHHANDLER_H_
#define _TOUCHHANDLER_H_
#ifdef TOUCH_ENABLE
#include <TFT_eSPI.h>  // TFT display library

#ifdef ESP32_2432S024R
// For ESP32-2432S024R use TFT_eSPI integrated touchscreen
// External function declaration for ESP32-2432S024R touch reading
extern bool esp32_2432S024R_getTouch(uint16_t *x, uint16_t *y);
#else
// For other devices use the liangyingy library
#include <xpt2046.h>   // https://github.com/liangyingy/arduino_xpt2046_library
#endif


class TouchHandler {
public:
  TouchHandler();
  ~TouchHandler();
  TouchHandler(TFT_eSPI& tft, uint8_t csPin, uint8_t irqPin, SPIClass& spi);
  void begin(uint16_t xres, uint16_t yres);
  uint16_t isTouched();
  void setScreenSwitchCallback(void (*callback)());
  void setScreenSwitchAltCallback(void (*callback)());
private:
  bool debounce();
  TFT_eSPI& tft;
#ifdef ESP32_2432S024R
  // Use external touchscreen function for ESP32-2432S024R
  // (no need for XPT2046 object here, it's handled in display driver)
#else
  XPT2046 touch;
#endif
  uint8_t csPin;
  uint8_t irqPin;
  SPIClass& spi;
  unsigned long lastTouchTime;
  // unsigned int lower_switch;
  void (*screenSwitchCallback)();
  void (*screenSwitchAltCallback)();
};
#endif

#endif