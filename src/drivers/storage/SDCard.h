#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "storage.h"
#include "nvMemory.h"
#include "..\devices\device.h"

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
#if defined (SDSPI_CLK) && defined (SDSPI_MOSI) && defined (SDSPI_MISO) && defined (NO_DISPLAY)
#define BUILD_SDSPI_SETUP
#endif // SPIPINS
#include <SPI.h>
#include <SD.h>
#warning SD card support in SPI mode enabled!
#endif

// Handles the transfer of settings from sd card to nv memory (wifi credentials are handled by wifimanager)
class SDCard
{
public:
    SDCard();
    ~SDCard();
    void SD2nvMemory(nvMemory* nvMem, TSettings* Settings);
    bool loadConfigFile(TSettings* Settings);
private:
    bool initSDcard();
    void unmount();

#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4)
    fs::SDMMCFS* iSD_;
#elif defined (BUILD_SDSPI)
    SPIClass* ispi_;
    fs::SDFS* iSD_;
#endif
};

#endif // _SDCARD_H_
