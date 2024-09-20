#include <Arduino.h>
#include "driver/uart.h"
#include "../devices/device.h"

#include "common.h"
#include "serial.h"

#define BUF_SIZE (1024)

void SERIAL_init() {

    Serial.println("Initializing serial");
    // Configure UART1 parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART1 parameters
    uart_param_config(UART_NUM_1, &uart_config);
    // Set UART1 pins(TX: IO17, RX: I018)
    uart_set_pin(UART_NUM_1, NERD_NOS_GPIO_TX, NERD_NOS_GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Install UART driver (we don't need an event queue here)
    // tx buffer 0 so the tx time doesn't overlap with the job wait time
    //  by returning before the job is written
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
}

void SERIAL_set_baud(int baud)
{
    Serial.printf("Changing UART baud to %i\n", baud);
    uart_set_baudrate(UART_NUM_1, baud);
}

int SERIAL_send(uint8_t *data, int len, bool debug)
{
    // if (debug)
    // {
    //     printf("->");
    //     prettyHex((unsigned char *)data, len);
    //     printf("\n");
    // }

    if (debug) {
        printBufferHex("TX", data, len);
    }
    return uart_write_bytes(UART_NUM_1, (const char *)data, len);
}

size_t SERIAL_check_for_data() {
    size_t length;
    uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length);
    return length;
}

/// @brief waits for a serial response from the device
/// @param buf buffer to read data into
/// @param buf number of ms to wait before timing out
/// @return number of bytes read, or -1 on error
int16_t SERIAL_rx_non_blocking(uint8_t *buf, uint16_t size) {
    // check how much data we have
    size_t available = SERIAL_check_for_data();

    // no data available, return 0
    if (!available) {
        return 0;
    }

    // check for incomplete data
    if (available && available < size) {
        Serial.printf("not returning incomplete data ... %d vs %d\n", (int) available, (int) size);
        return 0;
    }
    // timeout 0 means non_blocking read
    return SERIAL_rx(buf, size, 0);
}


/// @brief waits for a serial response from the device
/// @param buf buffer to read data into
/// @param buf number of ms to wait before timing out
/// @return number of bytes read, or -1 on error
int16_t SERIAL_rx(uint8_t *buf, uint16_t size, uint16_t timeout_ms)
{
    int16_t bytes_read = uart_read_bytes(UART_NUM_1, buf, size, timeout_ms / portTICK_PERIOD_MS);
    // if (bytes_read > 0) {
    //     printf("rx: ");
    //     prettyHex((unsigned char*) buf, bytes_read);
    //     printf("\n");
    // }
    if (bytes_read > 0) {
        printBufferHex("RX", buf, bytes_read);
    }
    return bytes_read;
}

void SERIAL_clear_buffer(void)
{
    uart_flush(UART_NUM_1);
}
