#include <FS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "storage.h"
#include "nvMemory.h"
#include "../devices/device.h"
#include  "SDCard.h"

#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4)
#include <SD_MMC.h>
#elif defined (BUILD_SDSPI)
#include <SD.h>
#include <SPI.h>
#endif // interface type

#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4) || defined (BUILD_SDSPI)

/// @param int ID only relevant in SPI mode, if you want to set up a custom SPI unit. Ignored in SD bus mode.
SDCard::SDCard(int ID):cardInitialized_(false),cardBusy_(false)
{
#if defined (BUILD_SDMMC_1) || defined(BUILD_SDMMC_4)
    iSD_ = &SD_MMC;
#elif defined (BUILD_SDSPI)
    if(ID>=0)
    {
        ispi_ = new SPIClass(ID);
        newInstance_ = true;
    }
    else
    {
        ispi_ = &SPI;
        newInstance_ = false;
    }
    iSD_ = &SD;
#endif // interface type
#ifndef SDMMC_1BIT_FIX
    initSDcard();
#endif
}

SDCard::~SDCard()
{
    iSD_->end();
#ifdef BUILD_SDSPI
    if(newInstance_) 
    {
        ispi_->end();
        delete ispi_;
    }
#endif // BUILD_SDSPI
    Serial.println("SDCard: Unmounted");  
}

/// @brief Check if the card is accessed right now.
/// @return true if active
bool SDCard::cardBusy()
{
    return cardBusy_;
}

/// @brief End the card to free heap space.
void SDCard::terminate()
{
    iSD_->end();
#ifdef BUILD_SDSPI
    ispi_->end(); 
#endif   
}

/// @brief Transfer settings from config file on a SD card to the device.
/// @param nvMemory* where to save
/// @param TSettings* passing a struct is required, to save memory
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

/// @brief Retreives settings from a config file on a SD card.
/// @param TSettings* Struct to update with new Settings
/// @return true on success
bool SDCard::loadConfigFile(TSettings* Settings)
{
    // Load existing configuration file
    // Read configuration from FS json

    if (cardAvailable())
    {
        if (iSD_->exists(JSON_CONFIG_FILE))
        {
            // The file exists, reading and loading
            File configFile = iSD_->open(JSON_CONFIG_FILE, "r");
            if (configFile)
            {
                cardBusy_ = true;
                StaticJsonDocument<512> json;
                DeserializationError error = deserializeJson(json, configFile);
                configFile.close();
                cardBusy_ = false;
                Serial.println("SDCard: Loading config file");
                if (!error)
                {
                    serializeJsonPretty(json, Serial);
                    Serial.print('\n');    
                    if (json.containsKey(JSON_KEY_SSID)) {                
                        Settings->WifiSSID = json[JSON_KEY_SSID] | Settings->WifiSSID;
                    }
                    if (json.containsKey(JSON_KEY_PASW)) {
                        Settings->WifiPW = json[JSON_KEY_PASW] | Settings->WifiPW;
                    }
                    Settings->PoolAddress = json[JSON_KEY_POOLURL] | Settings->PoolAddress;
                    strcpy(Settings->PoolPassword, json[JSON_KEY_POOLPASS] | Settings->PoolPassword);
                    strcpy(Settings->BtcWallet, json[JSON_KEY_WALLETID] | Settings->BtcWallet);
                    if (json.containsKey(JSON_KEY_POOLPORT))
                        Settings->PoolPort = json[JSON_KEY_POOLPORT].as<int>();
                    if (json.containsKey(JSON_KEY_TIMEZONE))
                        Settings->Timezone = json[JSON_KEY_TIMEZONE].as<int>();
                    if (json.containsKey(JSON_KEY_STATS2NV))
                        Settings->saveStats = json[JSON_KEY_STATS2NV].as<bool>();
                    if (json.containsKey(JSON_KEY_INVCOLOR)) {
                        Settings->invertColors = json[JSON_KEY_INVCOLOR].as<bool>();
                    } else {
                        Settings->invertColors = false;
                    }
                    // Serial.printf("Carteira Lida SD:%s\n", Settings.BtcWallet);       
                    Serial.printf("Carteira Lida SDs:%s\n", Settings->BtcWallet);                       
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
    }
    return false;
}

/// @brief Check if a SD card is inserted.
/// @return true if inserted.
bool SDCard::cardAvailable()
{
    if (cardInitialized_)
    {
        if (iSD_->cardType() != CARD_NONE)
        {
            Serial.println("SDCard: Inserted.");
            return true;
        }
        else
        {
            Serial.println("SDCard: Not inserted.");
        }
    }
    else
    {
        Serial.println("SDCard: Interface not initialized.");
    }
    return false;
}

/// @brief Init SD card interface. Normaly not required, as this is called by the constructor.
/// @return  true on success
bool SDCard::initSDcard()
{
    if (!cardAvailable()) 
    {
        Serial.println("SDCard: init SD card interface.");
#if defined (BUILD_SDMMC_4)
        iSD_->setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
        cardInitialized_ = iSD_->begin("/sd", false);
        Serial.println("SDCard: 4-Bit Mode.");
    }
#elif defined (BUILD_SDMMC_1)
#warning SDMMC : 1 - bit mode is not always working. If you experience issues, try other modes.
        iSD_->setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
#ifdef SD_FREQUENCY
        // Need to lower frequency to 20000 for proper detection
        cardInitialized_ = iSD_->begin("/sd", true, false, SD_FREQUENCY);
#else
        cardInitialized_ = iSD_->begin("/sd", true);
#endif
        Serial.println("SDCard: 1-Bit Mode.");
    }
#elif defined (BUILD_SDSPI)
        ispi_->begin(SDSPI_CLK, SDSPI_MISO, SDSPI_MOSI, SDSPI_CS);
        cardInitialized_ = iSD_->begin(SDSPI_CS, *ispi_);
        Serial.println("SDCard: SPI mode.");
    }
#else
    Serial.println("SDCard: interface not defined.");
    return false;
#endif // dataPinsDefined
    cardAvailable();
    return cardInitialized_;
}

#else
SDCard::SDCard(int ID) {}
SDCard::~SDCard() {}
void SDCard::SD2nvMemory(nvMemory* nvMem, TSettings* Settings) {};
bool SDCard::loadConfigFile(TSettings* Settings) { return false; }
bool SDCard::initSDcard() { return false; }
bool SDCard::cardAvailable() { return false; }
bool SDCard::cardBusy() { return false; }
void SDCard::terminate() {}
#endif //BUILD_SDMMC
