#include <Arduino.h>
#include <WiFi.h>
#include "mbedtls/md.h"
#include "HTTPClient.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <list>
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"

extern uint32_t templates;
extern uint32_t hashes;
extern uint32_t Mhashes;
extern uint32_t totalKHashes;
extern uint32_t elapsedKHs;
extern uint64_t upTime;

extern uint32_t shares; // increase if blockhash has 32 bits of zeroes
extern uint32_t valids; // increased if blockhash <= targethalfshares

extern double best_diff; // track best diff

extern monitor_data mMonitor;

//from saved config
extern TSettings Settings; 
bool invertColors = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
unsigned int bitcoin_price=0;
String current_block = "793261";
global_data gData;
pool_data pData;
String poolAPIUrl;

int BTCpriceHistoryMinutely[BTC_PRICE_HISTORY_SIZE];
int priceHistoryMinutelyIndex = 0;

int BTCpriceHistory5Min[BTC_PRICE_HISTORY_SIZE];
int priceHistory5MinIndex = 0;

int BTCpriceHistory30Min[BTC_PRICE_HISTORY_SIZE];
int priceHistory30MinIndex = 0;

int BTCpriceHistoryWeekly[BTC_PRICE_HISTORY_SIZE];
int priceHistoryWeeklyIndex = 0;

void setup_monitor(void){
    /******** TIME ZONE SETTING *****/

    timeClient.begin();
    
    // Adjust offset depending on your zone
    // GMT +2 in seconds (zona horaria de Europa Central)
    timeClient.setTimeOffset(3600 * Settings.Timezone);

    Serial.println("TimeClient setup done");
#ifdef SCREEN_WORKERS_ENABLE
    poolAPIUrl = getPoolAPIUrl();
    Serial.println("poolAPIUrl: " + poolAPIUrl);
#endif

    for (int i = 0; i < BTC_PRICE_HISTORY_SIZE; i++) {
      BTCpriceHistoryMinutely[i] = 0;
      BTCpriceHistory5Min[i] = 0;
      BTCpriceHistory30Min[i] = 0;
      BTCpriceHistoryWeekly[i] = 0;
    }

    // Get current price - runMonitor() will call this every minute
    saveBTCpriceHistory();
}

unsigned long mGlobalUpdate =0;

void updateGlobalData(void){
    
    if((mGlobalUpdate == 0) || (millis() - mGlobalUpdate > UPDATE_Global_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) return;
            
        //Make first API call to get global hash and current difficulty
        HTTPClient http;
        http.setTimeout(10000);
        try {
        http.begin(getGlobalHash);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            StaticJsonDocument<1024> doc;
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

      
        //Make third API call to get fees
        http.begin(getFees);
        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, payload);
            String temp = "";
            if (doc.containsKey("halfHourFee")) gData.halfHourFee = doc["halfHourFee"].as<int>();
#ifdef SCREEN_FEES_ENABLE
            if (doc.containsKey("fastestFee"))  gData.fastestFee = doc["fastestFee"].as<int>();
            if (doc.containsKey("hourFee"))     gData.hourFee = doc["hourFee"].as<int>();
            if (doc.containsKey("economyFee"))  gData.economyFee = doc["economyFee"].as<int>();
            if (doc.containsKey("minimumFee"))  gData.minimumFee = doc["minimumFee"].as<int>();
#endif
            doc.clear();

            mGlobalUpdate = millis();
        }
        
        http.end();
        } catch(...) {
          Serial.println("Global data HTTP error caught");
          http.end();
        }
    }
}

unsigned long mHeightUpdate = 0;

String getBlockHeight(void){
    
    if((mHeightUpdate == 0) || (millis() - mHeightUpdate > UPDATE_Height_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) return current_block;
            
        HTTPClient http;
        http.setTimeout(10000);
        try {
        http.begin(getHeightAPI);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            payload.trim();

            current_block = payload;

            mHeightUpdate = millis();
        }        
        http.end();
        } catch(...) {
          Serial.println("Height HTTP error caught");
          http.end();
        }
    }
  
  return current_block;
}

unsigned long mBTCUpdate = 0;

