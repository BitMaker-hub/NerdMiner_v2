#include "displayDriver.h"

#ifdef M5_STICKS3

#include <Arduino.h>
#include <Wire.h>

// Reusing functions from tDisplayV1Driver.cpp, only the display
// initialization differs

extern void tDisplay_Init(void);
extern void tDisplay_AlternateScreenState(void);
extern void tDisplay_AlternateRotation(void);
extern void tDisplay_LoadingScreen(void);
extern void tDisplay_SetupScreen(void);
extern void tDisplay_AnimateCurrentScreen(unsigned long);
extern void tDisplay_DoLedStuff(unsigned long);
extern void tDisplay_MinerScreen(unsigned long);
extern void tDisplay_ClockScreen(unsigned long);
extern void tDisplay_GlobalHashScreen(unsigned long);
extern void tDisplay_BTCprice(unsigned long);

// M5PM1 PMIC (I2C 0x6E, SDA=47, SCL=48) gates the LCD power rail via PM1_G2.
// Without this sequence the ST7789P3 has no VCC and the screen stays black

static void m5pm1_rmw(uint8_t reg, uint8_t setBits, uint8_t clearBits)
{
    const uint8_t addr = 0x6E;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) return;
    uint8_t v = Wire.read();
    v = (v & ~clearBits) | setBits;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(v);
    Wire.endTransmission();
}

static void m5StickS3_enableLcdPower(void)
{
    Wire.begin(47, 48);
    const uint8_t G2 = 1 << 2;
    m5pm1_rmw(0x16, 0, G2);  // FUNC0: PM1_G2 -> GPIO function
    m5pm1_rmw(0x10, G2, 0);  // MODE:  PM1_G2 -> output
    m5pm1_rmw(0x13, 0, G2);  // DRV:   PM1_G2 -> push-pull
    m5pm1_rmw(0x11, G2, 0);  // OUT:   PM1_G2 -> high (LCD power on)
}

static void m5StickS3_Init(void)
{
    m5StickS3_enableLcdPower();
    tDisplay_Init();
}

static CyclicScreenFunction m5StickS3CyclicScreens[] = {
    tDisplay_MinerScreen,
    tDisplay_ClockScreen,
    tDisplay_GlobalHashScreen,
    tDisplay_BTCprice};

DisplayDriver m5StickS3Driver = {
    m5StickS3_Init,
    tDisplay_AlternateScreenState,
    tDisplay_AlternateRotation,
    tDisplay_LoadingScreen,
    tDisplay_SetupScreen,
    m5StickS3CyclicScreens,
    tDisplay_AnimateCurrentScreen,
    tDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(m5StickS3CyclicScreens),
    0,
    240,
    135};

#endif
