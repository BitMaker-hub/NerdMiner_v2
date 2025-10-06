#include "nm_web.h"

#ifdef ENABLE_WEB_STATS

// Use ESPAsyncWebServer's WebRequestMethod enum values

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "monitor.h"
#include "mining.h"
#include "utils.h"

extern monitor_data mMonitor;

AsyncWebServer server(80);

// Forward declarations
String getSystemInfo();
String getMiningStats();
String getPoolStats();
String getBitcoinStats();

void setupWebServer() {
    Serial.println("Setting up web server...");
    
    // Serve the main dashboard page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = F("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>NerdMiner Dashboard</title><style>*{margin:0;padding:0;box-sizing:border-box;}body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;min-height:100vh;padding:20px;}.container{max-width:1200px;margin:0 auto;}.header{text-align:center;margin-bottom:30px;}.header h1{font-size:2.5em;margin-bottom:10px;text-shadow:2px 2px 4px rgba(0,0,0,0.3);}.status-indicator{display:inline-block;width:12px;height:12px;border-radius:50%;margin-right:8px;animation:pulse 2s infinite;}.status-online{background-color:#4CAF50;}.status-connecting{background-color:#FF9800;}.status-offline{background-color:#f44336;}@keyframes pulse{0%{opacity:1;}50%{opacity:0.5;}100%{opacity:1;}}.stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-bottom:20px;}.stat-card{background:rgba(255,255,255,0.1);border-radius:15px;padding:20px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.2);transition:transform 0.3s ease;}.stat-card:hover{transform:translateY(-5px);}.stat-card h3{margin-bottom:15px;color:#FFD700;border-bottom:2px solid #FFD700;padding-bottom:5px;}.stat-item{display:flex;justify-content:space-between;margin-bottom:10px;padding:8px 0;border-bottom:1px solid rgba(255,255,255,0.1);}.stat-label{font-weight:500;}.stat-value{font-weight:bold;color:#FFD700;}.big-stat{text-align:center;font-size:1.8em;color:#4CAF50;margin:10px 0;}.refresh-btn{position:fixed;bottom:20px;right:20px;background:#4CAF50;color:white;border:none;border-radius:50px;padding:15px 20px;cursor:pointer;font-size:16px;box-shadow:0 4px 20px rgba(0,0,0,0.3);transition:all 0.3s ease;}.refresh-btn:hover{background:#45a049;transform:scale(1.1);}.loading{text-align:center;color:#FFD700;font-style:italic;}.error{text-align:center;color:#ff6b6b;background:rgba(255,107,107,0.1);padding:10px;border-radius:5px;margin:10px 0;}@media (max-width:768px){.stats-grid{grid-template-columns:1fr;}.header h1{font-size:2em;}}</style></head><body><div class=\"container\"><div class=\"header\"><h1>⚡ NerdMiner Dashboard ⚡</h1><p><span id=\"status-indicator\" class=\"status-indicator status-online\"></span>Mining Status: <span id=\"mining-status\">Online</span></p></div><div class=\"stats-grid\"><div class=\"stat-card\"><h3>🔨 Mining Statistics</h3><div id=\"mining-stats\" class=\"loading\">Loading...</div></div><div class=\"stat-card\"><h3>💰 Bitcoin Network</h3><div id=\"bitcoin-stats\" class=\"loading\">Loading...</div></div><div class=\"stat-card\"><h3>🏊 Pool Statistics</h3><div id=\"pool-stats\" class=\"loading\">Loading...</div></div><div class=\"stat-card\"><h3>⚙️ System Information</h3><div id=\"system-info\" class=\"loading\">Loading...</div></div></div></div><button class=\"refresh-btn\" onclick=\"refreshAll()\">🔄 Refresh</button>");
        
        html += F("<script>async function fetchData(endpoint){try{const response=await fetch(endpoint);if(!response.ok)throw new Error('Network response was not ok');return await response.json();}catch(error){console.error('Fetch error:',error);return null;}}function formatStatItem(label,value,unit=''){return '<div class=\"stat-item\"><span class=\"stat-label\">'+label+':</span><span class=\"stat-value\">'+value+unit+'</span></div>';}async function updateMiningStats(){const data=await fetchData('/api/mining');const container=document.getElementById('mining-stats');if(!data){container.innerHTML='<div class=\"error\">Failed to load mining stats</div>';return;}container.innerHTML='<div class=\"big-stat\">'+data.currentHashRate+' KH/s</div>'+formatStatItem('Completed Shares',data.completedShares)+formatStatItem('Total KHashes',data.totalKHashes)+formatStatItem('Total MHashes',data.totalMHashes)+formatStatItem('Valid Blocks',data.valids)+formatStatItem('Templates',data.templates)+formatStatItem('Best Difficulty',data.bestDiff)+formatStatItem('Time Mining',data.timeMining)+formatStatItem('Temperature',data.temp,'°C');}async function updateBitcoinStats(){const data=await fetchData('/api/bitcoin');const container=document.getElementById('bitcoin-stats');if(!data){container.innerHTML='<div class=\"error\">Failed to load Bitcoin stats</div>';return;}container.innerHTML=formatStatItem('BTC Price','$'+data.btcPrice)+formatStatItem('Block Height',data.blockHeight)+formatStatItem('Network Hash Rate',data.globalHashRate)+formatStatItem('Difficulty',data.netwrokDifficulty)+formatStatItem('Next Halving',data.remainingBlocks+' blocks')+formatStatItem('Progress',Math.round(data.progressPercent*100)/100+'%')+formatStatItem('Fee (30min)',data.halfHourFee+' sat/vB');}async function updatePoolStats(){const data=await fetchData('/api/pool');const container=document.getElementById('pool-stats');if(!data){container.innerHTML='<div class=\"error\">Failed to load pool stats</div>';return;}container.innerHTML=formatStatItem('Workers Count',data.workersCount)+formatStatItem('Workers Hash Rate',data.workersHash)+formatStatItem('Best Difficulty',data.bestDifficulty);}async function updateSystemInfo(){const data=await fetchData('/api/system');const container=document.getElementById('system-info');if(!data){container.innerHTML='<div class=\"error\">Failed to load system info</div>';return;}container.innerHTML=formatStatItem('Device',data.device)+formatStatItem('Firmware',data.firmware)+formatStatItem('WiFi SSID',data.ssid)+formatStatItem('IP Address',data.ip)+formatStatItem('Signal Strength',data.rssi+' dBm')+formatStatItem('Free Heap',data.freeHeap+' bytes')+formatStatItem('Uptime',data.uptime)+formatStatItem('Last Update',data.timestamp);}async function refreshAll(){await Promise.all([updateMiningStats(),updateBitcoinStats(),updatePoolStats(),updateSystemInfo()]);}refreshAll();setInterval(refreshAll,30000);</script></body></html>");
        
        request->send(200, "text/html", html);
    });
    
    // API endpoint for mining statistics
    server.on("/api/mining", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getMiningStats());
    });
    
    // API endpoint for Bitcoin network statistics
    server.on("/api/bitcoin", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getBitcoinStats());
    });
    
    // API endpoint for pool statistics
    server.on("/api/pool", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getPoolStats());
    });
    
    // API endpoint for system information
    server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", getSystemInfo());
    });
    
    // Handle 404 errors
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });
    
    server.begin();
    Serial.println("Web server started on port 80");
    Serial.print("Dashboard available at: http://");
    Serial.println(WiFi.localIP());
}

