#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "stratum.h"
#include "config.h"
#include <string.h>

MinerJob current_stratum_job;
ShareSubmission current_submission = {"", "", "", 0, false};
volatile bool is_stratum_connected = false;

static WiFiClient client;
static String extraNonce1 = "";
static uint32_t extraNonce2_counter = 0;
static double pool_difficulty = 0.0001;

static void hex_to_bin(const char* hex, uint8_t* bin, size_t bin_len) {
    for (size_t i = 0; i < bin_len; i++) {
        sscanf(hex + 2 * i, "%02hhx", &bin[i]);
    }
}

void init_wifi() {
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

static void build_job_from_notify(JsonArray& p) {
    const char* job_id = p[0];
    const char* prevhash = p[1];
    const char* coinb1 = p[2];
    const char* coinb2 = p[3];
    JsonArray merkle_branch = p[4];
    const char* version = p[5];
    const char* nbits = p[6];
    const char* ntime = p[7];

    char en2_hex[16] = {0};
    snprintf(en2_hex, sizeof(en2_hex), "%08x", extraNonce2_counter++);

    size_t cb1_len = strlen(coinb1) / 2;
    size_t en1_len = extraNonce1.length() / 2;
    size_t en2_len = strlen(en2_hex) / 2;
    size_t cb2_len = strlen(coinb2) / 2;
    size_t cb_total = cb1_len + en1_len + en2_len + cb2_len;
    
    uint8_t coinbase[256];
    hex_to_bin(coinb1, coinbase, cb1_len);
    hex_to_bin(extraNonce1.c_str(), coinbase + cb1_len, en1_len);
    hex_to_bin(en2_hex, coinbase + cb1_len + en1_len, en2_len);
    hex_to_bin(coinb2, coinbase + cb1_len + en1_len + en2_len, cb2_len);

    uint8_t merkle_root[32];
    sha256d_full(coinbase, cb_total, merkle_root);

    for (size_t i = 0; i < merkle_branch.size(); i++) {
        uint8_t combined[64];
        memcpy(combined, merkle_root, 32);
        hex_to_bin(merkle_branch[i], combined + 32, 32);
        sha256d_full(combined, 64, merkle_root);
    }

    uint8_t header[80] = {0};
    hex_to_bin(version, header, 4);
    
    uint8_t raw_prev[32];
    hex_to_bin(prevhash, raw_prev, 32);
    for (int i = 0; i < 8; i++) {
        header[4 + i * 4 + 0] = raw_prev[i * 4 + 3];
        header[4 + i * 4 + 1] = raw_prev[i * 4 + 2];
        header[4 + i * 4 + 2] = raw_prev[i * 4 + 1];
        header[4 + i * 4 + 3] = raw_prev[i * 4 + 0];
    }
    memcpy(header + 36, merkle_root, 32);
    hex_to_bin(ntime, header + 68, 4);
    hex_to_bin(nbits, header + 72, 4);

    strncpy(current_stratum_job.job_id, job_id, sizeof(current_stratum_job.job_id));
    strncpy(current_stratum_job.extranonce2, en2_hex, sizeof(current_stratum_job.extranonce2));
    strncpy(current_stratum_job.ntime, ntime, sizeof(current_stratum_job.ntime));
    
    sha256_precompute_midstate(header, current_stratum_job.midstate);
    
    memset(current_stratum_job.chunk2, 0, sizeof(current_stratum_job.chunk2));
    current_stratum_job.chunk2[0] = ((uint32_t)header[64] << 24) | ((uint32_t)header[65] << 16) | ((uint32_t)header[66] << 8) | header[67];
    current_stratum_job.chunk2[1] = ((uint32_t)header[68] << 24) | ((uint32_t)header[69] << 16) | ((uint32_t)header[70] << 8) | header[71];
    current_stratum_job.chunk2[2] = ((uint32_t)header[72] << 24) | ((uint32_t)header[73] << 16) | ((uint32_t)header[74] << 8) | header[75];
    current_stratum_job.chunk2[4] = 0x80000000;
    current_stratum_job.chunk2[15] = 640;
    
    double target_val = 65535.0 / pool_difficulty;
    current_stratum_job.target_high = (target_val > 4294967295.0) ? 0xFFFFFFFF : (uint32_t)target_val;
}

void stratum_task(void* pvParameters) {
    while (true) {
        if (!client.connected()) {
            is_stratum_connected = false;
            if (!client.connect(STRATUM_HOST, STRATUM_PORT)) {
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                continue;
            }
            client.print("{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": [\"NerdMiner/2.0\"]}\n");
            String sub_res = client.readStringUntil('\n');
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, sub_res);
            extraNonce1 = doc["result"][1].as<String>();
            
            char auth_req[256];
            snprintf(auth_req, sizeof(auth_req), "{\"id\": 2, \"method\": \"mining.authorize\", \"params\": [\"%s.%s\", \"x\"]}\n", BTC_ADDRESS, WORKER_NAME);
            client.print(auth_req);
            is_stratum_connected = true;
        }

        while (client.available()) {
            String line = client.readStringUntil('\n');
            DynamicJsonDocument doc(4096);
            if (deserializeJson(doc, line) == DeserializationError::Ok) {
                if (doc.containsKey("method")) {
                    String method = doc["method"].as<String>();
                    if (method == "mining.set_difficulty") {
                        pool_difficulty = doc["params"][0].as<double>();
                    } else if (method == "mining.notify") {
                        build_job_from_notify(doc["params"].as<JsonArray>());
                    }
                }
            }
        }

        if (current_submission.ready) {
            char sub_buf[256];
            snprintf(sub_buf, sizeof(sub_buf), "{\"id\": 4, \"method\": \"mining.submit\", \"params\": [\"%s.%s\", \"%s\", \"%s\", \"%s\", \"%08x\"]}\n", 
                     BTC_ADDRESS, WORKER_NAME, current_submission.job_id, current_submission.extranonce2, current_submission.ntime, current_submission.nonce);
            client.print(sub_buf);
            current_submission.ready = false;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
