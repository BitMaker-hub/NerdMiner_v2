
#include "drivers/devices/device.h"

#ifdef BATTERY_MONITOR_ENABLE
#include <iostream>
#include <xpt2046.h>
#include <driver/adc.h>
#include <esp_adc_cal.h> // For calibration structure
#include "BatteryMonitor.h"

// is it 4.2 or 4.4 volts?
const float BATTERY_MAX = 4.2;
const float BATTERY_MIN = 3.3;
const int VOLTAGE_MIN = 3300;
const int VOLTAGE_MAX = 4200;

BatteryMonitor::BatteryMonitor() {}

void BatteryMonitor::setup() {
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    delay(100);
    
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);

    // Calibrate ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.println("ADC characterized using Two Point values stored in eFuse");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.println("ADC characterized using eFuse Vref");
    } else {
        Serial.println("ADC characterized using default Vref");
    }
    printEspAdcCalCharacteristics();
}

float BatteryMonitor::readBatteryVoltage() {
    uint32_t voltage = batteryAdcVoltageRead() * 2;
    return static_cast<float>(voltage) / 1000.0;
}

void BatteryMonitor::printBatteryVoltage() {
    float voltage = readBatteryVoltage();
    Serial.print("Battery voltage: ");
    Serial.println(voltage, 3);
}

float BatteryMonitor::calculateBatteryLevel() {
    
    if ((mThrottle == 0) || (millis() - mThrottle > UpdateThresholdMin * 60 * 1000)) {
        float voltage = readBatteryVoltage();
        Serial.print("Battery voltage: ");
        Serial.println(voltage, 3);
        batteryLevel = ((voltage * 1000) - VOLTAGE_MIN) * 100 / (VOLTAGE_MAX - VOLTAGE_MIN);
        Serial.print("Battery Level: ");
        Serial.println(batteryLevel, 2);
        mThrottle = millis();
    }
    return batteryLevel < 100 ? batteryLevel : 100.0f;
}

void BatteryMonitor::monitorBattery() {
    float batteryLevel = calculateBatteryLevel();
}

uint32_t BatteryMonitor::batteryAdcVoltageRead() {
    return esp_adc_cal_raw_to_voltage(analogRead(BAT_ADC_PIN), &adc_chars);
}

// Function to print the structure contents to the ESP32 console
void BatteryMonitor::printEspAdcCalCharacteristics() {
    // Begin serial communication if needed (make sure Serial.begin() is called in setup)
    Serial.println("ADC Calibration Characteristics:");

    // Print ADC number
    Serial.print("ADC Unit: ");
    Serial.println(static_cast<int>(adc_chars.adc_num));

    // Print ADC attenuation
    Serial.print("ADC Attenuation: ");
    Serial.println(static_cast<int>(adc_chars.atten));

    // Print ADC bit width
    Serial.print("ADC Bit Width: ");
    Serial.println(static_cast<int>(adc_chars.bit_width));

    // Print coefficients
    Serial.print("Coefficient A: ");
    Serial.println(adc_chars.coeff_a);

    Serial.print("Coefficient B: ");
    Serial.println(adc_chars.coeff_b);

    // Print Vref
    Serial.print("Vref: ");
    Serial.println(adc_chars.vref);

    // Print Low Curve
    if (adc_chars.low_curve != nullptr) {
        Serial.print("Low Curve Pointer: ");
        Serial.println(reinterpret_cast<uintptr_t>(adc_chars.low_curve), HEX);  // Print as hex
    } else {
        Serial.println("Low Curve: Not available");
    }

    // Print High Curve
    if (adc_chars.high_curve != nullptr) {
        Serial.print("High Curve Pointer: ");
        Serial.println(reinterpret_cast<uintptr_t>(adc_chars.high_curve), HEX);  // Print as hex
    } else {
        Serial.println("High Curve: Not available");
    }

    // Print Version
    Serial.print("Version: ");
    Serial.println(static_cast<int>(adc_chars.version));
}



#endif