void handleWebServerClient() {
    // AsyncWebServer handles requests automatically
    // This function kept for compatibility but not needed
}

String getMiningStats() {
    mining_data data = getMiningData(millis());
    
    DynamicJsonDocument doc(1024);
    doc["completedShares"] = data.completedShares;
    doc["totalMHashes"] = data.totalMHashes;
    doc["totalKHashes"] = data.totalKHashes;
    doc["currentHashRate"] = data.currentHashRate;
    doc["templates"] = data.templates;
    doc["bestDiff"] = data.bestDiff;
    doc["timeMining"] = data.timeMining;
    doc["valids"] = data.valids;
    doc["temp"] = data.temp;
    doc["currentTime"] = data.currentTime;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String getBitcoinStats() {
    coin_data data = getCoinData(millis());
    
    DynamicJsonDocument doc(1024);
    doc["btcPrice"] = data.btcPrice;
    doc["blockHeight"] = data.blockHeight;
    doc["globalHashRate"] = data.globalHashRate;
    doc["netwrokDifficulty"] = data.netwrokDifficulty;
    doc["remainingBlocks"] = data.remainingBlocks;
    doc["progressPercent"] = data.progressPercent;
    doc["halfHourFee"] = data.halfHourFee;
    doc["currentTime"] = data.currentTime;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String getPoolStats() {
    pool_data data = getPoolData();
    
    DynamicJsonDocument doc(512);
    doc["workersCount"] = data.workersCount;
    doc["workersHash"] = data.workersHash;
    doc["bestDifficulty"] = data.bestDifficulty;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String getSystemInfo() {
    DynamicJsonDocument doc(1024);
    
    unsigned long uptimeMs = millis();
    unsigned long uptimeSeconds = uptimeMs / 1000;
    unsigned long hours = uptimeSeconds / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    unsigned long seconds = uptimeSeconds % 60;
    
    String uptimeStr = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
    
    doc["device"] = "ESP32-WROOM Headless";
    doc["firmware"] = "NerdMiner v2 (Headless)";
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = uptimeStr;
    doc["timestamp"] = String(millis() / 1000);
    
    String output;
    serializeJson(doc, output);
    return output;
}

#endif // ENABLE_WEB_STATS