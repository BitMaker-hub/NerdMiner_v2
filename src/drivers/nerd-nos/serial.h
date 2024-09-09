#pragma once

#define CHUNK_SIZE 1024

int SERIAL_send(uint8_t *, int, bool);
void SERIAL_init(void);
void SERIAL_debug_rx(void);
int16_t SERIAL_rx(uint8_t *, uint16_t, uint16_t);
void SERIAL_clear_buffer(void);
void SERIAL_set_baud(int baud);

