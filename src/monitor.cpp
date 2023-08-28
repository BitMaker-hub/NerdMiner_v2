#include <Arduino.h>
#include <WiFi.h>
#include "mbedtls/md.h"
#include "HTTPClient.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "mining.h"
#include "utils.h"
#include "monitor.h"

extern char poolString[80];
extern unsigned long templates;
extern unsigned long hashes;
extern unsigned long Mhashes;
extern unsigned long totalKHashes;
extern unsigned long elapsedKHs;

extern unsigned int shares; // increase if blockhash has 32 bits of zeroes
extern unsigned int valids; // increased if blockhash <= target

extern double best_diff; // track best diff

extern monitor_data mMonitor;

extern int GMTzone; //Gotten from saved config

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
unsigned int bitcoin_price=0;
String current_block = "793261";
global_data gData;

void setup_monitor(void){
    /******** TIME ZONE SETTING *****/

    timeClient.begin();
    
    // Adjust offset depending on your zone
    // GMT +2 in seconds (zona horaria de Europa Central)
    timeClient.setTimeOffset(3600 * GMTzone);

    Serial.println("TimeClient setup done");
}

unsigned long mGlobalUpdate =0;

void updateGlobalData(void){
    
    if((mGlobalUpdate == 0) || (millis() - mGlobalUpdate > UPDATE_Global_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) return;
            
        //Make first API call to get global hash and current difficulty
        HTTPClient http;
        http.begin(getGlobalHash);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            String temp = "";
            if (doc.containsKey("currentHashrate")) temp = String(doc["currentHashrate"].as<float>());
            if(temp.length()>18 + 3) //Exahashes more than 18 digits + 3 digits decimals
              gData.globalHash = temp.substring(0,temp.length()-18 - 3);
            if (doc.containsKey("currentDifficulty")) temp = String(doc["currentDifficulty"].as<float>());
            if(temp.length()>10 + 3){ //Terahash more than 10 digits + 3 digit decimals
              temp = temp.substring(0,temp.length()-10 - 3);
              gData.difficulty = temp.substring(0,temp.length()-2) + "." + temp.substring(temp.length()-2,temp.length()) + "T";
            }
            doc.clear();

            mGlobalUpdate = millis();
        }
        http.end();

        
        //OLD code gets remaining blocks to next difficulty ajustment
        /*//Make second API call to get remaining Blocks
        http.begin(getDifficulty);
        httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            String temp = "";
            if (doc.containsKey("progressPercent")) gData.progressPercent = doc["progressPercent"].as<float>();
            if (doc.containsKey("remainingBlocks")) gData.remainingBlocks = doc["remainingBlocks"].as<int>();

            doc.clear();

            mGlobalUpdate = millis();
        }
        
        http.end();*/

        //Make third API call to get fees
        http.begin(getFees);
        httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            String temp = "";
            if (doc.containsKey("halfHourFee")) gData.halfHourFee = doc["halfHourFee"].as<int>();

            doc.clear();

            mGlobalUpdate = millis();
        }
        
        http.end();

    }
}

unsigned long mHeightUpdate = 0;

String getBlockHeight(void){
    
    if((mHeightUpdate == 0) || (millis() - mHeightUpdate > UPDATE_Height_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) return current_block;
            
        HTTPClient http;
        http.begin(getHeightAPI);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            payload.trim();

            current_block = payload;

            mHeightUpdate = millis();
        }
        
        http.end();

    }
  
  return current_block;
}

unsigned long mBTCUpdate = 0;

String getBTCprice(void){
    
    if((mBTCUpdate == 0) || (millis() - mBTCUpdate > UPDATE_BTC_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) return (String(bitcoin_price) + "$");
        
        HTTPClient http;
        http.begin(getBTCAPI);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();

            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            if (doc.containsKey("bitcoin")) bitcoin_price = doc["bitcoin"]["usd"];

            doc.clear();

            mBTCUpdate = millis();
        }
        
        http.end();

    }
  
  return (String(bitcoin_price) + "$");
}

unsigned long mTriggerUpdate = 0;
unsigned long initialMillis = millis();
unsigned long initialTime = 0;

