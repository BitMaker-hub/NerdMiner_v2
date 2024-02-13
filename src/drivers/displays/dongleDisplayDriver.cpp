#include "displayDriver.h"

#ifdef DONGLE_DISPLAY

#include <TFT_eSPI.h>
#include "media/images_160_80.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"

#ifdef USE_LED
#include <FastLED.h>
#endif

#define WIDTH 160
#define HEIGHT 80

#define BUFFER_WIDTH MinerWidth
#define BUFFER_HEIGHT MinerHeight

#define SCROLL_SPEED 2
int pos_y = 0;
int delta_y = SCROLL_SPEED;
int max_y = BUFFER_HEIGHT - HEIGHT;

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);

#ifdef USE_LED
#define MAX_BRIGHTNESS 16
#define SLOW_FADE 1;
#define FAST_FADE 4;

CRGB leds(0, 0, 0);
int brightness = 0;
int fadeDirection = 1;
int fadeAmount = 0;
#endif // USE_LED

extern monitor_data mMonitor;

#define BACK_COLOR TFT_BLACK
#define VALUE_COLOR TFT_WHITE
#define KEY_COLOR TFT_WHITE

#define RESET_SCREEN()                  \
  int32_t x = 8, y = 8;                 \
  background.setTextSize(1);            \
  background.setTextFont(FONT2);        \
  \
    background.setTextColor(KEY_COLOR); \
  \
    render.setFontSize(18);

#define CLEAR_SCREEN()                                                \
  RESET_SCREEN();                                                     \
  background.fillRect(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT, BACK_COLOR); \
  \


#define PRINT_STR(str)                                                \
  {                                                                   \
    background.drawString(String(str).c_str(), x, y);                 \
    y += 32;                                                          \
    max_y = y;                                                        \
  }

#define PRINT_VALUE(value)                                       \
  {                                                              \
    render.drawString(String(value).c_str(), x, y, VALUE_COLOR); \
    y += 27;                                                     \
  }

#define PUSH_SCREEN() \
  background.pushSprite(0, 0);

void dongleDisplay_Init(void)
{
  #ifdef USE_LED
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
  FastLED.show();
  #endif // USE_LED

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  background.createSprite(BUFFER_WIDTH, BUFFER_HEIGHT);
  background.setSwapBytes(true);
  render.setDrawer(background);
  render.setLineSpaceRatio(0.9);

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }
}

void dongleDisplay_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

int dongleDisplay_AlternateRotation(void)
{
  int screen_rotation = tft.getRotation() == 1 ? 3 : 1;
  tft.setRotation(screen_rotation); 
  return screen_rotation;
}

void dongleDisplay_SetRotation(int rotation)
{
  tft.setRotation(rotation);
}

void dongleDisplay_MinerScreen(unsigned long mElapsed)
{
  max_y = BUFFER_HEIGHT;
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
                data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  RESET_SCREEN();
  x = 74;
  y = 15;
  render.setFontSize(20);
  PRINT_VALUE(data.currentTime);

  render.setFontSize(18);
  x = 68;
  y = 51;
  PRINT_VALUE(data.currentHashRate);
  PRINT_VALUE(data.valids);
  PRINT_VALUE(data.templates);
  PRINT_VALUE(data.bestDiff);
  PRINT_VALUE(data.completedShares);
  PRINT_VALUE(data.totalMHashes);
  PRINT_VALUE(data.temp);
}

void dongleDisplay_LoadingScreen(void)
{
  CLEAR_SCREEN();
  PRINT_STR("Initializing...");
  PUSH_SCREEN();
}

void dongleDisplay_SetupScreen(void)
{
  CLEAR_SCREEN();
  PRINT_STR("Use WiFi for setup...");
  PUSH_SCREEN();
}

void dongleDisplay_AnimateCurrentScreen(unsigned long frame)
{
  if (pos_y > max_y)
  {
    pos_y = 0;
  }
  if (pos_y > max_y - HEIGHT)
  {
    background.pushSprite(0, max_y - pos_y);
  }
  pos_y += delta_y;
  background.pushSprite(0, -pos_y);
}

void dongleDisplay_DoLedStuff(unsigned long frame)
{
#ifdef USE_LED
  if (digitalRead(TFT_BL))
  {    
    FastLED.clear(true);
    return;
  }

  switch (mMonitor.NerdStatus)
  {
  case NM_waitingConfig:
    brightness = MAX_BRIGHTNESS;
    leds.setRGB(255, 255, 0);
    fadeAmount = 0;
    break;

  case NM_Connecting:
    leds.setRGB(0, 0, 255);
    fadeAmount = SLOW_FADE;
    break;

  case NM_hashing:
    leds.setRGB(0, 0, 255);
    fadeAmount = FAST_FADE;
    break;
  }

  leds.fadeLightBy(0xFF - brightness);
  FastLED.show();

  brightness = brightness + (fadeDirection * fadeAmount);
  if (brightness <= 0 || brightness >= MAX_BRIGHTNESS)
  {
    fadeDirection = -fadeDirection;
  }
  brightness = constrain(brightness, 0, MAX_BRIGHTNESS);
#endif
}

CyclicScreenFunction dongleDisplayCyclicScreens[] = {dongleDisplay_MinerScreen};

DisplayDriver dongleDisplayDriver = {
    dongleDisplay_Init,
    dongleDisplay_AlternateScreenState,
    dongleDisplay_AlternateRotation,
    dongleDisplay_SetRotation,
    dongleDisplay_LoadingScreen,
    dongleDisplay_SetupScreen,
    dongleDisplayCyclicScreens,
    dongleDisplay_AnimateCurrentScreen,
    dongleDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(dongleDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};

#endif
