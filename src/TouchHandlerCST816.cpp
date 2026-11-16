#include "TouchHandlerCST816.h"

// ---------------------------------------------------------------------------
// DLL527_FIXME note: this driver is self-contained and mirrors the register map
// used by SensorLib's TouchDrvCST816 (the driver the official
// Xinyuan-LilyGO/LilyGo-AMOLED-Series library uses on this board).
// ---------------------------------------------------------------------------

#define CST816_I2C_ADDR  0x15  // 7-bit I2C address
#define PIN_TOUCH_SDA    3     // I2C data
#define PIN_TOUCH_SCL    2     // I2C clock
#define PIN_TOUCH_IRQ    21    // active-low touch interrupt

// CST816T registers (SensorLib-compatible layout)
#define CST8xx_REG_STATUS        0x00 // gesture / status
#define CST8xx_REG_NUM_POINTS    0x02 // number of touch points (lower nibble)
#define CST8xx_REG_XPOS_HIGH     0x03 // X[11:8]
#define CST8xx_REG_XPOS_LOW      0x04 // X[7:0]
#define CST8xx_REG_YPOS_HIGH     0x05 // Y[11:8]
#define CST8xx_REG_YPOS_LOW      0x06 // Y[7:0]
#define CST8xx_REG_CHIP_ID       0xA7

#define CST816T_CHIP_ID_B5       0xB5
#define MAX_FINGER_NUM           1

// Gesture IDs delivered in CST8xx_REG_STATUS
#define GESTURE_NONE      0x00
#define GESTURE_SWIPE_UP  0x01
#define GESTURE_SWIPE_DOWN 0x02
#define GESTURE_SWIPE_LEFT 0x03
#define GESTURE_SWIPE_RIGHT 0x04
#define GESTURE_TAP       0x05
#define GESTURE_DOUBLE_TAP 0x06
#define GESTURE_LONG_PRESS 0x07

volatile bool TouchHandlerCST816::irqPending = false;

TouchHandlerCST816::TouchHandlerCST816()
    : xres(0), yres(0), lastTouchTime(0), screenSwitchCallback(nullptr), screenSwitchAltCallback(nullptr)
{
}

void TouchHandlerCST816::begin(uint16_t xres, uint16_t yres)
{
  this->xres = xres;
  this->yres = yres;

  pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);
  Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
  Wire.setClock(400000);
  delay(20);

  uint8_t chipId = i2cRead(CST8xx_REG_CHIP_ID);
  if (chipId == CST816T_CHIP_ID_B5)
  {
    Serial.printf("Touch CST816T detected (chip id 0x%02X)\n", chipId);
  }
  else
  {
    Serial.printf("Touch init: unexpected chip id 0x%02X (expected 0x%02X)\n",
                  chipId, CST816T_CHIP_ID_B5);
  }

  irqPending = false;
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_IRQ), irqHandler, FALLING);
}

void IRAM_ATTR TouchHandlerCST816::irqHandler()
{
  irqPending = true;
}

bool TouchHandlerCST816::debounce()
{
  unsigned long now = millis();
  if (now - lastTouchTime >= 300)
  {
    lastTouchTime = now;
    return true;
  }
  return false;
}

uint8_t TouchHandlerCST816::i2cRead(uint8_t reg)
{
  Wire.beginTransmission(CST816_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0)
    return 0xFF;
  if (Wire.requestFrom((uint8_t)CST816_I2C_ADDR, (uint8_t)1) != 1)
    return 0xFF;
  return Wire.read();
}

bool TouchHandlerCST816::i2cReadBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
  Wire.beginTransmission(CST816_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0)
    return false;
  if (Wire.requestFrom((uint8_t)CST816_I2C_ADDR, (uint8_t)len) < len)
    return false;
  for (uint8_t i = 0; i < len; i++)
    buf[i] = Wire.read();
  return true;
}

void TouchHandlerCST816::i2cWrite(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(CST816_I2C_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool TouchHandlerCST816::readTouch(uint16_t &x, uint16_t &y, uint8_t &gesture)
{
  uint8_t buf[7] = {0};
  if (!i2cReadBuf(CST8xx_REG_STATUS, buf, sizeof(buf)))
    return false;

  gesture = buf[0];

  uint8_t numPoints = buf[CST8xx_REG_NUM_POINTS] & 0x0F;
  if (numPoints == 0 || numPoints > MAX_FINGER_NUM)
    return false;

  x = ((buf[CST8xx_REG_XPOS_HIGH] & 0x0F) << 8) | buf[CST8xx_REG_XPOS_LOW];
  y = ((buf[CST8xx_REG_YPOS_HIGH] & 0x0F) << 8) | buf[CST8xx_REG_YPOS_LOW];

  return true;
}

uint16_t TouchHandlerCST816::isTouched()
{
  bool irqActive = digitalRead(PIN_TOUCH_IRQ) == LOW;
  if (!irqPending && !irqActive)
    return 0;

  irqPending = false;

  uint16_t x = 0, y = 0;
  uint8_t gesture = GESTURE_NONE;
  bool hasTouch = readTouch(x, y, gesture);
  if (!hasTouch)
    return 0;

  if (!debounce())
    return 0;

  if (gesture == GESTURE_SWIPE_LEFT || gesture == GESTURE_SWIPE_RIGHT)
  {
    Serial.println("Touch swipe horizontal: next screen");
    if (screenSwitchCallback)
      screenSwitchCallback();
    return 2;
  }

  if (gesture == GESTURE_SWIPE_UP || gesture == GESTURE_SWIPE_DOWN)
  {
    Serial.println("Touch swipe vertical: toggle screen state");
    if (screenSwitchAltCallback)
      screenSwitchAltCallback();
    return 1;
  }

  // Single tap (or raw touch with no gesture): split the panel in half.
  // Top half switches screens, bottom half toggles the display state.
  if (gesture == GESTURE_TAP || gesture == GESTURE_NONE || gesture == GESTURE_DOUBLE_TAP)
  {
    if (y < yres / 2)
    {
      Serial.println("Touch top-half tap: next screen");
      if (screenSwitchCallback)
        screenSwitchCallback();
      return 2;
    }
    else
    {
      Serial.println("Touch bottom-half tap: toggle screen state");
      if (screenSwitchAltCallback)
        screenSwitchAltCallback();
      return 1;
    }
  }

  return 0;
}

void TouchHandlerCST816::setScreenSwitchCallback(void (*callback)())
{
  screenSwitchCallback = callback;
}

void TouchHandlerCST816::setScreenSwitchAltCallback(void (*callback)())
{
  screenSwitchAltCallback = callback;
}