int getBTCprice(void){
    
    if((mBTCUpdate == 0) || (millis() - mBTCUpdate > UPDATE_BTC_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) {
            return bitcoin_price;
        }
        
        HTTPClient http;
        http.setTimeout(10000);
        bool priceUpdated = false;

        try {
        http.begin(getBTCAPI);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            StaticJsonDocument<1024> doc;
            deserializeJson(doc, payload);
          
            if (doc.containsKey("bitcoin") && doc["bitcoin"].containsKey("usd")) {
                bitcoin_price = doc["bitcoin"]["usd"];

                Serial.println(">>>> ===== >>>> new BTC price:");
                Serial.println(bitcoin_price);
                Serial.println(">>>> ===== >>>> new BTC price");
            }

            doc.clear();

            mBTCUpdate = millis();
        }
        
        http.end();
        } catch(...) {
          Serial.println("BTC price HTTP error caught");
          http.end();
        }
    }  
  
  return bitcoin_price;
}

unsigned long mTriggerUpdate = 0;
unsigned long initialMillis = millis();
unsigned long initialTime = 0;
unsigned long mPoolUpdate = 0;

void getTime(unsigned long* currentHours, unsigned long* currentMinutes, unsigned long* currentSeconds){
  
  //Check if need an NTP call to check current time
  if((mTriggerUpdate == 0) || (millis() - mTriggerUpdate > UPDATE_PERIOD_h * 60 * 60 * 1000)){ //60 sec. * 60 min * 1000ms
    if(WiFi.status() == WL_CONNECTED) {
        if(timeClient.update()) mTriggerUpdate = millis(); //NTP call to get current time
        initialTime = timeClient.getEpochTime(); // Guarda la hora inicial (en segundos desde 1970)
        Serial.print("TimeClient NTPupdateTime ");
    }
  }

  unsigned long elapsedTime = (millis() - mTriggerUpdate) / 1000; // Tiempo transcurrido en segundos
  unsigned long currentTime = initialTime + elapsedTime; // La hora actual

  // convierte la hora actual en horas, minutos y segundos
  *currentHours = currentTime % 86400 / 3600;
  *currentMinutes = currentTime % 3600 / 60;
  *currentSeconds = currentTime % 60;
}

unsigned long getNow()
{
  unsigned long elapsedTime = (millis() - mTriggerUpdate) / 1000; // Tiempo transcurrido en segundos
  unsigned long currentTime = initialTime + elapsedTime; // La hora actual

  return currentTime;
}

