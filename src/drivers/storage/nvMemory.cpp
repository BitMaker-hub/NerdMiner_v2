#include "nvMemory.h"

#ifdef NVMEM_SPIFFS

#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "../devices/device.h"
#include "storage.h"

nvMemory::nvMemory() : Initialized_(false) {};

nvMemory::~nvMemory()
{
    if (Initialized_)
        SPIFFS.end();
};

/// @brief Save settings to config file on SPIFFS
/// @param TSettings* Settings to be saved.
/// @return true on success
bool nvMemory::saveConfig(TSettings *Settings)
{
    if (init())
    {
        // Save Config in JSON format
        Serial.println(F("SPIFS: Grabando Configuración"));

        // Create a JSON document
        StaticJsonDocument<512> json;
        json[JSON_SPIFFS_KEY_POOLURL] = Settings->PoolAddress;
        json[JSON_SPIFFS_KEY_POOLPORT] = Settings->PoolPort;
        json[JSON_SPIFFS_KEY_POOLPASS] = Settings->PoolPassword;
        json[JSON_SPIFFS_KEY_WALLETID] = Settings->BtcWallet;
        json[JSON_SPIFFS_KEY_BOTTELEGRAM] = Settings->botTelegram;
        json[JSON_SPIFFS_KEY_CHANELID] = Settings->ChanelIDTelegram;
        json[JSON_SPIFFS_KEY_TIMEZONE] = Settings->Timezone;
        json[JSON_SPIFFS_KEY_STATS2NV] = Settings->saveStats;
        json[JSON_SPIFFS_KEY_INVCOLOR] = Settings->invertColors;
        json[JSON_SPIFFS_KEY_BRIGHTNESS] = Settings->Brightness;

        // Open config file
        File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
        if (!configFile)
        {
            // Error, file did not open
            Serial.println("SPIFS: Error Al Abrir El Fichero De Configuración Para Escritura");
            return false;
        }

        // Serialize JSON data to write to file
        serializeJsonPretty(json, Serial);
        Serial.print('\n');
        if (serializeJson(json, configFile) == 0)
        {
            // Error writing file
            Serial.println(F("SPIFS: Fallo Al Grabar Al Fichero De Configuración"));
            return false;
        }
        // Close file
        configFile.close();
        return true;
    };
    return false;
}

/// @brief Load settings from config file located in SPIFFS.
/// @param TSettings* Struct to update with new settings.
/// @return true on success
bool nvMemory::loadConfig(TSettings *Settings)
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
            File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
            if (configFile)
            {
                Serial.println("SPIFS: Cargando Fichero De Configuración");
                StaticJsonDocument<512> json;
                DeserializationError error = deserializeJson(json, configFile);
                configFile.close();
                serializeJsonPretty(json, Serial);
                Serial.print('\n');
                if (!error)
                {
                    Settings->PoolAddress = json[JSON_SPIFFS_KEY_POOLURL] | Settings->PoolAddress;
                    strcpy(Settings->PoolPassword, json[JSON_SPIFFS_KEY_POOLPASS] | Settings->PoolPassword);
                    strcpy(Settings->BtcWallet, json[JSON_SPIFFS_KEY_WALLETID] | Settings->BtcWallet);
                    strcpy(Settings->botTelegram, json[JSON_SPIFFS_KEY_BOTTELEGRAM] | Settings->botTelegram);
                    strcpy(Settings->ChanelIDTelegram, json[JSON_SPIFFS_KEY_CHANELID] | Settings->ChanelIDTelegram);
                    if (json.containsKey(JSON_SPIFFS_KEY_POOLPORT))
                        Settings->PoolPort = json[JSON_SPIFFS_KEY_POOLPORT].as<int>();
                    if (json.containsKey(JSON_SPIFFS_KEY_TIMEZONE))
                        Settings->Timezone = json[JSON_SPIFFS_KEY_TIMEZONE].as<int>();
                    if (json.containsKey(JSON_SPIFFS_KEY_STATS2NV))
                        Settings->saveStats = json[JSON_SPIFFS_KEY_STATS2NV].as<bool>();
                    if (json.containsKey(JSON_SPIFFS_KEY_INVCOLOR))
                    {
                        Settings->invertColors = json[JSON_SPIFFS_KEY_INVCOLOR].as<bool>();
                    }
                    else
                    {
                        Settings->invertColors = false;
                    }
                    if (json.containsKey(JSON_SPIFFS_KEY_BRIGHTNESS))
                    {
                        Settings->Brightness = json[JSON_SPIFFS_KEY_BRIGHTNESS].as<int>();
                    }
                    else
                    {
                        Settings->Brightness = 250;
                    }
                    return true;
                }
                else
                {
                    // Error loading JSON data
                    Serial.println("SPIFS: Error Analizando Fichero De Configuración!");
                }
            }
            else
            {
                Serial.println("SPIFS: Error Abriendo Fichero De Configuración!");
            }
        }
        else
        {
            Serial.println("SPIFS: No Hay Ficheror De Configuración Disponible!");
        }
    }
    return false;
}

/// @brief Delete config file from SPIFFS
/// @return true on successs
bool nvMemory::deleteConfig()
{
    Serial.println("SPIFS: Borrando Fichero De Configuración");
    return SPIFFS.remove(JSON_CONFIG_FILE); // Borramos fichero
}

/// @brief Prepare and mount SPIFFS
/// @return true on success
bool nvMemory::init()
{
    if (!Initialized_)
    {
        Serial.println("SPIFS: Montando Sistema De Ficheros...");
        // May need to make it begin(true) first time you are using SPIFFS
        Initialized_ = SPIFFS.begin(false) || SPIFFS.begin(true);
        Initialized_ ? Serial.println("SPIFS: Montado") : Serial.println("SPIFS: Montado De Sistema Fallido...");
    }
    else
    {
        Serial.println("SPIFS: Ya Está Montado...");
    }
    return Initialized_;
};

#else

nvMemory::nvMemory() {}
nvMemory::~nvMemory() {}
bool nvMemory::saveConfig(TSettings *Settings) { return false; }
bool nvMemory::loadConfig(TSettings *Settings) { return false; }
bool nvMemory::deleteConfig() { return false; }
bool nvMemory::init() { return false; }

#endif // NVMEM_TYPE