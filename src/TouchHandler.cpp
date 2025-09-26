
#include "drivers/devices/device.h"
#ifdef TOUCH_ENABLE
#include "TouchHandler.h"



TouchHandler::~TouchHandler() {
}

TouchHandler::TouchHandler(TFT_eSPI& tft, uint8_t csPin, uint8_t irqPin, SPIClass& spi)
  : tft(tft), csPin(csPin), irqPin(irqPin), spi(spi), lastTouchTime(0),
  screenSwitchCallback(nullptr), screenSwitchAltCallback(nullptr)
#ifndef ESP32_2432S024R
  , touch(spi, csPin, irqPin)
#endif
{

}

void TouchHandler::begin(uint16_t xres, uint16_t yres) {
#ifdef ESP32_2432S024R
    // For ESP32-2432S024R, touch is initialized in display driver
    // Nothing to do here
    Serial.println("[TouchHandler] Using ESP32-2432S024R external touch function");
#else
    spi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI);
    touch.begin(xres, yres);
#endif
}

void TouchHandler::setScreenSwitchCallback(void (*callback)()) {
  screenSwitchCallback = callback;
}

void TouchHandler::setScreenSwitchAltCallback(void (*callback)()) {
  screenSwitchAltCallback = callback;
}


uint16_t TouchHandler::isTouched() {
  // XXX - move touch_x, touch_y to private and min_x, min_y,max_x, max_y
  uint16_t touch_x, touch_y, code = 0;
  bool touched = false;

#ifdef ESP32_2432S024R
  // Use external touch function for ESP32-2432S024R
  touched = esp32_2432S024R_getTouch(&touch_x, &touch_y);
#else
  // Use original method for other devices
  if (touch.pressed()) {
    touched = true;
    touch_x = touch.RawX();
    touch_y = touch.RawY();
  }
#endif

  if (touched) {
    // Perform actions based on touch coordinates
    // if (y < y_min + (y_max - y_min) / 4) {
#ifdef ESP32_2432S024R
    // For ESP32-2432S024R, use screen coordinates directly
    if (touch_y < 240 / 4) {  // Bottom quarter of screen
#else
    // For other devices, use raw coordinates
    if (touch_x < 200 + (1700 - 200) / 4) {
#endif
      // bottom
      code = 1;
      if (debounce() && screenSwitchAltCallback) {
        screenSwitchAltCallback();
      }
    } else {
      // top
      code = 2;
      if (debounce() && screenSwitchCallback) {
        screenSwitchCallback();
      }
    }

    if (code) {
      if (code == 1)
        Serial.print("Touch bottom\n");
      else
        Serial.print("Touch top\n");
    }
  }
  return code;
}

bool TouchHandler::debounce() {
  unsigned long currentTime = millis();
  if (currentTime - lastTouchTime >= 2000) {
    lastTouchTime = currentTime;
    return true;
  }
  return false;
}
#endif