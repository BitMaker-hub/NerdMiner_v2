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
#include "timezones.h"

#include <ArduinoJson.h>
#include <esp_flash.h>


// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
TSettings Settings;

// Define WiFiManager Object
WiFiManager wm;
extern monitor_data mMonitor;

nvMemory nvMem;

extern SDCard SDCrd;

// Web config parameters. They must live for the whole execution because the web portal keeps
// running (and WiFiManager keeps pointers to them and to their custom HTML) while mining.
static WiFiManagerParameter* pool_text_box;
static WiFiManagerParameter* port_text_box_num;
static WiFiManagerParameter* password_text_box;
static WiFiManagerParameter* addr_text_box;
static WiFiManagerParameter* tz_select_html;
static WiFiManagerParameter* tz_rule_hidden;
static WiFiManagerParameter* web_password_text_box;
static WiFiManagerParameter* features_html;
static WiFiManagerParameter* save_stats_to_nvs;
static char saveStatsAttrs[24];
static String tzSelectHtml;
#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
static WiFiManagerParameter* invertColors;
static WiFiManagerParameter* brightness_text_box_num;
static char invertColorsAttrs[24];
#endif

// Delayed restart after saving from the web portal, so the HTTP response reaches the browser
static bool restartPending = false;
static unsigned long restartRequestedAt = 0;

