#define ESP_DRD_USE_SPIFFS true

// Include Libraries
//#include ".h"

#include <WiFi.h>
#include <WiFiManager.h>
#include "wManager.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"
#include "mining.h"
#include "timeconst.h"

// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
TSettings Settings;

// Define WiFiManager Object
WiFiManager wm;
extern monitor_data mMonitor;

nvMemory nvMem;

extern SDCard SDCrd;

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Debería Guardar Configuración");
    shouldSaveConfig = true;    
    //wm.setConfigPortalBlocking(false);
}

/* void saveParamsCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
    nvMem.saveConfig(&Settings);
} */

void configModeCallback(WiFiManager* myWiFiManager)
// Called when config mode launched
{
    Serial.println("Entrando En Modo Configuración");
    drawSetupScreen();
    Serial.print("Configuración SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());

    Serial.print("Configuración Dirección IP: ");
    Serial.println(WiFi.softAPIP());
}

void reset_configuration()
{
    Serial.println("Borrando Configuración, Reiniciando");
    nvMem.deleteConfig();
    resetStat();
    wm.resetSettings();
    ESP.restart();
}

void init_WifiManager()
{
#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED
    //Serial.setTxTimeoutMs(10);

    //Init pin 15 to eneble 5V external power (LilyGo bug)
#ifdef PIN_ENABLE5V
    pinMode(PIN_ENABLE5V, OUTPUT);
    digitalWrite(PIN_ENABLE5V, HIGH);
#endif

    // Change to true when testing to force configuration every time we run
    bool forceConfig = false;

#if defined(PIN_BUTTON_2)
    // Check if button2 is pressed to enter configMode with actual configuration
    if (!digitalRead(PIN_BUTTON_2)) {
        Serial.println(F("Botón Presionado Para Forzar Entrar En Modo Configuración"));
        forceConfig = true;
        wm.setBreakAfterConfig(true); //Set to detect config edition and save
    }
#endif
    // Explicitly set WiFi mode
    WiFi.mode(WIFI_STA);

    if (!nvMem.loadConfig(&Settings))
    {
        //No config file on internal flash.
        if (SDCrd.loadConfigFile(&Settings))
        {
            //Config file on SD card.
            SDCrd.SD2nvMemory(&nvMem, &Settings); // reboot on success.          
        }
        else
        {
            //No config file on SD card. Starting wifi config server.
            forceConfig = true;
        }
    };
    
    // Free the memory from SDCard class 
    SDCrd.terminate();
    
    // Reset settings (only for development)
    //wm.resetSettings();

    //Set dark theme
    //wm.setClass("invert"); // dark theme

    // Set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveConfigCallback);

    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wm.setAPCallback(configModeCallback);    

    //Advanced settings
    wm.setConfigPortalBlocking(false); //Hacemos que el portal no bloquee el firmware
    wm.setConnectTimeout(40); // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(180); // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    //wm.setTimeout(120);
    //wm.setConfigPortalTimeout(120); //seconds

    // Custom elements

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter pool_text_box("Poolurl", "URL Del Pool", Settings.PoolAddress.c_str(), 80);
    // Need to convert numerical input to string to display the default value.
    char convertedValue[6];
    sprintf(convertedValue, "%d", Settings.PoolPort);
    // Text box (Number) - 7 characters maximum
    WiFiManagerParameter port_text_box_num("Poolport", "Puerto Del Pool", convertedValue, 7);

    // Text box (String) - 80 characters maximum
    //WiFiManagerParameter password_text_box("Poolpassword", "Pool password (Optional)", Settings.PoolPassword, 80);

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter addr_text_box("btcAddress", "Tu Billetera De Bitcoin, ( Añade .Identificador Para Identificar El Minador )", Settings.BtcWallet, 80);

    WiFiManagerParameter addr_text_box1("botTelegram", "Solo Para NerdMinerV2: Configuración Del Bot De Telegram. Introduce El Token De Tu Bot De Telegram. Si No Deseas Usar Un Bot, Deja Este Campo Como Está. No Incluyas La Palabra 'bot' Al Inicio, Solo Copia El Token Tal Como Lo Proporciona Telegram. Dónde Encontrar Tu Token: 1. Abre Telegram Y Busca 'BotFather'. 2. Escribe '/newbot' Y Sigue Las Instrucciones. 3. Copia El Token Que Te Proporcione Y Pégalo Aquí.", Settings.botTelegram, 80);

    WiFiManagerParameter addr_text_box2("chanelIDTelegram", "Solo Para NerdMinerV2: Configuración Del ID Del Canal De Telegram. Introduce El ID Del Canal De Telegram Al Que Quieres Que El NerdMinerV2 Envíe Mensajes. Si No Deseas Usar Esta Función, Deja Este Campo Como Está. Asegúrate De Copiar El ID Correctamente, Para Que El Envío De Mensajes Funcione. Importante: El Bot Debe Ser Administrador Del Canal Para Poder Enviar Mensajes. Dónde Encontrar El ID Del Canal: 1. Abre Telegram Y Ve Al Canal. 2. Reenvía Un Mensaje Del Canal A '@userinfobot'. 3. Copia El ID Que Te Proporcione Y Pégalo Aquí. 4. Agrega Tu Bot Al Canal Y Conviértelo En Administrador. Si Todo Está Bien Configurado, El NerdMinerV2 Mandará Un Mensaje Al Canal De Telegram Con Estadísticas De Minado Cada 2h, Día Y Noche Incluidas, ' 12 Mensajes Diarios '...", Settings.ChanelIDTelegram, 80);

  // Text box (Number) - 2 characters maximum
  char charZone[6];
  sprintf(charZone, "%d", Settings.Timezone);
  WiFiManagerParameter time_text_box_num("TimeZone", "Zona Horaria Desde UTC (-12/+12)", charZone, 3);

  WiFiManagerParameter features_html("<hr><br><label style=\"font-weight: bold;margin-bottom: 25px;display: inline-block;\">Nuevas Funcionalidades</label>");

  char checkboxParams[24] = "type=\"checkbox\"";
  if (Settings.saveStats)
  {
    strcat(checkboxParams, " checked");
  }
  WiFiManagerParameter save_stats_to_nvs("SaveStatsToNVS", "Graba Estadísticas De Minado A La Memoria Flash, Se Recuperan Al Reiniciar.", "T", 2, checkboxParams, WFM_LABEL_AFTER);
  // Text box (String) - 80 characters maximum
  WiFiManagerParameter password_text_box("PoolPassword - Opcional", "Pool Password", Settings.PoolPassword, 80);

  // Add all defined parameters
  wm.addParameter(&pool_text_box);
  wm.addParameter(&port_text_box_num);
  wm.addParameter(&password_text_box);
  wm.addParameter(&addr_text_box);
  wm.addParameter(&addr_text_box1);
  wm.addParameter(&addr_text_box2);
  wm.addParameter(&time_text_box_num);
  wm.addParameter(&features_html);
  wm.addParameter(&save_stats_to_nvs);
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  char checkboxParams2[24] = "type=\"checkbox\"";
  if (Settings.invertColors)
  {
    strcat(checkboxParams2, " checked");
  }
  WiFiManagerParameter invertColors("inverColors", "Invert Display Colors (if the colors looks weird)", "T", 2, checkboxParams2, WFM_LABEL_AFTER);
  wm.addParameter(&invertColors);
  #endif
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    char brightnessConvValue[2];
    sprintf(brightnessConvValue, "%d", Settings.Brightness);
    // Text box (Number) - 3 characters maximum
    WiFiManagerParameter brightness_text_box_num("Brightness", "Screen backlight Duty Cycle (0-255)", brightnessConvValue, 3);
    wm.addParameter(&brightness_text_box_num);
  #endif

    Serial.println("¡ Todo Hecho !: ");
    if (forceConfig)    
    {
        // Run if we need a configuration
        //No configuramos timeout al modulo
        wm.setConfigPortalBlocking(true); //Hacemos que el portal SI bloquee el firmware
        drawSetupScreen();
        mMonitor.NerdStatus = NM_Connecting;
        if (!wm.startConfigPortal(DEFAULT_SSID, DEFAULT_WIFIPW))
        {
            //Could be break forced after edditing, so save new config
            Serial.println("Falló Al Conectar Y Se Alcanzó El Tiempo De Espera");
            Settings.PoolAddress = pool_text_box.getValue();
            Settings.PoolPort = atoi(port_text_box_num.getValue());
            strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
            strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
            strncpy(Settings.botTelegram, addr_text_box1.getValue(), sizeof(Settings.botTelegram));
            strncpy(Settings.ChanelIDTelegram, addr_text_box2.getValue(), sizeof(Settings.ChanelIDTelegram));
            Settings.Timezone = atoi(time_text_box_num.getValue());
            //Serial.println(save_stats_to_nvs.getValue());
            Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
            #endif
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
            #endif
            nvMem.saveConfig(&Settings);
            delay(3*SECOND_MS);
            //reset and try again, or maybe put it to deep sleep
            ESP.restart();            
        };
    }
    else
    {
        //Tratamos de conectar con la configuración inicial ya almacenada
        mMonitor.NerdStatus = NM_Connecting;
        // disable captive portal redirection
        wm.setCaptivePortalEnable(true); 
        wm.setConfigPortalBlocking(true);
        wm.setEnableConfigPortal(true);
        // if (!wm.autoConnect(Settings.WifiSSID.c_str(), Settings.WifiPW.c_str()))
        if (!wm.autoConnect(DEFAULT_SSID, DEFAULT_WIFIPW))
        {
            Serial.println("Falló Al Conectar Al WIFI Configurado Y Se Alcanzó El Tiempo De Espera");
            if (shouldSaveConfig) {
                // Save new config            
                Settings.PoolAddress = pool_text_box.getValue();
                Settings.PoolPort = atoi(port_text_box_num.getValue());
                strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
                strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
                strncpy(Settings.botTelegram, addr_text_box1.getValue(), sizeof(Settings.botTelegram));
                strncpy(Settings.ChanelIDTelegram, addr_text_box2.getValue(), sizeof(Settings.ChanelIDTelegram));
                Settings.Timezone = atoi(time_text_box_num.getValue());
                // Serial.println(save_stats_to_nvs.getValue());
                Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
                #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
                #endif
                #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
                #endif
                nvMem.saveConfig(&Settings);
                vTaskDelay(2000 / portTICK_PERIOD_MS);      
            }        
            ESP.restart();                            
        } 
    }
    
    //Conectado a la red Wifi
    if (WiFi.status() == WL_CONNECTED) {
        //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
        Serial.println("");
        Serial.println("WiFi Conectada");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());

        // Lets deal with the user config values

        // Copy the string value
        Settings.PoolAddress = pool_text_box.getValue();
        //strncpy(Settings.PoolAddress, pool_text_box.getValue(), sizeof(Settings.PoolAddress));
        Serial.print("Cadena De Pool: ");
        Serial.println(Settings.PoolAddress);

        //Convert the number value
        Settings.PoolPort = atoi(port_text_box_num.getValue());
        Serial.print("Número De Puerto: ");
        Serial.println(Settings.PoolPort);

        // Copy the string value
        strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
        Serial.print("Password De Pool: ");
        Serial.println(Settings.PoolPassword);

        // Copy the string value
        strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
        Serial.print("Billetera De Bitcoin: ");
        Serial.println(Settings.BtcWallet);

         // Copy the string value
         strncpy(Settings.botTelegram, addr_text_box1.getValue(), sizeof(Settings.botTelegram));
         Serial.print("Bot De Telegram: ");
         Serial.println(Settings.botTelegram);

          // Copy the string value
        strncpy(Settings.ChanelIDTelegram, addr_text_box2.getValue(), sizeof(Settings.ChanelIDTelegram));
        Serial.print("ID Canal De Telegram: ");
        Serial.println(Settings.ChanelIDTelegram);

        //Convert the number value
        Settings.Timezone = atoi(time_text_box_num.getValue());
        Serial.print("ZonaHoraria Desde UTC: ");
        Serial.println(Settings.Timezone);

        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
        Serial.print("Invert Colors: ");
        Serial.println(Settings.invertColors);        
        #endif

        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        Settings.Brightness = atoi(brightness_text_box_num.getValue());
        Serial.print("Brightness: ");
        Serial.println(Settings.Brightness);
        #endif

    }

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        nvMem.saveConfig(&Settings);
        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
         if (Settings.invertColors) ESP.restart();                
        #endif
        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        if (Settings.Brightness != 250) ESP.restart();
        #endif
    }
}

//----------------- MAIN PROCESS WIFI MANAGER --------------
int oldStatus = 0;

void wifiManagerProcess() {

    wm.process(); // avoid delays() in loop when non-blocking and other long running code

    int newStatus = WiFi.status();
    if (newStatus != oldStatus) {
        if (newStatus == WL_CONNECTED) {
            Serial.println("CONECTADO - IP Actual: " + WiFi.localIP().toString());
        } else {
            Serial.print("[Error] - Estado Actual: ");
            Serial.println(newStatus);
        }
        oldStatus = newStatus;
    }
}
