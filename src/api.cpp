#include "api.h"

WebServer apiServer(80);
extern TSettings Settings;
extern nvMemory nvMem;

void handleConfigGet() {
    StaticJsonDocument<512> doc;
    
    doc["pool"] = Settings.PoolAddress;
    doc["port"] = Settings.PoolPort;
    doc["password"] = Settings.PoolPassword;
    doc["address"] = Settings.BtcWallet;
    doc["timezone"] = Settings.Timezone;
    
    String response;
    serializeJson(doc, response);
    apiServer.send(200, "application/json", response);
}

void handleConfigPost() {
    if (apiServer.hasArg("plain") == false) {
        apiServer.send(400, "application/json", "{\"error\":\"Body not received\"}");
        return;
    }
    
    String body = apiServer.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        apiServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    bool changed = false;
    
    if (doc.containsKey("pool")) {
        Settings.PoolAddress = doc["pool"].as<String>();
        changed = true;
    }
    
    if (doc.containsKey("port")) {
        Settings.PoolPort = doc["port"].as<int>();
        changed = true;
    }
    
    if (doc.containsKey("password")) {
        String pass = doc["password"].as<String>();
        strncpy(Settings.PoolPassword, pass.c_str(), sizeof(Settings.PoolPassword));
        changed = true;
    }
    
    if (doc.containsKey("address")) {
        String addr = doc["address"].as<String>();
        strncpy(Settings.BtcWallet, addr.c_str(), sizeof(Settings.BtcWallet));
        changed = true;
    }

    if (doc.containsKey("timezone")) {
        Settings.Timezone = doc["timezone"].as<int>();
        changed = true;
    }
    
    if (changed) {
        nvMem.saveConfig(&Settings);
        apiServer.send(200, "application/json", "{\"success\":true, \"message\":\"Configuration saved. Restarting...\"}");
        delay(1000);
        ESP.restart();
    } else {
        apiServer.send(200, "application/json", "{\"success\":true, \"message\":\"No changes detected\"}");
    }
}

void setupAPI() {
    apiServer.on("/api/config", HTTP_GET, handleConfigGet);
    apiServer.on("/api/config", HTTP_POST, handleConfigPost);
    
    // Enable CORS
    apiServer.enableCORS(true);
    
    apiServer.begin();
    Serial.println("API Server started");
}

void api_loop() {
    apiServer.handleClient();
}