String readCustomAPName() {
    Serial.println("DEBUG: Attempting to read custom AP name from flash at 0x3F0000...");
    
    // Leer directamente desde flash
    const size_t DATA_SIZE = 128;
    uint8_t buffer[DATA_SIZE];
    memset(buffer, 0, DATA_SIZE); // Clear buffer
    
    // Leer desde 0x3F0000
    esp_err_t result = esp_flash_read(NULL, buffer, 0x3F0000, DATA_SIZE);
    if (result != ESP_OK) {
        Serial.printf("DEBUG: Flash read error: %s\n", esp_err_to_name(result));
        return "";
    }
    
    Serial.println("DEBUG: Successfully read from flash");
    String data = String((char*)buffer);
    
    // Debug: show raw data read
    Serial.printf("DEBUG: Raw flash data: '%s'\n", data.c_str());
    
    if (data.startsWith("WEBFLASHER_CONFIG:")) {
        Serial.println("DEBUG: Found WEBFLASHER_CONFIG marker");
        String jsonPart = data.substring(18); // Después del marcador "WEBFLASHER_CONFIG:"
        
        Serial.printf("DEBUG: JSON part: '%s'\n", jsonPart.c_str());
        
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, jsonPart);
        
        if (error == DeserializationError::Ok) {
            Serial.println("DEBUG: JSON parsed successfully");
            
            if (doc.containsKey("apname")) {
                String customAP = doc["apname"].as<String>();
                customAP.trim();
                
                if (customAP.length() > 0 && customAP.length() < 32) {
                    Serial.printf("✅ Custom AP name from webflasher: %s\n", customAP.c_str());
                    return customAP;
                } else {
                    Serial.printf("DEBUG: AP name invalid length: %d\n", customAP.length());
                }
            } else {
                Serial.println("DEBUG: 'apname' key not found in JSON");
            }
        } else {
            Serial.printf("DEBUG: JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.println("DEBUG: WEBFLASHER_CONFIG marker not found - no custom config");
    }
    
    Serial.println("DEBUG: Using default AP name");
    return "";
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
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
    Serial.println("Entered Configuration Mode");
    drawSetupScreen();
    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());

    Serial.print("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
}

// Copies the values of the web form parameters to Settings
static void applyParamsToSettings()
{
    Settings.PoolAddress = pool_text_box->getValue();
    Serial.print("PoolString: ");
    Serial.println(Settings.PoolAddress);

    Settings.PoolPort = atoi(port_text_box_num->getValue());
    Serial.print("portNumber: ");
    Serial.println(Settings.PoolPort);

    strncpy(Settings.PoolPassword, password_text_box->getValue(), sizeof(Settings.PoolPassword) - 1);
    Settings.PoolPassword[sizeof(Settings.PoolPassword) - 1] = '\0';
    Serial.print("poolPassword: ");
    Serial.println(Settings.PoolPassword);

    strncpy(Settings.BtcWallet, addr_text_box->getValue(), sizeof(Settings.BtcWallet) - 1);
    Settings.BtcWallet[sizeof(Settings.BtcWallet) - 1] = '\0';
    Serial.print("btcString: ");
    Serial.println(Settings.BtcWallet);

    Settings.TzRule = tz_rule_hidden->getValue();
    Serial.print("TzRule: ");
    Serial.println(Settings.TzRule);

    // Empty means no password for the web portal
    Settings.WebPassword = web_password_text_box->getValue();

    Settings.saveStats = (strncmp(save_stats_to_nvs->getValue(), "T", 1) == 0);

    #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    Settings.invertColors = (strncmp(invertColors->getValue(), "T", 1) == 0);
    Serial.print("Invert Colors: ");
    Serial.println(Settings.invertColors);
    #endif

    #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    Settings.Brightness = atoi(brightness_text_box_num->getValue());
    Serial.print("Brightness: ");
    Serial.println(Settings.Brightness);
    #endif
}

// Builds the <select> with the time zones. The selected option is copied to the hidden TzRule input.
static String buildTzSelectHtml(const String& currentRule)
{
    String html = "<br/><label for='tzSelect'>Time zone</label><br/>"
                  "<select id='tzSelect' onchange=\"document.getElementById('TzRule').value=this.value\">";
    bool found = false;
    String options;
    for (size_t i = 0; i < TZ_OPTIONS_COUNT; i++) {
        bool selected = currentRule == TZ_OPTIONS[i].rule;
        found = found || selected;
        options += String("<option value='") + TZ_OPTIONS[i].rule + "'" + (selected ? " selected" : "") + ">" + TZ_OPTIONS[i].label + "</option>";
    }
    if (!found) {
        // Old config with only an integer offset (or a rule that is not in the list): keep it selectable
        String label = Settings.TzRule.length() > 0
            ? String("Custom (") + currentRule + ")"
            : String("UTC") + (Settings.Timezone >= 0 ? "+" : "") + Settings.Timezone + " (fixed, no DST)";
        html += String("<option value='") + currentRule + "' selected>" + label + "</option>";
    }
    html += options;
    html += "</select>";
    return html;
}

// Asks for HTTP basic auth (user "admin") on every request of the web portal
class WebAuthHandler : public RequestHandler {
public:
    explicit WebAuthHandler(WebServer* server) : _server(server) {}
    bool canHandle(HTTPMethod, String) override { return needsAuth(); }
    bool canUpload(String) override { return needsAuth(); }
    bool handle(WebServer& server, HTTPMethod, String) override
    {
        server.requestAuthentication(BASIC_AUTH, "NerdMiner");
        return true;
    }

private:
    bool needsAuth()
    {
        return Settings.WebPassword.length() > 0 && !_server->authenticate("admin", Settings.WebPassword.c_str());
    }
    WebServer* _server;
};

// Called by WiFiManager when it creates the web server, before it registers its own routes
static void webServerCallback()
{
    wm.server->collectHeaders(nullptr, 0); // so authenticate() can see the Authorization header
    if (Settings.WebPassword.length() > 0) {
        wm.server->addHandler(new WebAuthHandler(wm.server.get()));
    }
    // Routes that are not offered in the menu stay disabled
    const char* disabledRoutes[] = { "/erase", "/update", "/updatedone", "/exit", "/close" };
    for (const char* route : disabledRoutes) {
        wm.server->on(route, []() { wm.server->send(404, "text/plain", "Not found"); });
    }
}

// Called when the parameters are saved from the web portal while mining
static void webSaveParamsCallback()
{
    Serial.println("Config saved from web portal, restarting");
    applyParamsToSettings();
    nvMem.saveConfig(&Settings);
    restartPending = true;
    restartRequestedAt = millis();
}

void reset_configuration()
{
    Serial.println("Erasing Config, restarting");
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
    
    // Check for custom AP name from flasher config, otherwise use default
    String customAPName = readCustomAPName();
    const char* apName = customAPName.length() > 0 ? customAPName.c_str() : DEFAULT_SSID;

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
        Serial.println(F("Button pressed to force start config mode"));
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
    pool_text_box = new WiFiManagerParameter("Poolurl", "Pool url", Settings.PoolAddress.c_str(), 80);

    // Need to convert numerical input to string to display the default value.
    char convertedValue[6];
    sprintf(convertedValue, "%d", Settings.PoolPort);

    // Text box (Number) - 7 characters maximum
    port_text_box_num = new WiFiManagerParameter("Poolport", "Pool port", convertedValue, 7);

    // Text box (String) - 80 characters maximum
    //WiFiManagerParameter password_text_box("Poolpassword", "Pool password (Optional)", Settings.PoolPassword, 80);

    // Text box (String) - 80 characters maximum
    addr_text_box = new WiFiManagerParameter("btcAddress", "Your BTC address", Settings.BtcWallet, 80);

  // Time zone: a <select> that copies the POSIX rule of the chosen option to a hidden input
  String currentTzRule = effectiveTzRule(Settings);
  tzSelectHtml = buildTzSelectHtml(currentTzRule);
  tz_select_html = new WiFiManagerParameter(tzSelectHtml.c_str());
  tz_rule_hidden = new WiFiManagerParameter("TzRule", "", currentTzRule.c_str(), 64, "type='hidden'", WFM_NO_LABEL);

  // Text box (Password) - 64 characters maximum
  web_password_text_box = new WiFiManagerParameter("WebPassword", "Web config password (user: admin, empty = no password)", Settings.WebPassword.c_str(), 64, "type='password'");

  features_html = new WiFiManagerParameter("<hr><br><label style=\"font-weight: bold;margin-bottom: 25px;display: inline-block;\">Features</label>");

  strcpy(saveStatsAttrs, "type=\"checkbox\"");
  if (Settings.saveStats)
  {
    strcat(saveStatsAttrs, " checked");
  }
  save_stats_to_nvs = new WiFiManagerParameter("SaveStatsToNVS", "Save mining statistics to flash memory.", "T", 2, saveStatsAttrs, WFM_LABEL_AFTER);
  // Text box (String) - 80 characters maximum
  password_text_box = new WiFiManagerParameter("Poolpassword - Optional", "Pool password", Settings.PoolPassword, 80);

  // Add all defined parameters
  wm.addParameter(pool_text_box);
  wm.addParameter(port_text_box_num);
  wm.addParameter(password_text_box);
  wm.addParameter(addr_text_box);
  wm.addParameter(tz_select_html);
  wm.addParameter(tz_rule_hidden);
  wm.addParameter(web_password_text_box);
  wm.addParameter(features_html);
  wm.addParameter(save_stats_to_nvs);
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  strcpy(invertColorsAttrs, "type=\"checkbox\"");
  if (Settings.invertColors)
  {
    strcat(invertColorsAttrs, " checked");
  }
  invertColors = new WiFiManagerParameter("inverColors", "Invert Display Colors (if the colors looks weird)", "T", 2, invertColorsAttrs, WFM_LABEL_AFTER);
  wm.addParameter(invertColors);
  #endif
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    char brightnessConvValue[6];
    sprintf(brightnessConvValue, "%d", Settings.Brightness);
    // Text box (Number) - 3 characters maximum
    brightness_text_box_num = new WiFiManagerParameter("Brightness", "Screen backlight Duty Cycle (0-255)", brightnessConvValue, 3);
    wm.addParameter(brightness_text_box_num);
  #endif

    Serial.println("AllDone: ");
    if (forceConfig)    
    {
        // Run if we need a configuration
        //No configuramos timeout al modulo
        wm.setConfigPortalBlocking(true); //Hacemos que el portal SI bloquee el firmware
        drawSetupScreen();
        mMonitor.NerdStatus = NM_Connecting;
        wm.startConfigPortal(apName, DEFAULT_WIFIPW);

        if (shouldSaveConfig)
        {
            //Could be break forced after edditing, so save new config
            Serial.println("failed to connect and hit timeout");
            applyParamsToSettings();
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
        if (!wm.autoConnect(apName, DEFAULT_WIFIPW))
        {
            Serial.println("Failed to connect to configured WIFI, and hit timeout");
            if (shouldSaveConfig) {
                // Save new config            
                applyParamsToSettings();
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
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());


        // Lets deal with the user config values
        applyParamsToSettings();

        if (!forceConfig) {
            // Keep the configuration web portal available on the miner's IP while mining
            wm.setSaveParamsCallback(webSaveParamsCallback);
            wm.setWebServerCallback(webServerCallback);
            wm.setConfigPortalBlocking(false);
            wm.setShowInfoErase(false);
            wm.setShowInfoUpdate(false);
            std::vector<const char*> menu = { "param", "wifi", "info", "sep", "restart" };
            wm.setMenu(menu);
            wm.startWebPortal();
            Serial.println("Web config available at http://" + WiFi.localIP().toString());
        }
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

    if (restartPending && millis() - restartRequestedAt > 2000) {
        ESP.restart();
    }

    int newStatus = WiFi.status();
    if (newStatus != oldStatus) {
        if (newStatus == WL_CONNECTED) {
            Serial.println("CONNECTED - Current ip: " + WiFi.localIP().toString());
        } else {
            Serial.print("[Error] - current status: ");
            Serial.println(newStatus);
        }
        oldStatus = newStatus;
    }
}