String getTime(void){
  
  //Check if need an NTP call to check current time
  if((mTriggerUpdate == 0) || (millis() - mTriggerUpdate > UPDATE_PERIOD_h * 60 * 60 * 1000)){ //60 sec. * 60 min * 1000ms
    if(WiFi.status() == WL_CONNECTED) {
        timeClient.update(); //NTP call to get current time
        mTriggerUpdate = millis();
        initialTime = timeClient.getEpochTime(); // Guarda la hora inicial (en segundos desde 1970)
        Serial.print("TimeClient NTPupdateTime ");
    }
  }

  unsigned long elapsedTime = (millis() - mTriggerUpdate) / 1000; // Tiempo transcurrido en segundos
  unsigned long currentTime = initialTime + elapsedTime; // La hora actual

  // convierte la hora actual en horas, minutos y segundos
  unsigned long currentHours = currentTime % 86400 / 3600;
  unsigned long currentMinutes = currentTime % 3600 / 60;
  unsigned long currentSeconds = currentTime % 60;

  char LocalHour[10];
  sprintf(LocalHour, "%02d:%02d", currentHours, currentMinutes);
  
  String mystring(LocalHour);
  return LocalHour;
}

// Variables para controlar el parpadeo con millis()
unsigned long previousMillis = 0;

void doLedStuff(int ledPin){

  unsigned long currentMillis = millis();

  switch (mMonitor.NerdStatus) {

    case NM_waitingConfig: digitalWrite(ledPin, HIGH); // LED encendido de forma continua
                           break;

    case NM_Connecting: if (currentMillis - previousMillis >= 500) { //0.5sec blink
                            previousMillis = currentMillis;
                            digitalWrite(ledPin, !digitalRead(ledPin)); // Cambia el estado del LED
                        } 
                        break;

    case NM_hashing:    if (currentMillis - previousMillis >= 100) { //0.1sec blink
                            previousMillis = currentMillis;
                            digitalWrite(ledPin, !digitalRead(ledPin)); // Cambia el estado del LED
                        }
                        break;
  }
}

String getCurrentHashRate(unsigned long mElapsed){
  return String((1.0 * (elapsedKHs * 1000)) / mElapsed, 2);
}

mining_data getMiningData(unsigned long mElapsed){
  mining_data data;

  char best_diff_string[16] = {0};
  suffix_string(best_diff, best_diff_string, 16, 0);

  char timeMining[15] = {0}; 
  unsigned long secElapsed = millis() / 1000;
  int days = secElapsed / 86400; 
  int hours = (secElapsed - (days * 86400)) / 3600; //Number of seconds in an hour
  int mins = (secElapsed - (days * 86400) - (hours * 3600)) / 60; //Remove the number of hours and calculate the minutes.
  int secs = secElapsed - (days * 86400) - (hours * 3600) - (mins * 60);   
  sprintf(timeMining, "%01d  %02d:%02d:%02d", days, hours, mins, secs);  

  data.completedShares = shares;
  data.totalMHashes = Mhashes;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.templates = templates;
  data.bestDiff = best_diff_string;
  data.timeMining = timeMining;
  data.valids = valids;
  data.temp = String(temperatureRead(), 0);
  data.currentTime = getTime();
  
  return data;
}

clock_data getClockData(unsigned long mElapsed){
  clock_data data;

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = getBTCprice();
  data.blockHeight = getBlockHeight();
  data.currentTime = getTime();

  return data;
}

coin_data getCoinData(unsigned long mElapsed){
  coin_data data;

  updateGlobalData(); //Update gData vars asking mempool APIs

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = getBTCprice();
  data.currentTime = getTime();
  data.halfHourFee = String(gData.halfHourFee) + " sat/vB";
  data.netwrokDifficulty = gData.difficulty;
  data.globalHashRate = gData.globalHash;
  data.blockHeight = getBlockHeight();

  unsigned long currentBlock = data.blockHeight.toInt();
  unsigned long remainingBlocks = (((currentBlock / HALVING_BLOCKS)+1) * HALVING_BLOCKS) - currentBlock;
  data.progressPercent = (HALVING_BLOCKS - remainingBlocks) * 100 / HALVING_BLOCKS;
  data.remainingBlocks = String(remainingBlocks) + " BLOCKS";

  return data;
}