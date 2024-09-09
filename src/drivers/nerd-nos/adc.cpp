#include <stdio.h>
#include <math.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define ADC_CHANNEL ADC1_CHANNEL_1  // GPIO2 corresponds to ADC1 channel 1 on ESP32-S3
#define BETA 3380  // Beta value of the thermistor
#define R0 10000   // Resistance at 25°C (10kΩ)
#define ADC_MAX 4095  // Max ADC value for 12-bit resolution
#define DEFAULT_VREF 1100  // Default VREF in millivolts for ESP32 ADC

// ADC Calibration characteristics
static esp_adc_cal_characteristics_t adc1_chars;

void nerdnos_adc_init() {
    // Configure the ADC
    adc1_config_width(ADC_WIDTH_BIT_12);  // Set ADC width (12-bit)
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);  // Set attenuation to read the full range of 0 to 3.3V

    // Characterize ADC at given attenuation
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);
}

float nerdnos_get_temperature() {
    // Convert the raw ADC value to a voltage using esp_adc_cal
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC_CHANNEL), &adc1_chars);  // Voltage in millivolts

    // Convert millivolts to volts
    float voltage = voltage_mv / 1000.0;

    // Ensure the voltage is within a valid range
    if (voltage <= 0) {
        printf("Error: Invalid voltage reading.\n");
        return -273.15; // Return a clearly invalid temperature to indicate an error
    }

    // Calculate the thermistor resistance using the voltage divider formula
    // R_T = R0 * (Vout / (VREF - Vout))
    float thermistor_resistance = R0 * (voltage / (3.3 - voltage));

    // Use the Beta parameter equation to calculate the temperature in Kelvin
    float temperature_kelvin = (float)(BETA / (log(thermistor_resistance / R0) + (BETA / 298.15)));

    // Convert the temperature to Celsius
    float temperature_celsius = temperature_kelvin - 273.15;

    return temperature_celsius;
}
