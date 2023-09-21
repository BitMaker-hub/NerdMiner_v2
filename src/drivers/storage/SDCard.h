#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "storage.h"
#include "nvMemory.h"
#include "..\devices\device.h"

#if defined (SDMMC_D0) && defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3)
#define BUILD_SDMMC_4
#include <SD_MMC.h>
#elif defined (SDMMC_D0) && !(defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3))
#define BUILD_SDMMC_1
#include <SD_MMC.h>
#else
#warning SD card support disabled!
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
#error You chose to run the SD card in SPI mode. This is not implemented yet.
    fs::SDFS* iSD_;
#endif
};

#endif // _SDCARD_H_
