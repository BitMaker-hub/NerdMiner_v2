#define ESP_DRD_USE_SPIFFS true

// Include Libraries
//#include ".h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <FS.h>

#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "media/images.h"
#include <TFT_eSPI.h> // Graphics and font library
#include "wManager.h"

// JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
char poolString[80] = "solo.ckpool.org";
int portNumber = 3333;
char btcString[80] = "yourBtcAddress";


// Define WiFiManager Object
WiFiManager wm;

static int buttonReset = 1;
static unsigned long lastButtonPress = 0; // Última vez que se pulsó el botón

volatile bool buttonPressed = false;

extern TFT_eSPI tft;  // tft variable declared on main

void saveConfigFile()
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));
  
  // Create a JSON document
  StaticJsonDocument<512> json;
  json["poolString"] = poolString;
  json["portNumber"] = portNumber;
  json["btcString"] = btcString;

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

bool loadConfigFile()
// Load existing configuration file
{
  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
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

          strcpy(poolString, json["poolString"]);
          strcpy(btcString, json["btcString"]);
          portNumber = json["portNumber"].as<int>();

          return true;
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

  return false;
}


void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager)
// Called when config mode launched
{
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void init_WifiManager()
{
  Serial.begin(115200);
  //Serial.setTxTimeoutMs(10);
  //Init config pin
  // pinMode(TRIGGER_PIN, INPUT);
  
  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;
 
  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup)
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  // Reset settings (only for development)
  //wm.resetSettings();

  //Set dark theme
  //wm.setClass("invert"); // dark theme
  
  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //Advanced settings
  wm.setConfigPortalBlocking(false); //Hacemos que el portal no bloquee el firmware
  wm.setConnectTimeout(30); // how long to try to connect for before continuing
  //wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  // wm.setAPClientCheck(true); // avoid timeout if client connected to softap
  //wm.setTimeout(120);
  //wm.setConfigPortalTimeout(120); //seconds
  
  // Custom elements

  // Text box (String) - 80 characters maximum
  WiFiManagerParameter pool_text_box("Poolurl", "Pool url", poolString, 80);

  // Need to convert numerical input to string to display the default value.
  char convertedValue[6];
  sprintf(convertedValue, "%d", portNumber); 
  
  // Text box (Number) - 7 characters maximum
  WiFiManagerParameter port_text_box_num("Poolport", "Pool port", convertedValue, 7); 

  // Text box (String) - 80 characters maximum
  WiFiManagerParameter addr_text_box("btcAddress", "Your BTC address", btcString, 80);

  // Add all defined parameters
  wm.addParameter(&pool_text_box);
  wm.addParameter(&port_text_box_num);
  wm.addParameter(&addr_text_box);

  Serial.println("AllDone: ");
  if (forceConfig)
    // Run if we need a configuration
  {
    //No configuramos timeout al modulo
    wm.setConfigPortalBlocking(true); //Hacemos que el portal SI bloquee el firmware
    tft.pushImage(0, 0, setupModeWidth, setupModeHeight, setupModeScreen);
    if (!wm.startConfigPortal("NerdMinerAP","MineYourCoins"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  }
  else
  {
    //Tratamos de conectar con la configuración inicial ya almacenada
    wm.setCaptivePortalEnable(false); // disable captive portal redirection
    if (!wm.autoConnect("NerdMinerAP","MineYourCoins"))
    {
      Serial.println("Failed to connect and hit timeout");
      //delay(3000);
      // if we still have not connected restart and try all over again
      //ESP.restart();
      //delay(5000);
    }
  }

  //Conectado a la red Wifi
  if(WiFi.status() == WL_CONNECTED){
    //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // If we get here, we are connected to the WiFi

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    // Lets deal with the user config values
  
    // Copy the string value
    strncpy(poolString, pool_text_box.getValue(), sizeof(poolString));
    Serial.print("PoolString: ");
    Serial.println(poolString);
  
    //Convert the number value
    portNumber = atoi(port_text_box_num.getValue());
    Serial.print("portNumber: ");
    Serial.println(portNumber);
  
    // Copy the string value
    strncpy(btcString, addr_text_box.getValue(), sizeof(btcString));
    Serial.print("btcString: ");
    Serial.println(btcString);
  }
  
  // Save the custom parameters to FS
  if (shouldSaveConfig)
  {
    saveConfigFile();
  }
}

void checkResetConfigButton(){

  // Leer el estado del botón
  int buttonState = digitalRead(PIN_BUTTON_1);
  unsigned int last_time = (millis() - lastButtonPress);
  Serial.printf("button pressed %i - %u\n", buttonReset, last_time);

  buttonReset++;
  lastButtonPress = millis();

  if ( last_time > 1000) {
    buttonReset = 1;
  }

  // Si el botón está pulsado y ha pasado suficiente tiempo desde la última pulsación
  if (last_time < 1000 && buttonReset == 4) {
    buttonPressed = true;
  } 
    
}

void checkRemoveConfiguration() {
  if(!buttonPressed){
    return;
  }

  buttonPressed = false;
  // check for button press
  Serial.printf("[CONFIG] Button reset pressed %i\n", buttonReset);
  Serial.println("[CONFIG] Erasing pool config");
  Serial.println("[CONFIG] Deleting existing configuration");
  SPIFFS.remove(JSON_CONFIG_FILE); //Borramos fichero
  Serial.println("[CONFIG] Erasing wifi config");
  wm.resetSettings();
  Serial.println("[CONFIG] Restarting");
  delay(1000); 
  ESP.restart();
}


void wifiManagerProcess() {
  wm.process(); // avoid delays() in loop when non-blocking and other long running code
}


