#include <stdint.h>
#include <vector>
#pragma once

struct I2cSlaveHarvest
{
    uint8_t address;
    uint32_t nonce;
    uint32_t processed_nonce;
    bool has_nonce;
};

struct I2cMasterDiagSnapshot
{
    uint32_t feed_tx_ok;
    uint32_t feed_tx_err;
    uint32_t hit_tx_ok;
    uint32_t hit_tx_err;
    uint32_t harvest_rx_ok;
    uint32_t harvest_rx_err;
    uint32_t harvest_crc_err;
    uint32_t harvest_cmd_err;
    uint32_t harvest_id_err;
    uint32_t max_rx_ms;
    uint32_t last_err_ms;
    int32_t last_err;
    uint8_t last_err_addr;
};

int i2c_master_start();
std::vector<uint8_t> i2c_master_scan(uint8_t start, uint8_t end);
void i2c_feed_slaves(const std::vector<uint8_t>& slaves, uint8_t id, const std::vector<uint32_t>& nonce_starts, uint32_t nonce_stride, float difficulty, const uint8_t* buffer);
void i2c_feed_slaves(const std::vector<uint8_t>& slaves, uint8_t id, uint32_t nonce_start, uint32_t nonce_stride, float difficulty, const uint8_t* buffer);
void i2c_hit_slaves(const std::vector<uint8_t>& slaves);
void i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id, std::vector<I2cSlaveHarvest>& results);
std::vector<I2cSlaveHarvest> i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id);
std::vector<uint32_t> i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id, uint32_t &total_procesed_nonce);
void i2c_master_diag_reset();
void i2c_master_diag_snapshot(I2cMasterDiagSnapshot &snapshot);
