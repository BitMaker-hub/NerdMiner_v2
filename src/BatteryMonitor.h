#include "drivers/devices/device.h"
#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#ifdef BATTERY_MONITOR_ENABLE
#include <driver/adc.h>
#include <esp_adc_cal.h>


class BatteryMonitor {

public:
    BatteryMonitor();

    void setup();
    float readBatteryVoltage();
    void printBatteryVoltage();
    float calcBatteryLevel();
    float calculateBatteryLevel();
    void monitorBattery();

private:
    unsigned long mThrottle = 0;
    int UpdateThresholdMin = 1;
    float batteryLevel = 0.0f;
    esp_adc_cal_characteristics_t adc_chars;
    uint32_t batteryAdcVoltageRead();
    void printEspAdcCalCharacteristics();
};

#endif
#endif // BATTERY_MONITOR_H
