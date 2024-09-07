#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t crc5(uint8_t *buffer, uint16_t len);
uint16_t crc16(uint8_t *buffer, uint16_t len);
uint16_t crc16_false(uint8_t *buffer, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // CRC_H_