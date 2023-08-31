#include "../drivers.h"

#ifdef AMOLED_DISPLAY

#include <rm67162.h>
#include <TFT_eSPI.h>
#include "media/images.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"

#define WIDTH 536
#define HEIGHT 240

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite background = TFT_eSprite(&tft);

void amoledDisplay_Init(void) {
    rm67162_init();
    lcd_setRotation(1);

    background.createSprite(WIDTH, HEIGHT);
    background.setSwapBytes(true);
    render.setDrawer(background);
    render.setLineSpaceRatio(0.9);

    if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers))){
        Serial.println("Initialise error");
        return;    
    }
}

int screen_state= 1;
void amoledDisplay_AlternateScreenState(void) {
    screen_state == 1 ? lcd_off() : lcd_on();
    screen_state ^= 1;
}

int screen_rotation = 1;
void amoledDisplay_AlternateRotation(void) {
    screen_rotation == 1 ? lcd_setRotation(3) : lcd_setRotation(1);
    screen_rotation ^= 1; 
}

void amoledDisplay_MinerScreen(unsigned long mElapsed) {
    mining_data data = getMiningData(mElapsed);

    //Print background screen
    background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen); 

    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
    data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

    //Hashrate
    render.setFontSize(35);
    render.setCursor(19, 118);
    render.setFontColor(TFT_BLACK);
    
    render.rdrawString(data.currentHashRate.c_str(), 118, 114, TFT_BLACK);
    //Total hashes
    render.setFontSize(18);
    render.rdrawString(data.totalMHashes.c_str(), 268, 138, TFT_BLACK);
    //Block templates
    render.setFontSize(18);
    render.drawString(data.templates.c_str(), 186, 20, 0xDEDB);
    //Best diff
    render.drawString(data.bestDiff.c_str(), 186, 48, 0xDEDB);
    //32Bit shares
    render.setFontSize(18);
    render.drawString(data.completedShares.c_str(), 186, 76, 0xDEDB);
    //Hores
    render.setFontSize(14);
    render.rdrawString(data.timeMining.c_str(), 315, 104, 0xDEDB);

    //Valid Blocks
    render.setFontSize(24);
    render.drawString(data.valids.c_str(), 285, 56, 0xDEDB);

    //Print Temp
    render.setFontSize(10);
    render.rdrawString(data.temp.c_str(), 239, 1, TFT_BLACK);

    render.setFontSize(4);
    render.rdrawString(String(0).c_str(), 244, 3, TFT_BLACK);

    //Print Hour
    render.setFontSize(10);
    render.rdrawString(data.currentTime.c_str(), 286, 1, TFT_BLACK);

    //Push prepared background to screen
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_ClockScreen(unsigned long mElapsed) {
    clock_data data = getClockData(mElapsed);

    //Print background screen
    background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen); 

    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
    data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

    //Hashrate
    render.setFontSize(25);
    render.setCursor(19, 122);
    render.setFontColor(TFT_BLACK);
    render.rdrawString(data.currentHashRate.c_str(), 94, 129, TFT_BLACK);

    //Print BTC Price
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.btcPrice.c_str(), 202, 3, GFXFF);

    //Print BlockHeight
    render.setFontSize(18);
    render.rdrawString(data.blockHeight.c_str(), 254, 140, TFT_BLACK);

    //Print Hour
    background.setFreeFont(FF23);
    background.setTextSize(2);
    background.setTextColor(0xDEDB, TFT_BLACK);
    
    background.drawString(data.currentTime.c_str(), 130, 50, GFXFF);

    //Push prepared background to screen
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_GlobalHashScreen(unsigned long mElapsed) {
    coin_data data = getCoinData(mElapsed);

    //Print background screen
    background.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen); 

    Serial.printf(">>> Completed %s share(s), %s Khashes, avg. hashrate %s KH/s\n",
    data.completedShares.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

    //Print BTC Price
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.btcPrice.c_str(), 198, 3, GFXFF);

    //Print Hour
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.currentTime.c_str(), 268, 3, GFXFF);

    //Print Last Pool Block
    background.setFreeFont(FSS9);
    background.setTextDatum(TR_DATUM);
    background.setTextColor(0x9C92);
    background.drawString(data.halfHourFee.c_str(), 302, 52, GFXFF);

    //Print Difficulty
    background.setFreeFont(FSS9);
    background.setTextDatum(TR_DATUM);
    background.setTextColor(0x9C92);
    background.drawString(data.netwrokDifficulty.c_str(), 302, 88, GFXFF);

    //Print Global Hashrate
    render.setFontSize(17);
    render.rdrawString(data.globalHashRate.c_str(), 274, 145, TFT_BLACK);

    //Print BlockHeight
    render.setFontSize(28);
    render.rdrawString(data.blockHeight.c_str(), 140, 104, 0xDEDB);

    //Draw percentage rectangle
    int x2 = 2 + (138*data.progressPercent/100);
    background.fillRect(2, 149, x2, 168, 0xDEDB);

    //Print Remaining BLocks
    background.setTextFont(FONT2);
    background.setTextSize(1);
    background.setTextDatum(MC_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(data.remainingBlocks.c_str(), 72, 159, FONT2);

    //Push prepared background to screen
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_LoadingScreen(void) {
    background.fillScreen(TFT_BLACK);
    background.pushImage(0, 0, initWidth, initHeight, initScreen);
    background.setTextColor(TFT_BLACK);
    background.drawString(CURRENT_VERSION, 24, 147, FONT2);

    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_SetupScreen(void) {
    background.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);

    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)background.getPointer());
}

void amoledDisplay_AnimateCurrentScreen(unsigned long frame) {
}

CyclicScreenFunction amoledDisplayCyclicScreens[] = { amoledDisplay_MinerScreen, amoledDisplay_ClockScreen, amoledDisplay_GlobalHashScreen };

DisplayDriver amoledDisplayDriver = { 
    amoledDisplay_Init,
    amoledDisplay_AlternateScreenState,
    amoledDisplay_AlternateRotation,
    amoledDisplay_LoadingScreen, 
    amoledDisplay_SetupScreen,
    amoledDisplayCyclicScreens, 
    amoledDisplay_AnimateCurrentScreen,
    SCREENS_ARRAY_SIZE(amoledDisplayCyclicScreens), 
    0,
    WIDTH,
    HEIGHT
};
#endif