#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include "media/Free_Fonts.h"
#include "media/images.h"
#include "mbedtls/md.h"
#include "OpenFontRender.h"
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

extern unsigned long halfshares; // increase if blockhash has 16 bits of zeroes
extern unsigned int shares; // increase if blockhash has 32 bits of zeroes
extern unsigned int valids; // increased if blockhash <= targethalfshares

extern OpenFontRender render;
extern TFT_eSprite background;
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

void changeScreen(void){
    mMonitor.screen++;
    if(mMonitor.screen> SCREEN_GLOBAL) mMonitor.screen = SCREEN_MINING;
}
void show_MinerScreen(unsigned long mElapsed){

    //Print background screen
    background.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen); 

    char CurrentHashrate[10] = {0};
    sprintf(CurrentHashrate, "%.2f", (1.0*(elapsedKHs*1000))/mElapsed);

    //Serial.println("[runMonitor Task] -> Printing results on screen ");
    
     Serial.printf(">>> Completed %d share(s), %d Khashes, avg. hashrate %s KH/s\n",
      shares, totalKHashes, CurrentHashrate);

    //Hashrate
    render.setFontSize(70);
    render.setCursor(19, 118);
    render.setFontColor(TFT_BLACK);
    
    render.rdrawString(CurrentHashrate, 118, 114, TFT_BLACK);
    //Total hashes
    render.setFontSize(36);
    render.rdrawString(String(Mhashes).c_str(), 268, 138, TFT_BLACK);
    //Block templates
    render.setFontSize(36);
    render.drawString(String(templates).c_str(), 186, 20, 0xDEDB);
    //16Bit shares
    render.setFontSize(36);
    render.drawString(String(halfshares).c_str(), 186, 48, 0xDEDB);
    //32Bit shares
    render.setFontSize(36);
    render.drawString(String(shares).c_str(), 186, 76, 0xDEDB);
    //Hores
    char timeMining[15]; 

    unsigned long secElapsed = millis() / 1000;
    int days = secElapsed / 86400; 
    int hours = (secElapsed - (days * 86400)) / 3600;                                                        //Number of seconds in an hour
    int mins = (secElapsed - (days * 86400) - (hours * 3600)) / 60;                                              //Remove the number of hours and calculate the minutes.
    int secs = secElapsed - (days * 86400) - (hours * 3600) - (mins * 60);   
    sprintf(timeMining, "%01d  %02d:%02d:%02d", days, hours, mins, secs);
    render.setFontSize(27);
    render.rdrawString(String(timeMining).c_str(), 315, 104, 0xDEDB);

    //Valid Blocks
    render.setFontSize(48);
    render.drawString(String(valids).c_str(), 285, 56, 0xDEDB);

    //Print Temp
    String temp = String(temperatureRead(), 0);
    render.setFontSize(20);
    render.rdrawString(String(temp).c_str(), 239, 1, TFT_BLACK);

    render.setFontSize(7);
    render.rdrawString(String(0).c_str(), 244, 3, TFT_BLACK);

    //Print Hour
    render.setFontSize(20);
    render.rdrawString(getTime().c_str(), 286, 1, TFT_BLACK);

    // pool url
    /*background.setTextSize(1);
    background.setTextDatum(MC_DATUM);
    background.setTextColor(0xDEDB);
    background.drawString(String(poolString), 59, 85, FONT2);*/

    //Push prepared background to screen
    background.pushSprite(0,0);
}


