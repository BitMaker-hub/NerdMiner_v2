#ifndef _SPISTORAGE_H_
#define _SPISTORAGE_H_

#define ESP_DRD_USE_SPIFFS true

#include <WiFiManager.h>
#include <SPIFFS.h>
#include <FS.h>

#include <ArduinoJson.h>

#include "..\drivers.h"
#include "storage.h"

class SPIStorage
{
private:
    bool SPIFFSInitialized_;
public:
    SPIStorage()
    {
        SPIFFSInitialized_ = false;
    }

    ~SPIStorage()
    {
        if (SPIFFSInitialized_)
            SPIFFS.end();
    };

    void saveConfigFile(TSettings*Settings)
    {
        if (init())
        {
            // Save Config in JSON format
            Serial.println(F("SPIFS: Saving configuration..."));

            // Create a JSON document
            StaticJsonDocument<512> json;
            json[JSON_KEY_POOLURL] = Settings->PoolAddress;
            json[JSON_KEY_POOLPORT] = Settings->PoolPort;
            json[JSON_KEY_WALLETID] = Settings->BtcWallet;
            json[JSON_KEY_TIMEZONE] = Settings->Timezone;
    
            // Open config file
            File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
            if (!configFile)
            {
                // Error, file did not open
                Serial.println("SPIFS: failed to open config file for writing");
            }

            // Serialize JSON data to write to file
            serializeJsonPretty(json, Serial);
            Serial.print('\n');
            if (serializeJson(json, configFile) == 0)
            {
                // Error writing file
                Serial.println(F("SPIFS: Failed to write to file"));
            }
            // Close file
            configFile.close();
        };
    }

    bool loadConfigFile(TSettings* Settings)
    {
        // Uncomment if we need to format filesystem
        // SPIFFS.format();

        // Load existing configuration file
        // Read configuration from FS json

        if (init())
        {
            if (SPIFFS.exists(JSON_CONFIG_FILE))
            {
                // The file exists, reading and loading
                Serial.println("SPIFS: Reading config file");
                File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
                if (configFile)
                {
                    Serial.println("SPIFS: Opened configuration file");
                    StaticJsonDocument<512> json;
                    DeserializationError error = deserializeJson(json, configFile);
                    configFile.close();
                    serializeJsonPretty(json, Serial);
                    Serial.print('\n');
                    if (!error)
                    {
                        Serial.println("SPIFS: Parsing JSON");
                        strcpy(Settings->PoolAddress, json[JSON_KEY_POOLURL] | Settings->PoolAddress);
                        strcpy(Settings->BtcWallet, json[JSON_KEY_WALLETID] | Settings->BtcWallet);
                        if(json.containsKey(JSON_KEY_POOLPORT))
                            Settings->PoolPort = json[JSON_KEY_POOLPORT].as<int>();
                        if (json.containsKey(JSON_KEY_TIMEZONE))
                            Settings->Timezone = json[JSON_KEY_TIMEZONE].as<int>();
                        return true;
                    }
                    else
                    {
                        // Error loading JSON data
                        Serial.println("SPIFS: Failed to load json config");
                    }
                }
            }
        }
        return false;
    }

    void deleteConfigFile()
    {
        Serial.println("SPIFS: Erasing config file..");       
        SPIFFS.remove(JSON_CONFIG_FILE); //Borramos fichero
    }
private:
    bool init()
    {
        if (!SPIFFSInitialized_) 
        {
            Serial.println("SPIFS: Mounting File System...");
            // May need to make it begin(true) first time you are using SPIFFS
            SPIFFSInitialized_ = SPIFFS.begin(false) || SPIFFS.begin(true);
            SPIFFSInitialized_ ? Serial.println("SPIFS: Mounted") : Serial.println("SPIFS: Mounting failed.");
        }
        else
        {
            Serial.println("SPIFS: Already Mounted");
        }
        return SPIFFSInitialized_;
    };
};

#endif // _SPISTORAGE_H_