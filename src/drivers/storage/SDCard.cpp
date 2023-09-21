
#include <FS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "storage.h"
#include "nvMemory.h"
#include "..\devices\device.h"
#include  "SDCard.h"

#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4)

#include <SD_MMC.h>

SDCard::SDCard()
{
#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4)
    iSD_ = &SD_MMC;
#elif defined (BUILD_SDSPI)
#error You chose to run the sd card in SPI mode. This is not implemented yet.
#endif 
}

SDCard::~SDCard()
{
    unmount();
}

void SDCard::SD2nvMemory(nvMemory* nvMem, TSettings* Settings)
{
    if (loadConfigFile(Settings))
    {
        nvMem->saveConfig(Settings);
        WiFi.begin(Settings->WifiSSID, Settings->WifiPW);
        Serial.println("SDCard: Settings transfered to internal memory. Restarting now.");
        ESP.restart();
    }
}

bool SDCard::loadConfigFile(TSettings* Settings)
{
    // Load existing configuration file
    // Read configuration from FS json

    if (initSDcard())
    {
        if (iSD_->exists(JSON_CONFIG_FILE))
        {
            // The file exists, reading and loading
            File configFile = iSD_->open(JSON_CONFIG_FILE, "r");
            if (configFile)
            {
                StaticJsonDocument<512> json;
                DeserializationError error = deserializeJson(json, configFile);
                configFile.close();
                Serial.println("SDCard: Loading config file");
                serializeJsonPretty(json, Serial);
                Serial.print('\n');
                unmount();
                if (!error)
                {
                    Settings->WifiSSID = json[JSON_KEY_SSID] | Settings->WifiSSID;
                    Settings->WifiPW = json[JSON_KEY_PASW] | Settings->WifiPW;
                    Settings->PoolAddress = json[JSON_KEY_POOLURL] | Settings->PoolAddress;
                    strcpy(Settings->BtcWallet, json[JSON_KEY_WALLETID] | Settings->BtcWallet);
                    if (json.containsKey(JSON_KEY_POOLPORT))
                        Settings->PoolPort = json[JSON_KEY_POOLPORT].as<int>();
                    if (json.containsKey(JSON_KEY_TIMEZONE))
                        Settings->Timezone = json[JSON_KEY_TIMEZONE].as<int>();
                    if (json.containsKey(JSON_KEY_STATS2NV))
                        Settings->saveStats = json[JSON_KEY_STATS2NV].as<bool>();
                    return true;
                }
                else
                {
                    // Error loading JSON data
                    Serial.println("SDCard: Error parsing config file!");
                }
            }
            else
            {
                Serial.println("SDCard: Error opening config file!");
            }
        }
        else
        {
            Serial.println("SDCard: No config file available!");
        }
        unmount();
    }
    return false;
}

void SDCard::unmount()
{
    iSD_->end();
    Serial.println("SDCard: Unmounted");
}

bool SDCard::initSDcard()
{
    if (iSD_->cardType() != CARD_NONE)
    {
        Serial.println("SDCard: Already mounted.");
        return true;
    }
    Serial.println("SDCard: Mounting card.");

    bool cardInitialized = false;
#if defined (BUILD_SDMMC_4)
    if (iSD_->cardType() == CARD_NONE)
    {
        iSD_->setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
        Serial.println("SDCard: 4-Bit Mode.");
        cardInitialized = iSD_->begin("/sd", false);
    }
#elif defined (BUILD_SDMMC_1)
    #warning SDMMC : 1 - bit mode is not always working.If you experience issues, try other modes.
        if (iSD_->cardType() == CARD_NONE)
        {
            iSD_->setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
            Serial.println("SDCard: 1-Bit Mode.");
            cardInitialized = iSD_->begin("/sd", true);
        }
#elif defined (BUILD_SDSPI)
#error You chose to run the sd card in SPI mode. This is not implemented yet.
#else
    Serial.println("SDCard: interface not available.");
    return false;
#endif // dataPinsDefined
    if (cardInitialized)
    {
        if (iSD_->cardType() != CARD_NONE)
        {
            Serial.println("SDCard: Mounted.");
            return true;
        }
        else
        {
            Serial.println("SDCard: Mounting failed.");
            iSD_->end();
        }
    }
    return false;
}

#else

SDCard::SDCard() {}
SDCard::~SDCard() {}
void SDCard::SD2nvMemory(nvMemory* nvMem, TSettings* Settings) {};
bool SDCard::loadConfigFile(TSettings* Settings) { return false; }
bool SDCard::initSDcard() { return false; }
void unmount() {}

#endif //BUILD_SDMMC