#ifndef _SPISTORAGE_H_
#define _SPISTORAGE_H_

#define ESP_DRD_USE_SPIFFS true

#include <WiFiManager.h>
#include <SPIFFS.h>
#include <FS.h>

#include <ArduinoJson.h>

#include "..\drivers.h"
#include "..\storage.h"

// JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

class SPIStorage
{
private:
    bool SPIFFSInitialized_;
public:
    SPIStorage()
    {
        SPIFFSInitialized_ = init();
    }

    ~SPIStorage()
    {
        SPIFFS.end();
    };

    void saveConfigFile(TSettings*Settings)
    {
        // Save Config in JSON format
        Serial.println(F("Saving configuration..."));

        // Create a JSON document
        StaticJsonDocument<512> json;
        json["poolString"] = Settings->PoolAddress;
        json["portNumber"] = Settings->PoolPort;
        json["btcString"] = Settings->BtcWallet;
        json["gmtZone"] = Settings->Timezone;

        // Open config file
        File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
        if (!configFile)
        {
            // Error, file did not open
            Serial.println("failed to open config file for writing");
        }

        // Serialize JSON data to write to file
        serializeJsonPretty(json, Serial);
        if (serializeJson(json, configFile) == 0)
        {
            // Error writing file
            Serial.println(F("Failed to write to file"));
        }
        // Close file
        configFile.close();
    }

    bool init()
    {
        if (SPIFFSInitialized_)
            return SPIFFSInitialized_;
        return SPIFFS.begin(false) || SPIFFS.begin(true);
    };

    TSettings loadConfigFile()
    {
        // Load existing configuration file
        // Uncomment if we need to format filesystem
        // SPIFFS.format();

        // Read configuration from FS json
        Serial.println("Mounting File System...");
        TSettings Settings;
        // May need to make it begin(true) first time you are using SPIFFS
        if ((SPIFFSInitialized_)||(init()))
        {
            Serial.println("mounted file system");
            if (SPIFFS.exists(JSON_CONFIG_FILE))
            {
                // The file exists, reading and loading
                Serial.println("reading config file");
                File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
                if (configFile)
                {
                    Serial.println("Opened configuration file");
                    StaticJsonDocument<512> json;
                    DeserializationError error = deserializeJson(json, configFile);
                    configFile.close();
                    serializeJsonPretty(json, Serial);
                    if (!error)
                    {
                        Serial.println("Parsing JSON");

                        strcpy(Settings.PoolAddress, json["poolString"]);
                        strcpy(Settings.BtcWallet, json["btcString"]);
                        Settings.PoolPort = json["portNumber"].as<int>();
                        Settings.Timezone = json["gmtZone"].as<int>();
                        Settings.holdsData = true;
                    }
                    else
                    {
                        // Error loading JSON data
                        Serial.println("Failed to load json config");
                    }
                }
            }
        }
        else
        {
            // Error mounting file system
            Serial.println("Failed to mount FS");
        }
        return Settings;
    }

    void deleteConfigFile()
    {
        Serial.println("Erasing config file..");       
        SPIFFS.remove(JSON_CONFIG_FILE); //Borramos fichero
    }
};

#endif // _SPISTORAGE_H_
