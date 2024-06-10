#ifndef _TOUCHHANDLER_H_
#define _TOUCHHANDLER_H_
#ifdef TOUCH_ENABLE
#include <TFT_eSPI.h>  // TFT display library
#include <xpt2046.h>   // https://github.com/liangyingy/arduino_xpt2046_library


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
  XPT2046 touch;
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