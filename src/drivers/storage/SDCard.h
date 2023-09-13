#ifndef _SDCARD_H_
#define _SDCARD_H_

#include <FS.h>
#include <SD_MMC.h>
#include <SD.h>

#include <ArduinoJson.h>

#include "..\drivers.h"
#include "storage.h"
#include "SPIStorage.h"

#define JSON_CONFIG_FILE "/config.json"

class SDCard
{
private:
    bool cardInitialized_;
public:
    SDCard()
    {
        cardInitialized_ = initSDcard();
    }

    ~SDCard()
    {
        if (cardInitialized_)
            SD_MMC.end();
    }

    bool initSDcard()
    {
        if (cardInitialized_)
            return cardInitialized_;

        bool oneBitMode = true;
#if defined (SDMMC_D0) && defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3)
        if (SD_MMC.cardType() == CARD_NONE)
            SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
        oneBitMode = false;
#elif defined (SDMMC_D0) && !(defined (SDMMC_D1) && defined (SDMMC_D2) && defined (SDMMC_D3))
        if (SD_MMC.cardType() == CARD_NONE)
            SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
#else
        Serial.println("SDCard: interface not available.");
        return false;
#endif // dataPinsDefined

        if ((!SD_MMC.begin("/sdcard", oneBitMode)) || (SD_MMC.cardType() == CARD_NONE))
        {
            Serial.println("SDCard: No card found.");
            return false;
        }
        return true;
    }

    TSettings loadConfigFile()
    {
        // Load existing configuration file
        // Read configuration from FS json
        Serial.println("SDCard: Mounting File System...");
        TSettings Settings;

        if (initSDcard())
        {
            Serial.println("SDCard: Mounted");
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
                    if (!error)
                    {
                        Serial.println("SDCard: Parsing JSON");
                        strcpy(Settings.WifiSSID, json["SSID"]);
                        strcpy(Settings.WifiPW, json["Password"]);
                        strcpy(Settings.PoolAddress, json["PoolURL"]);
                        strcpy(Settings.BtcWallet, json["WalletID"]);
                        Settings.PoolPort = json["Port"].as<int>();
                        Settings.Timezone = json["Timezone"].as<int>();
                        Settings.holdsData = true;
                    }
                    else
                    {
                        // Error loading JSON data
                        Serial.println("SDCard: Failed to load json config");
                    }
                }
                SD_MMC.end();
            }
        }
        else
        {
            // Error mounting file system
            Serial.println("SDCard: Failed to mount.");
        }
        return Settings;
    }

    void SD2SPIStorage(SPIStorage* spifs)
    {
        TSettings Settings = loadConfigFile();
        if (Settings.holdsData)
        {
            spifs->saveConfigFile(&Settings);
            WiFi.begin(Settings.WifiSSID, Settings.WifiPW);
            Serial.println("SDCard: Settings transfered to internal memory. Restarting now.");
            ESP.restart();
        }
    }
};

#endif // _SDCARD_H_
