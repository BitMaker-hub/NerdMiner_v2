#pragma once

#define CHUNK_SIZE 1024

int SERIAL_send(uint8_t *, int, bool);
void SERIAL_init(void);
int16_t SERIAL_rx(uint8_t *, uint16_t, uint16_t);
int16_t SERIAL_rx_non_blocking(uint8_t *buf, uint16_t size);
void SERIAL_clear_buffer(void);
void SERIAL_set_baud(int baud);