void show_ClockScreen(unsigned long mElapsed){

    //Print background screen
    background.pushImage(0, 0, minerClockWidth, minerClockHeight, minerClockScreen); 

    char CurrentHashrate[10] = {0};
    sprintf(CurrentHashrate, "%.2f", (1.0*(elapsedKHs*1000))/mElapsed);

    //Serial.println("[runMonitor Task] -> Printing results on screen ");
    
     Serial.printf(">>> Completed %d share(s), %d Khashes, avg. hashrate %s KH/s\n",
      shares, totalKHashes, CurrentHashrate);

    //Hashrate
    render.setFontSize(50);
    render.setCursor(19, 122);
    render.setFontColor(TFT_BLACK);
    
    render.rdrawString(CurrentHashrate, 94, 129, TFT_BLACK);

    //Print BTC Price
    //render.setFontSize(22);
    //render.drawString(getBTCprice().c_str(), 202, 3, TFT_BLACK);
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(getBTCprice().c_str(), 202, 3, GFXFF);

    //Print BlockHeight
    render.setFontSize(36);
    render.rdrawString(getBlockHeight().c_str(), 254, 140, TFT_BLACK);

    //Print Hour
    background.setFreeFont(FF23);
    background.setTextSize(2);
    background.setTextColor(0xDEDB, TFT_BLACK);
    
    //background.setTexSize(2);
    background.drawString(getTime().c_str(), 130, 50, GFXFF);
    //render.setFontColor(TFT_WHITE);
    //render.setFontSize(110);
    //render.rdrawString(getTime().c_str(), 290, 40, TFT_WHITE);

    //Push prepared background to screen
    background.pushSprite(0,0);
}

void show_GlobalHashScreen(unsigned long mElapsed){

    //Print background screen
    background.pushImage(0, 0, globalHashWidth, globalHashHeight, globalHashScreen); 

    char CurrentHashrate[10] = {0};
    sprintf(CurrentHashrate, "%.2f", (1.0*(elapsedKHs*1000))/mElapsed);

    //Serial.println("[runMonitor Task] -> Printing results on screen ");
    
     Serial.printf(">>> Completed %d share(s), %d Khashes, avg. hashrate %s KH/s\n",
      shares, totalKHashes, CurrentHashrate);

    //Hashrate
    updateGlobalData(); //Update gData vars asking mempool APIs
    
    //Print BTC Price
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(getBTCprice().c_str(), 198, 3, GFXFF);

    //Print Hour
    background.setFreeFont(FSSB9);
    background.setTextSize(1);
    background.setTextDatum(TL_DATUM);
    background.setTextColor(TFT_BLACK);
    background.drawString(getTime().c_str(), 268, 3, GFXFF);

    //Print Last Pool Block
    background.setFreeFont(FSS9);
    background.setTextDatum(TR_DATUM);
    background.setTextColor(0x9C92);
    String temp = String(gData.halfHourFee) + " sat/vB";
    background.drawString(temp.c_str(), 302, 52, GFXFF);

    //Print Difficulty
    background.setFreeFont(FSS9);
    background.setTextDatum(TR_DATUM);
    background.setTextColor(0x9C92);
    background.drawString(gData.difficulty.c_str(), 302, 88, GFXFF);

    //Print Global Hashrate
    render.setFontSize(34);
    render.rdrawString(gData.globalHash.c_str(), 274, 145, TFT_BLACK);

    //Print BlockHeight
    render.setFontSize(55);
    gData.currentBlock = getBlockHeight();
    render.rdrawString(gData.currentBlock.c_str(), 140, 104, 0xDEDB);

    //Draw percentage rectangle
    //width percent bar 140 - 2
    unsigned long cBlock = gData.currentBlock.toInt();
    gData.remainingBlocks = (((cBlock / HALVING_BLOCKS)+1) * HALVING_BLOCKS) - cBlock;
    gData.progressPercent = (HALVING_BLOCKS-gData.remainingBlocks)*100/HALVING_BLOCKS;
    int x2 = 2 + (138*gData.progressPercent/100);
    background.fillRect(2, 149, x2, 168, 0xDEDB);

    //Print Remaining BLocks
    //background.setFreeFont(FSSB9);
    background.setTextFont(FONT2);
    background.setTextSize(1);
    background.setTextDatum(MC_DATUM);
    background.setTextColor(TFT_BLACK);
    temp = String(gData.remainingBlocks) + " BLOCKS";
    background.drawString(temp.c_str(), 72, 159, FONT2);//GFXFF);

    //Push prepared background to screen
    background.pushSprite(0,0);
}