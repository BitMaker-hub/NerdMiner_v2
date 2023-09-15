#ifndef _NVMEMORY_H_
#define _NVMEMORY_H_

// we only have one implementation right now and nothing to choose from.
#define NVMEM_SPIFFS

#include "..\devices\device.h"
#include "storage.h"

// Handles load and store of user settings, except wifi credentials. Those are managed by the wifimanager.
class nvMemory
{
public: 
    nvMemory();
    ~nvMemory();
    bool saveConfig(TSettings* Settings);
    bool loadConfig(TSettings* Settings);
    bool deleteConfig();
private:
    bool init();
    bool Initialized_;
};

#ifdef NVMEM_SPIFFS

#define ESP_DRD_USE_SPIFFS true

#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

nvMemory::nvMemory()
{
    Initialized_ = false;
}

nvMemory::~nvMemory()
{
    if (Initialized_)
        SPIFFS.end();
};

bool nvMemory::saveConfig(TSettings* Settings)
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
            return false;
        }

        // Serialize JSON data to write to file
        serializeJsonPretty(json, Serial);
        Serial.print('\n');
        if (serializeJson(json, configFile) == 0)
        {
            // Error writing file
            Serial.println(F("SPIFS: Failed to write to file"));
            return false;
        }
        // Close file
        configFile.close();
        return true;
    };
    return false;
}

bool nvMemory::loadConfig(TSettings* Settings)
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
                    if (json.containsKey(JSON_KEY_POOLPORT))
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

bool nvMemory::deleteConfig()
{
    Serial.println("SPIFS: Erasing config file..");
    return SPIFFS.remove(JSON_CONFIG_FILE); //Borramos fichero
}

bool nvMemory::init()
{
    if (!Initialized_)
    {
        Serial.println("SPIFS: Mounting File System...");
        // May need to make it begin(true) first time you are using SPIFFS
        Initialized_ = SPIFFS.begin(false) || SPIFFS.begin(true);
        Initialized_ ? Serial.println("SPIFS: Mounted") : Serial.println("SPIFS: Mounting failed.");
    }
    else
    {
        Serial.println("SPIFS: Already Mounted");
    }
    return Initialized_;
};


#else
#error We need some kind of permanent storage implemented here!

nvMemory::nvMemory() {}
nvMemory::~nvMemory() {}
bool nvMemory::saveConfig(TSettings* Settings) { return false; }
bool nvMemory::loadConfig(TSettings* Settings) { return false; }
bool nvMemory::deleteConfig() { return false; }
bool nvMemory::init() { return false; }


#endif //NVMEM_TYPE

#endif // _NVMEMORY_H_
