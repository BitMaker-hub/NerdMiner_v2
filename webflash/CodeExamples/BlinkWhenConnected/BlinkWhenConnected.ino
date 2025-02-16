/*******************************************************************
    An ESP32 project that blinks when connected to WiFi, which is configured
    via serial

    It's inteded to be used with the ESP Web Tools, but if you want to test
    with the Arduino IDE, select "Both NL & CR" in the Serial Monitor

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

#include "WiFi.h"

//This is needed for some of the Wifi functions we are using
#include <esp_wifi.h>

#define LED_PIN 2

bool connectWifi(String ssid, String pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int retry = 0;
  while (retry < 10 && WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    retry++;
    delay(500);
  }

  return WiFi.status() == WL_CONNECTED;
}


bool autoConnectToWifi() {
  String ssid;
  String pass;

  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);

  ssid = String(reinterpret_cast<const char*>(conf.sta.ssid));
  pass = String(reinterpret_cast<char*>(conf.sta.password));

  Serial.println("autoConnectToWifi");
  Serial.println(ssid);
  Serial.println(pass);

  bool connected = false;
  // Check if the ESP auto connected
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
  }

  // If it didn't auto connect, try connect with the stored
  // details
  if (connected || connectWifi(ssid, pass)) {

    return true; // connected success
  }

  return false;
}

// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(115200);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_PIN, OUTPUT);

  bool forceToDemo = true; // Will force it to always enter config mode

  //AutoConnect will try connect to Wifi using details stored in flash
  if (forceToDemo || !autoConnectToWifi()) {
    bool exitWifiConnect = false;
    String ssid = "";
    String pass = "";
    int stage = 0;
    Serial.setTimeout(20000); //20 seconds
    //Wait here til we get Wifi details
    while (!exitWifiConnect) {

      Serial.println("Please enter your Wifi SSID:");
      ssid = Serial.readStringUntil('\r'); // Web Flash tools use '\r\n' for line endings
      Serial.read();
      if (ssid != "") {
        Serial.println("Please enter your Wifi password:");
        pass = Serial.readStringUntil('\r');
        Serial.read();

        exitWifiConnect = connectWifi(ssid, pass);
      }
      if (!exitWifiConnect) {
        Serial.println("Failed to connect:");
      } else {
        Serial.println("connect");
      }
    }
  }

  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_PIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
