#include "i2c_master.h"
#include <Arduino.h>
#include <driver/i2c.h>

#define I2C_MASTER_NUM_PORT 0
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define I2C_MASTER_TX_BUF_LEN 1024
#define I2C_MASTER_RX_BUF_LEN 1024

static i2c_config_t s_i2c_config;

#define I2C_CMD_FEED 0xA1
#define I2C_CMD_REQUEST_RESULT 0xA9
#define I2C_CMD_SLAVE_RESULT 0xAA

struct __attribute__((__packed__)) JobI2cRequest
{
  //84 bytes
  uint8_t cmd;
  uint8_t crc;
  uint8_t id;
  uint8_t nonce_start;
  float difficulty;
  uint8_t buffer[76];
};

struct __attribute__((__packed__)) JobI2cResult
{
  //11 bytes
  uint8_t cmd;
  uint8_t crc;
  uint8_t id;
  uint32_t nonce;
  uint32_t processed_nonce;
};

const uint8_t s_crc8_table[256] =
{
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
    0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
    0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
    0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52,
    0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
    0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
    0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
    0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
    0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
    0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
    0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
    0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
    0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
    0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
    0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
    0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
    0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
    0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
    0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
    0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
    0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
    0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
    0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
    0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
    0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
    0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
    0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
    0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
};

static uint8_t CommandCrc8(const void* data, size_t len)
{
  const uint8_t* ptr = (const uint8_t*)data;
  uint8_t crc = 0xFF;
  crc = s_crc8_table[crc ^ ptr[0]];
  for (size_t n = 2; n < len; ++n)
      crc = s_crc8_table[crc ^ ptr[n]];
  return crc;
}

int i2c_master_start()
{
    memset(&s_i2c_config, 0, sizeof(s_i2c_config));
    s_i2c_config.mode = I2C_MODE_MASTER;
    s_i2c_config.sda_io_num = PIN_I2C_SDA;
    s_i2c_config.scl_io_num = PIN_I2C_SCL;
    s_i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    s_i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    s_i2c_config.master.clk_speed = 50000;

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM_PORT, &s_i2c_config);
    if (err != ESP_OK)
        return err;
    
    return i2c_driver_install(I2C_MASTER_NUM_PORT, s_i2c_config.mode, I2C_MASTER_TX_BUF_LEN, I2C_MASTER_RX_BUF_LEN, 0);
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
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM_PORT, cmd, 50 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK)
            vec.push_back(addr);
    }
    return vec;
}

void i2c_feed_slaves(const std::vector<uint8_t>& slaves, uint8_t id, uint8_t nonce_start, float difficulty, const uint8_t* buffer)
{    
    JobI2cRequest request;    
    request.cmd = I2C_CMD_FEED;
    request.id = id;
    request.difficulty = difficulty;
    memcpy(request.buffer, buffer, sizeof(request.buffer));
    
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        request.nonce_start = nonce_start;
        nonce_start += 0x10;
        request.crc = CommandCrc8(&request, sizeof(request));

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (slaves[n] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, (const uint8_t*)&request, sizeof(request), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM_PORT, cmd, 5 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
    }
}


void i2c_hit_slaves(const std::vector<uint8_t>& slaves)
{
    uint8_t request[2];
    request[0] = I2C_CMD_REQUEST_RESULT;
    request[1] = CommandCrc8(request, 2);
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (slaves[n] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, request, sizeof(request), true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM_PORT, cmd, 5 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
    }
}

std::vector<uint32_t> i2c_harvest_slaves(const std::vector<uint8_t>& slaves, uint8_t id, uint32_t &total_procesed_nonce)
{
    std::vector<uint32_t> nonce_vector;
    JobI2cResult result;
    for (size_t n = 0; n < slaves.size(); ++n)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (slaves[n] << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, (uint8_t*)&result, sizeof(result), I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM_PORT, cmd, 5 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        uint8_t crc = CommandCrc8(&result, sizeof(result));

        if (crc != result.crc)
            continue;
        if (result.nonce != 0xFFFFFFFF)
            nonce_vector.push_back(result.nonce);
        total_procesed_nonce += result.processed_nonce;
    }
    return nonce_vector;
}