String getDate(){
  unsigned long currentTime = getNow();

  // Convierte la hora actual (epoch time) en una estructura tm
  struct tm *tm = localtime((time_t *)&currentTime);

  int year = tm->tm_year + 1900; // tm_year es el número de años desde 1900
  int month = tm->tm_mon + 1;    // tm_mon es el mes del año desde 0 (enero) hasta 11 (diciembre)
  int day = tm->tm_mday;         // tm_mday es el día del mes

  char currentDate[20];
  sprintf(currentDate, "%02d/%02d/%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);

  return String(currentDate);
}

String getTime(void){
  unsigned long currentHours, currentMinutes, currentSeconds;
  getTime(&currentHours, &currentMinutes, &currentSeconds);

  char LocalHour[10];
  sprintf(LocalHour, "%02d:%02d", currentHours, currentMinutes);
  
  String mystring(LocalHour);
  return LocalHour;
}

enum EHashRateScale
{
  HashRateScale_99KH,
  HashRateScale_999KH,
  HashRateScale_9MH
};

static EHashRateScale s_hashrate_scale = HashRateScale_99KH;
static uint32_t s_skip_first = 3;
static double s_top_hashrate = 0.0;

static std::list<double> s_hashrate_avg_list;
static double s_hashrate_summ = 0.0;
static uint8_t s_hashrate_recalc = 0;

String getCurrentHashRate(unsigned long mElapsed)
{
  double hashrate = (double)elapsedKHs * 1000.0 / (double)mElapsed;

  s_hashrate_summ += hashrate;
  s_hashrate_avg_list.push_back(hashrate);
  if (s_hashrate_avg_list.size() > 10)
  {
    s_hashrate_summ -= s_hashrate_avg_list.front();
    s_hashrate_avg_list.pop_front();
  }

  ++s_hashrate_recalc;
  if (s_hashrate_recalc == 0)
  {
    s_hashrate_summ = 0.0;
    for (auto itt = s_hashrate_avg_list.begin(); itt != s_hashrate_avg_list.end(); ++itt)
      s_hashrate_summ += *itt;
  }

  double avg_hashrate = s_hashrate_summ / (double)s_hashrate_avg_list.size();
  if (avg_hashrate < 0.0)
    avg_hashrate = 0.0;

  if (s_skip_first > 0)
  {
    s_skip_first--;
  } else
  {
    if (avg_hashrate > s_top_hashrate)
    {
      s_top_hashrate = avg_hashrate;
      if (avg_hashrate > 999.9)
        s_hashrate_scale = HashRateScale_9MH;
      else if (avg_hashrate > 99.9)
        s_hashrate_scale = HashRateScale_999KH;
    }
  }

  switch (s_hashrate_scale)
  {
    case HashRateScale_99KH:
      return String(avg_hashrate, 2);
    case HashRateScale_999KH:
      return String(avg_hashrate, 1);
    default:
      return String((int)avg_hashrate );
  }
}

mining_data getMiningData(unsigned long mElapsed)
{
  mining_data data;

  char best_diff_string[16] = {0};
  suffix_string(best_diff, best_diff_string, 16, 0);

  char timeMining[15] = {0};
  uint64_t tm = upTime;
  int secs = tm % 60;
  tm /= 60;
  int mins = tm % 60;
  tm /= 60;
  int hours = tm % 24;
  int days = tm / 24;
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

void initFAKEdata(int actualPrice)
{
  int price1 = actualPrice;
  int price2 = actualPrice;
  int price3 = actualPrice;
  int price4 = actualPrice;

  for (int i = BTC_PRICE_HISTORY_SIZE - 1; i >= 0; i--) {
    BTCpriceHistoryMinutely[i] = price1;
    price1 += (rand() % 100) - 50;

    BTCpriceHistory5Min[i] = price2;
    price2 += (rand() % 100) - 50;

    BTCpriceHistory30Min[i] = price3;
    price3 += (rand() % 100) - 50;

    BTCpriceHistoryWeekly[i] = price4;
    price4 += (rand() % 100) - 50;
  }

  // Enable all 4 graphs
  priceHistoryMinutelyIndex = BTC_PRICE_HISTORY_SIZE - 1;
  priceHistory5MinIndex     = BTC_PRICE_HISTORY_SIZE - 1;
  priceHistory30MinIndex    = BTC_PRICE_HISTORY_SIZE - 1;
  priceHistoryWeeklyIndex   = BTC_PRICE_HISTORY_SIZE - 1;
}

void saveBTCpriceHistory()
{
  int price = getBTCprice();

  if (BTCpriceHistoryMinutely[0] == 0) {
    // Init all history data to current price
    for (int i=0; i<BTC_PRICE_HISTORY_SIZE; i++) {
      BTCpriceHistoryMinutely[i] = price;
      BTCpriceHistory5Min[i] = price;
      BTCpriceHistory30Min[i] = price;
      BTCpriceHistoryWeekly[i] = price;
    }
    // initFAKEdata(price);
  } else {
    BTCpriceHistoryMinutely[priceHistoryMinutelyIndex] = price;
  }

  if (priceHistoryMinutelyIndex % 5 == 0) {
    // Consolidate last 5 minutes data to create a 5-minute average data point.
    // This data is primarily used for the "Day" graph.
    int average = 0;
    for (int minutesAgo=0; minutesAgo<5; minutesAgo++) {
      int p = getBTCpriceHistory(minutesAgo, BTC_PRICE_HISTORY_GRAPH_MIN);
      average += p;
    }
    average /= 5;
    BTCpriceHistory5Min[priceHistory5MinIndex] = average;

    if (priceHistory5MinIndex % 6 == 0) {
      // Consolidate last 30 minutes data (6x 5-minute averages) to create a 30-minute average data point.
      // This data is primarily used for the "Week" graph.
      int average = 0;
      for (int min5Ago=0; min5Ago<6; min5Ago++) {
        int p = getBTCpriceHistory(min5Ago, BTC_PRICE_HISTORY_GRAPH_5MIN);
        average += p;
      }
      average /= 6;
      BTCpriceHistory30Min[priceHistory30MinIndex] = average;

      if (priceHistory30MinIndex % 4 == 0) { // Every 2 hours (4x 30-minute averages)
        // Consolidate last 2 hours data (4x 30-minute averages) to create a 2-hour average data point.
        // This data is primarily used for the "Month" graph (currently named "Weekly").
        int average = 0;
        for (int min30Ago=0; min30Ago<4; min30Ago++) {
          int p = getBTCpriceHistory(min30Ago, BTC_PRICE_HISTORY_GRAPH_30MIN);
          average += p;
        }
        average /= 4;
        BTCpriceHistoryWeekly[priceHistoryWeeklyIndex] = average;
        priceHistoryWeeklyIndex = (priceHistoryWeeklyIndex + 1) % BTC_PRICE_HISTORY_SIZE;
      }
      
      priceHistory30MinIndex = (priceHistory30MinIndex + 1) % BTC_PRICE_HISTORY_SIZE;
    }

    priceHistory5MinIndex = (priceHistory5MinIndex + 1) % BTC_PRICE_HISTORY_SIZE;
  }

  priceHistoryMinutelyIndex = (priceHistoryMinutelyIndex + 1) % BTC_PRICE_HISTORY_SIZE;
}

String getBTCpriceHistoryName(int graphID)
{
  switch (graphID) {
  case BTC_PRICE_HISTORY_GRAPH_MIN:    return "5h";
  case BTC_PRICE_HISTORY_GRAPH_5MIN:   return "Day";
  case BTC_PRICE_HISTORY_GRAPH_30MIN:  return "Week";
  case BTC_PRICE_HISTORY_GRAPH_WEEKLY: return "Month";
  default: return "??";
  }
}

unsigned long getBTCpriceLegendSecsAgo(int graphID)
{
  switch (graphID) {
  case BTC_PRICE_HISTORY_GRAPH_MIN:    return 3600;
  case BTC_PRICE_HISTORY_GRAPH_5MIN:   return (5 * 3600);
  case BTC_PRICE_HISTORY_GRAPH_30MIN:  return ((5 * 3600) * 6);
  case BTC_PRICE_HISTORY_GRAPH_WEEKLY: return (((5 * 3600) * 6) * 4);
  }

  return 0; // Error!
}

int getBTCpriceHistoryIndex(int graphID)
{
  switch (graphID) {
  case BTC_PRICE_HISTORY_GRAPH_MIN:    return priceHistoryMinutelyIndex;
  case BTC_PRICE_HISTORY_GRAPH_5MIN:   return priceHistory5MinIndex;
  case BTC_PRICE_HISTORY_GRAPH_30MIN:  return priceHistory30MinIndex;
  case BTC_PRICE_HISTORY_GRAPH_WEEKLY: return priceHistoryWeeklyIndex;
  }

  // Error!
  Serial.println("===>>> invalid graphID");
  Serial.println(graphID);
  Serial.println("===>>> invalid graphID");
  return 0;
}

int getBTCpriceHistory(int stepAgo, int graphID)
{
  if (stepAgo == 0) return bitcoin_price;

  if (stepAgo >= BTC_PRICE_HISTORY_SIZE) stepAgo = BTC_PRICE_HISTORY_SIZE - 1;

  int priceIndex = getBTCpriceHistoryIndex(graphID) - stepAgo;
  if (priceIndex < 0)
    priceIndex = priceIndex + BTC_PRICE_HISTORY_SIZE;

  int price = 0;
  switch (graphID) {
  case BTC_PRICE_HISTORY_GRAPH_MIN:    price = BTCpriceHistoryMinutely[priceIndex]; break;
  case BTC_PRICE_HISTORY_GRAPH_5MIN:   price = BTCpriceHistory5Min[priceIndex];     break;
  case BTC_PRICE_HISTORY_GRAPH_30MIN:  price = BTCpriceHistory30Min[priceIndex];    break;
  case BTC_PRICE_HISTORY_GRAPH_WEEKLY: price = BTCpriceHistoryWeekly[priceIndex];    break;
  }

  return price;
}

clock_data getClockData(unsigned long mElapsed)
{
  clock_data data;

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = String("$") + String(bitcoin_price);
  data.blockHeight = getBlockHeight();
  data.currentTime = getTime();
  data.currentDate = getDate();

  return data;
}

clock_data_t getClockData_t(unsigned long mElapsed)
{
  clock_data_t data;

  data.valids = valids;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  getTime(&data.currentHours, &data.currentMinutes, &data.currentSeconds);

  return data;
}

coin_data getCoinData(unsigned long mElapsed)
{
  coin_data data;

  updateGlobalData(); // Update gData vars asking mempool APIs

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = String("$") + String(bitcoin_price);
  data.currentTime = getTime();
#ifdef SCREEN_FEES_ENABLE
  data.hourFee = String(gData.hourFee);
  data.fastestFee = String(gData.fastestFee);
  data.economyFee = String(gData.economyFee);
  data.minimumFee = String(gData.minimumFee);
#endif
  data.halfHourFee = String(gData.halfHourFee) + " sat/vB";
  data.netwrokDifficulty = gData.difficulty;
  data.globalHashRate = gData.globalHash;
  data.blockHeight = getBlockHeight();

  unsigned long currentBlock = data.blockHeight.toInt();
  unsigned long remainingBlocks = (((currentBlock / HALVING_BLOCKS) + 1) * HALVING_BLOCKS) - currentBlock;
  data.progressPercent = (HALVING_BLOCKS - remainingBlocks) * 100 / HALVING_BLOCKS;
  data.remainingBlocks = String(remainingBlocks) + " BLOCKS";

  return data;
}

String getPoolAPIUrl(void) {
    poolAPIUrl = String(getPublicPool);
    if (Settings.PoolAddress == "public-pool.io") {
        poolAPIUrl = "https://public-pool.io:40557/api/client/";
    } 
    else {
        if (Settings.PoolAddress == "pool.nerdminers.org") {
            poolAPIUrl = "https://pool.nerdminers.org/users/";
        }
        else {
            switch (Settings.PoolPort) {
                case 3333:
                    if (Settings.PoolAddress == "pool.sethforprivacy.com")
                        poolAPIUrl = "https://pool.sethforprivacy.com/api/client/";
                    if (Settings.PoolAddress == "pool.solomining.de")
                        poolAPIUrl = "https://pool.solomining.de/api/client/";
                    // Add more cases for other addresses with port 3333 if needed
                    break;
                case 2018:
                    // Local instance of public-pool.io on Umbrel or Start9
                    poolAPIUrl = "http://" + Settings.PoolAddress + ":2019/api/client/";
                    break;
                default:
                    poolAPIUrl = String(getPublicPool);
                    break;
            }
        }
    }
    return poolAPIUrl;
}

pool_data getPoolData(void){
    //pool_data pData;    
    if((mPoolUpdate == 0) || (millis() - mPoolUpdate > UPDATE_POOL_min * 60 * 1000)){      
        if (WiFi.status() != WL_CONNECTED) return pData;            
        //Make first API call to get global hash and current difficulty
        HTTPClient http;
        http.setTimeout(10000);        
        try {          
          String btcWallet = Settings.BtcWallet;
          // Serial.println(btcWallet);
          if (btcWallet.indexOf(".")>0) btcWallet = btcWallet.substring(0,btcWallet.indexOf("."));
#ifdef SCREEN_WORKERS_ENABLE
          Serial.println("Pool API : " + poolAPIUrl+btcWallet);
          http.begin(poolAPIUrl+btcWallet);
#else
          http.begin(String(getPublicPool)+btcWallet);
#endif
          int httpCode = http.GET();
          if (httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              // Serial.println(payload);
              StaticJsonDocument<300> filter;
              filter["bestDifficulty"] = true;
              filter["workersCount"] = true;
              filter["workers"][0]["sessionId"] = true;
              filter["workers"][0]["hashRate"] = true;
              StaticJsonDocument<2048> doc;
              deserializeJson(doc, payload, DeserializationOption::Filter(filter));
              //Serial.println(serializeJsonPretty(doc, Serial));
              if (doc.containsKey("workersCount")) pData.workersCount = doc["workersCount"].as<int>();
              const JsonArray& workers = doc["workers"].as<JsonArray>();
              float totalhashs = 0;
              for (const JsonObject& worker : workers) {
                totalhashs += worker["hashRate"].as<double>();
                /* Serial.print(worker["sessionId"].as<String>()+": ");
                Serial.print(" - "+worker["hashRate"].as<String>()+": ");
                Serial.println(totalhashs); */
              }
              char totalhashs_s[16] = {0};
              suffix_string(totalhashs, totalhashs_s, 16, 0);
              pData.workersHash = String(totalhashs_s);

              double temp;
              if (doc.containsKey("bestDifficulty")) {
              temp = doc["bestDifficulty"].as<double>();            
              char best_diff_string[16] = {0};
              suffix_string(temp, best_diff_string, 16, 0);
              pData.bestDifficulty = String(best_diff_string);
              }
              doc.clear();
              mPoolUpdate = millis();
              Serial.println("\n####### Pool Data OK!");               
          } else {
              Serial.println("\n####### Pool Data HTTP Error!");    
              /* Serial.println(httpCode);
              String payload = http.getString();
              Serial.println(payload); */
              // mPoolUpdate = millis();
              pData.bestDifficulty = "P";
              pData.workersHash = "E";
              pData.workersCount = 0;
              http.end();
              return pData; 
          }
          http.end();
        } catch(...) {
          Serial.println("####### Pool Error!");          
          // mPoolUpdate = millis();
          pData.bestDifficulty = "P";
          pData.workersHash = "Error";
          pData.workersCount = 0;
          http.end();
          return pData;
        } 
    }
    return pData;
}
