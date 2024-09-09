#pragma once

#include <stdint.h>

uint8_t crc5(uint8_t *buffer, uint16_t len);
uint16_t crc16(uint8_t *buffer, uint16_t len);
uint16_t crc16_false(uint8_t *buffer, uint16_t len);

