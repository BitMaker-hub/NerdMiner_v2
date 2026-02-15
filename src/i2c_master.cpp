#include "i2c_master.h"
#include "i2c_hash_config.h"
#include <Arduino.h>
#include <driver/i2c.h>
#include "i2c_protocol.h"

#ifndef I2C_HASH_BUS_PORT
#define I2C_HASH_BUS_PORT 0
#endif
#ifndef I2C_HASH_SDA_PIN
#define I2C_HASH_SDA_PIN 21
#endif
#ifndef I2C_HASH_SCL_PIN
#define I2C_HASH_SCL_PIN 22
#endif
#ifndef I2C_HASH_CLOCK_HZ
#define I2C_HASH_CLOCK_HZ 100000
#endif
#define I2C_MASTER_TX_BUF_LEN 0
#define I2C_MASTER_RX_BUF_LEN 0

static i2c_config_t s_i2c_config;

static bool i2c_probe_hash_slave(uint8_t addr)
{
    uint8_t request[2] = {I2C_CMD_REQUEST_RESULT, 0};
    request[1] = i2cCommandCrc8(request, sizeof(request));

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, request, sizeof(request), true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_HASH_BUS_PORT, cmd, 20 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
        return false;

    JobI2cResult result{};
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, (uint8_t *)&result, sizeof(result), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_HASH_BUS_PORT, cmd, 20 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
        return false;

    if (result.cmd != I2C_CMD_SLAVE_RESULT)
        return false;
    return i2cCommandCrc8(&result, sizeof(result)) == result.crc;
}

int i2c_master_start()
{
    memset(&s_i2c_config, 0, sizeof(s_i2c_config));
    s_i2c_config.mode = I2C_MODE_MASTER;
    s_i2c_config.sda_io_num = I2C_HASH_SDA_PIN;
    s_i2c_config.scl_io_num = I2C_HASH_SCL_PIN;
    s_i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    s_i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    s_i2c_config.master.clk_speed = I2C_HASH_CLOCK_HZ;

    esp_err_t err = i2c_param_config(I2C_HASH_BUS_PORT, &s_i2c_config);
    if (err != ESP_OK)
        return err;
    
    err = i2c_driver_install(I2C_HASH_BUS_PORT, s_i2c_config.mode, I2C_MASTER_TX_BUF_LEN, I2C_MASTER_RX_BUF_LEN, 0);
    if (err == ESP_ERR_INVALID_STATE)
        return ESP_OK; // Bus already initialized by another component (e.g. display)
    return err;
}

std::vector<uint8_t> i2c_master_scan(uint8_t start, uint8_t end)
{
    std::vector<uint8_t> vec;
    for (int addr = start; addr < end; ++addr)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        //i2c_master_write(cmd, data_wr, size, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_HASH_BUS_PORT, cmd, 25 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK && i2c_probe_hash_slave((uint8_t)addr))
            vec.push_back(addr);
    }
    return vec;
}

void i2c_feed_slaves(const std::vector<uint8_t>& slaves, uint8_t id, const std::vector<uint32_t>& nonce_starts, uint32_t nonce_stride, float difficulty, const uint8_t* buffer)
{
    if (slaves.empty())
        return;

    JobI2cRequest request{};
    request.cmd = I2C_CMD_FEED;
    request.id = id;
    request.difficulty = difficulty;
    request.nonce_stride = nonce_stride == 0 ? 1 : nonce_stride;
    memcpy(request.buffer, buffer, sizeof(request.buffer));

    uint32_t fallback_start = 0x20000000u;
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        if (n < nonce_starts.size())
            request.nonce_start = nonce_starts[n];
        else
            request.nonce_start = fallback_start;
        fallback_start += request.nonce_stride;
        request.crc = i2cCommandCrc8(&request, sizeof(request));

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (slaves[n] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, (const uint8_t*)&request, sizeof(request), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_HASH_BUS_PORT, cmd, 10 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    }
}

void i2c_feed_slaves(const std::vector<uint8_t>& slaves, uint8_t id, uint32_t nonce_start, uint32_t nonce_stride, float difficulty, const uint8_t* buffer)
{
    std::vector<uint32_t> nonce_starts;
    nonce_starts.reserve(slaves.size());
    uint32_t current = nonce_start;
    uint32_t step = nonce_stride == 0 ? 1 : nonce_stride;
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        nonce_starts.push_back(current);
        current += step;
    }
    i2c_feed_slaves(slaves, id, nonce_starts, step, difficulty, buffer);
}


void i2c_hit_slaves(const std::vector<uint8_t>& slaves)
{
    uint8_t request[2];
    request[0] = I2C_CMD_REQUEST_RESULT;
    request[1] = i2cCommandCrc8(request, 2);
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (slaves[n] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, request, sizeof(request), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_HASH_BUS_PORT, cmd, 10 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    }
}

void i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id, std::vector<I2cSlaveHarvest>& results)
{
    results.clear();
    if (results.capacity() < slaves.size())
        results.reserve(slaves.size());

    JobI2cResult result{};
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        result = {};
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (slaves[n] << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, (uint8_t*)&result, sizeof(result), I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_HASH_BUS_PORT, cmd, 10 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
            continue;

        uint8_t crc = i2cCommandCrc8(&result, sizeof(result));

        if (crc != result.crc)
            continue;
        if (result.cmd != I2C_CMD_SLAVE_RESULT)
            continue;
        if (result.id != id)
            continue;

        I2cSlaveHarvest item{};
        item.address = slaves[n];
        item.nonce = result.nonce;
        item.processed_nonce = result.processed_nonce;
        item.has_nonce = (result.nonce != kI2cInvalidNonce);
        results.push_back(item);
    }
}

std::vector<I2cSlaveHarvest> i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id)
{
    std::vector<I2cSlaveHarvest> results;
    results.reserve(slaves.size());
    i2c_harvest_slaves(slaves, id, results);
    return results;
}

std::vector<uint32_t> i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id, uint32_t &total_procesed_nonce)
{
    std::vector<uint32_t> nonce_vector;
    std::vector<I2cSlaveHarvest> records;
    records.reserve(slaves.size());
    i2c_harvest_slaves(slaves, id, records);
    total_procesed_nonce = 0;
    nonce_vector.reserve(records.size());
    for (size_t i = 0; i < records.size(); ++i)
    {
        total_procesed_nonce += records[i].processed_nonce;
        if (records[i].has_nonce)
            nonce_vector.push_back(records[i].nonce);
    }
    return nonce_vector;
}
