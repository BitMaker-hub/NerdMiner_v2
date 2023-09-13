
#ifndef _UARTCFGUI_H_
#define _UARTCFGUI_H_

#include <Arduino.h>

#include "storage.h"
#include "SPIStorage.h"

class SerialCfgUI
{
public:
    SerialCfgUI()
    {
        #ifdef MONITOR_SPEED
            Serial.begin(MONITOR_SPEED);
        #else
            Serial.begin(115200);
        #endif //MONITOR_SPEED
    }

    void UARTUI2SPIStorage(SPIStorage* spifs)
    {
        TSettings Settings;
        
        if ((spifs->loadConfigFile(&Settings)) && StartUI(&Settings))
        {
            spifs->saveConfigFile(&Settings);
            WiFi.begin(Settings.WifiSSID, Settings.WifiPW);
            Serial.println("SerialUI: Settings transfered to internal memory. Restarting now.");
            ESP.restart();
        }
        Serial.println("SerialUI: not started.");
    }

private:

    bool StartUI(TSettings* Settings)
    {
        Serial.println("Welcome to Nerdminer v2 serial config Interface.");
        Serial.println("Fill out the form to set up your Nerdminer.");
        Serial.println("Press 'Enter' to save your input or the default value.");

        strcpy(Settings->WifiSSID, readEntry<const char*>("Enter Wifi SSID:", Settings->WifiSSID));
        strcpy(Settings->WifiPW, readEntry<const char*>("Enter Wifi Password:", Settings->WifiPW));
        strcpy(Settings->PoolAddress, readEntry<const char*>("Enter Pool Address:", Settings->PoolAddress));
        Settings->PoolPort = readEntry<int>("Enter Pool Port:", Settings->PoolPort);
        strcpy(Settings->BtcWallet, readEntry<const char*>("Enter your BTC Wallet ID:", Settings->BtcWallet));
        Settings->Timezone = readEntry<int>("Enter your Timezone (UTC+-12):", Settings->Timezone);

        Serial.println("Setup complete.");

        return true;
    }

    template <typename T>
    T readEntry(const char* message = "newEntry:", T defaultvalue = "", const char delimieter = '\n') 
        requires(
        (std::is_same_v<T, const char*>) 
        || (std::is_same_v<T, char*>) 
        || (std::is_same_v<T, int>) 
        || (std::is_same_v<T, double>) 
        || (std::is_same_v<T, float>)
        || (std::is_same_v<T, String>))
    {
        Serial.println(message);

        if(!String(defaultvalue).isEmpty())
        {
            Serial.print("Default Value: >");
            Serial.print(defaultvalue);
            Serial.println("<");
        };
        String value = Serial.readStringUntil(delimieter);
        value.trim();
        while((value.length() > 0) && value.endsWith(String('\r')))
            value.remove(value.length()-1);
        value.trim();
        if (value.length() > 0)
        {
            if constexpr (std::is_same_v<T, String>)
                return value;
            else if constexpr ((std::is_same_v<T, const char*>)|| (std::is_same_v<T, char*>))
                return value.c_str();
            else if constexpr (std::is_same_v<T, int>)
                return value.toInt();
            else if constexpr (std::is_same_v<T, double>)
                return value.toDouble();
            else if constexpr (std::is_same_v<T, float>)
                return value.toFloat();
        }
        else
        {
            return defaultvalue;
        }
    }
};


#endif // _UARTCFGUI_H_