#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "storage.h"
#include "nvMemory.h"
#include "../devices/device.h"

// configuration example and description in /devices/esp32cam.h

// select interface and options according to provided pins
#if defined (SDMMC_D0) && defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3)
    #define BUILD_SDMMC_4
    #undef BUILD_SDMMC_1
    #undef BUILD_SDSPI
    #include <SD_MMC.h>
    #warning SD card support in 4-Bit mode enabled!
#elif defined (SDMMC_D0) && !(defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3))
    #define BUILD_SDMMC_1
    #undef BUILD_SDMMC_4
    #undef BUILD_SDSPI
    #include <SD_MMC.h>
    #warning SD card support in 1-Bit mode enabled!
#elif defined (SDSPI_CS)
    #undef BUILD_SDMMC_1
    #undef BUILD_SDMMC_4
    #define BUILD_SDSPI

    #include <SPI.h>
    #include <SD.h>

    #if !defined(NO_DISPLAY)
        #include "../lib/TFT_eSPI/User_Setup_Select.h"
    #endif

    #if !defined(NO_DISPLAY) && !defined(LED_DISPLAY)
        #if !defined(SDSPI_CLK) && defined(TFT_CLK)
            #define SDSPI_CLK TFT_CLK
        #endif // SDSPI_CLK
        #if !defined(SDSPI_MOSI) && defined(TFT_MOSI)
            #define SDSPI_MOSI TFT_MOSI
        #endif // SDSPI_MOSI
        #if !defined(SDSPI_MISO) && defined(TFT_MISO)
            #define SDSPI_MISO TFT_MISO
        #endif // SDSPI_MISO
    #elif !defined(SDSPI_CLK) || !defined(SDSPI_MOSI) || !defined(SDSPI_MISO)
        #error: Please define SDSPI pins!
    #endif // NO_DISPLAY or LED_DISPLAY
    
    #warning SD card support in SPI mode enabled!
#endif

class SDCard
{
public:
    SDCard(int ID=-1);
    ~SDCard();
    void SD2nvMemory(nvMemory* nvMem, TSettings* Settings);
    bool loadConfigFile(TSettings* Settings);
    bool cardAvailable();
    bool cardBusy();
private:
    bool initSDcard();
    bool cardInitialized_;
    bool cardBusy_;
#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4)
    fs::SDMMCFS* iSD_;
#elif defined (BUILD_SDSPI)
    SPIClass* ispi_;
    fs::SDFS* iSD_;
    bool newInstance_;
#endif
};

#endif // _SDCARD_H_
