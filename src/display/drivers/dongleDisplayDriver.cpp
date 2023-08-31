#include "../drivers.h"

#ifdef DONGLE_DISPLAY

#include <TFT_eSPI.h>
#include "media/images.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"

#define WIDTH 160
#define HEIGHT 80

#define BUFFER_WIDTH WIDTH
#define BUFFER_HEIGHT HEIGHT * 4

#define SCROLL_SPEED 1
int pos_y = 0;
int delta_y = SCROLL_SPEED;
int max_y = BUFFER_HEIGHT - HEIGHT;

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);

#define BACK_COLOR TFT_BLACK
#define VALUE_COLOR TFT_WHITE
#define KEY_COLOR TFT_WHITE

#define CLEAR_SCREEN() \
    int32_t x = 4, y = 8; \
    background.setTextSize(1);\
    background.setTextFont(FONT2);\    
    background.setTextColor(KEY_COLOR);\    
    background.fillRect(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT, BACK_COLOR);\
    render.setFontSize(24);\

#define PRINT_STR(key, value, x, y)\
  {\
    background.drawString(String(key).c_str(), x, y);\
    y -= 8;\
    render.rdrawString(String(value).c_str(), WIDTH - 4, y, VALUE_COLOR);\
    y += 40;\
    max_y = y;\
  }

#define PUSH_SCREEN() \
    background.pushSprite(0,0);

void dongleDisplay_Init(void) {
    tft.init();
    tft.setRotation(3);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_RED);
    background.createSprite(BUFFER_WIDTH, BUFFER_HEIGHT);
    background.setSwapBytes(true);
    render.setDrawer(background);
    render.setLineSpaceRatio(0.9);

    // Load the font and check it can be read OK
    //if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
    if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers))){
        Serial.println("Initialise error");
        return;
    }  
}

void dongleDisplay_AlternateScreenState(void) {
}

void dongleDisplay_AlternateRotation(void) {
  tft.getRotation() == 1 ? tft.setRotation(3) : tft.setRotation(1);
}

void dongleDisplay_MinerScreen(unsigned long mElapsed) {
    mining_data data = getMiningData(mElapsed);

    //Print background screen
    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
    data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

    CLEAR_SCREEN();
    PRINT_STR("M.Hashes", data.totalMHashes, x, y)
    PRINT_STR("Templates", data.templates, x, y)
    PRINT_STR("Best Diff", data.bestDiff, x, y)
    PRINT_STR("Shares", data.completedShares, x, y)
    PRINT_STR("Hash rate", data.currentHashRate, x, y)    
    PRINT_STR("Valids", data.valids, x, y)
    PRINT_STR("Temp", data.temp, x, y)
    PRINT_STR("Time", data.currentTime, x, y)
}

void dongleDisplay_LoadingScreen(void) {
    CLEAR_SCREEN();
    PRINT_STR("Initializing...","", x, y);
    PUSH_SCREEN();
}

void dongleDisplay_SetupScreen(void) {
    CLEAR_SCREEN();
    PRINT_STR("Use WiFi for setup...","", x, y);
    PUSH_SCREEN();
}

void dongleDisplay_AnimateCurrentScreen(unsigned long frame) {
    if(pos_y >= max_y - HEIGHT) {
        delta_y = -SCROLL_SPEED;
        pos_y = max_y - HEIGHT;
    } else if(pos_y <= 0) {
        delta_y = SCROLL_SPEED;
        pos_y = 0;
    }
    pos_y += delta_y;
    background.pushSprite(0, -pos_y);    
}

CyclicScreenFunction dongleDisplayCyclicScreens[] = { dongleDisplay_MinerScreen };

DisplayDriver dongleDisplayDriver = { 
    dongleDisplay_Init,
    dongleDisplay_AlternateScreenState,
    dongleDisplay_AlternateRotation,
    dongleDisplay_LoadingScreen, 
    dongleDisplay_SetupScreen,
    dongleDisplayCyclicScreens,
    dongleDisplay_AnimateCurrentScreen, 
    SCREENS_ARRAY_SIZE(dongleDisplayCyclicScreens), 
    0,
    WIDTH,
    HEIGHT
};

#endif