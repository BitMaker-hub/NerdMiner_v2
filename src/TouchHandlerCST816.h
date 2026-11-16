#ifndef _TOUCHHANDLERCST816_H_
#define _TOUCHHANDLERCST816_H_

#include <Arduino.h>
#include <Wire.h>

// Touch handler for the LilyGO T-Display S3 AMOLED (CST816T over I2C).
//   Touch controller : CST816T  (I2C address 0x15)
//   Wiring           : SDA = GPIO 3, SCL = GPIO 2, IRQ = GPIO 21
// The IRQ pin (GPIO 21) is an active-low open-drain output; it is NOT a button.
class TouchHandlerCST816
{
public:
  TouchHandlerCST816();

  void begin(uint16_t xres, uint16_t yres);

  // Polls the touch controller. Invokes the registered callbacks and returns:
  //   1 = screenSwitchAltCallback() was fired
  //   2 = screenSwitchCallback() was fired
  //   0 = no action
  uint16_t isTouched();

  void setScreenSwitchCallback(void (*callback)());
  void setScreenSwitchAltCallback(void (*callback)());

  // ISR entry point: raises the pending flag on the touch IRQ edge
  static void IRAM_ATTR irqHandler();

private:
  bool debounce();
  bool readTouch(uint16_t &x, uint16_t &y, uint8_t &gesture);
  uint8_t i2cRead(uint8_t reg);
  bool i2cReadBuf(uint8_t reg, uint8_t *buf, uint8_t len);
  void i2cWrite(uint8_t reg, uint8_t val);

  uint16_t xres;
  uint16_t yres;
  unsigned long lastTouchTime;
  void (*screenSwitchCallback)();
  void (*screenSwitchAltCallback)();
  static volatile bool irqPending;
};

#endif // _TOUCHHANDLERCST816_H_