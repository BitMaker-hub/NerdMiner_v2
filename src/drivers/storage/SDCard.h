#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "..\drivers.h"
#include "storage.h"
#include "nvMemory.h"

class SDCard
{
public:
    SDCard();
    ~SDCard();
    void SD2nvMemory(nvMemory* nvMem);
    bool loadConfigFile(TSettings* Settings);
private:
    bool initSDcard();

    bool cardInitialized_;
};


#ifdef BUILD_SDMMC

#include <FS.h>
#include <SD_MMC.h>

#include <ArduinoJson.h>

SDCard::SDCard()
{
    cardInitialized_ = false;
}

SDCard::~SDCard()
{
    if (cardInitialized_)
        SD_MMC.end();
}

void SDCard::SD2nvMemory(nvMemory* nvMem)
{
    TSettings Settings;
    if (loadConfigFile(&Settings))
    {
        nvMem->saveConfig(&Settings);
        WiFi.begin(Settings.WifiSSID, Settings.WifiPW);
        Serial.println("SDCard: Settings transfered to internal memory. Restarting now.");
        ESP.restart();
    }
}

bool SDCard::loadConfigFile(TSettings* Settings)
{
    // Load existing configuration file
    // Read configuration from FS json
    Serial.println("SDCard: Mounting File System...");

    if (initSDcard())
    {
        if (SD_MMC.exists(JSON_CONFIG_FILE))
        {
            // The file exists, reading and loading
            Serial.println("SDCard: Reading config file");
            File configFile = SD_MMC.open(JSON_CONFIG_FILE, "r");
            if (configFile)
            {
                Serial.println("SDCard: Opened configuration file");
                StaticJsonDocument<512> json;
                DeserializationError error = deserializeJson(json, configFile);
                configFile.close();
                serializeJsonPretty(json, Serial);
                Serial.print('\n');
                if (!error)
                {
                    Serial.println("SDCard: Parsing JSON");
                    strcpy(Settings->WifiSSID, json[JSON_KEY_SSID] | Settings->WifiSSID);
                    strcpy(Settings->WifiPW, json[JSON_KEY_PASW] | Settings->WifiPW);
                    strcpy(Settings->PoolAddress, json[JSON_KEY_POOLURL] | Settings->PoolAddress);
                    strcpy(Settings->BtcWallet, json[JSON_KEY_WALLETID] | Settings->BtcWallet);
                    if (json.containsKey(JSON_KEY_POOLPORT))
                        Settings->PoolPort = json[JSON_KEY_POOLPORT].as<int>();
                    if (json.containsKey(JSON_KEY_TIMEZONE))
                        Settings->Timezone = json[JSON_KEY_TIMEZONE].as<int>();
                    SD_MMC.end();
                    return true;
                }
                else
                {
                    // Error loading JSON data
                    Serial.println("SDCard: Failed to load json config");
                }
            }
        }
        else
        {
            Serial.println("SDCard: No config file available!");        
        }
        SD_MMC.end();
    }
    return false;
}

bool SDCard::initSDcard()
{
    if((cardInitialized_)&&(SD_MMC.cardType() != CARD_NONE))
    {
        Serial.println("SDCard: Already mounted.");
        return cardInitialized_;
    }

    bool oneBitMode = true;
#if defined (SDMMC_D0) && defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3)
    if (SD_MMC.cardType() == CARD_NONE)
    {
        SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
        oneBitMode = false;
        Serial.println("SDCard: 4-Bit Mode.");
    }
#elif defined (SDMMC_D0) && !(defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3))
    if (SD_MMC.cardType() == CARD_NONE)
    {
        SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
        Serial.println("SDCard: 1-Bit Mode.");
    }
#else
    Serial.println("SDCard: interface not available.");
    return false;
#endif // dataPinsDefined
    cardInitialized_ = SD_MMC.begin("/sdcard", oneBitMode);
    if ((cardInitialized_) && (SD_MMC.cardType() != CARD_NONE))
    {
        Serial.println("SDCard: Card mounted."); 
        return true;
    }
    else
    {
        Serial.println("SDCard: No card found.");
        return false;
    }
}

#else

SDCard::SDCard() {}
SDCard::~SDCard() {}
void SDCard::SD2NVMemory(NVMemory* nvMem) {};
bool SDCard::loadConfigFile(TSettings* Settings) { return false; }
bool SDCard::initSDcard() { return false; }

#endif //BUILD_SDMMC


#endif // _SDCARD_H_
