/*
 * Fix the rtc_tempsensor.c
 * ESP32-IDF code has an infinity loop that can lock the firmware. This fix removes that infinity loop.
 */


#include <math.h>
#include "driver/temp_sensor.h"
#include "esp_check.h"
#include "esp_err.h"
#include "soc/sens_struct.h"
#include "esp_efuse_rtc_calib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TSENS_ADC_FACTOR  (0.4386)
#define TSENS_DAC_FACTOR  (27.88)
#define TSENS_SYS_OFFSET  (20.52)

static float s_deltaT = NAN; // unused number

static const char *TAG = "tsens";
typedef struct {
    int index;
    int offset;
    int set_val;
    int range_min;
    int range_max;
    int error_max;
} tsens_dac_offset_t;

static const tsens_dac_offset_t dac_offset[TSENS_DAC_MAX] = {
    /*     DAC     Offset reg_val  min  max  error */
    {TSENS_DAC_L0,   -2,     5,    50,  125,   3},
    {TSENS_DAC_L1,   -1,     7,    20,  100,   2},
    {TSENS_DAC_L2,    0,    15,   -10,   80,   1},
    {TSENS_DAC_L3,    1,    11,   -30,   50,   2},
    {TSENS_DAC_L4,    2,    10,   -40,   20,   3},
};

esp_err_t temp_sensor_read_raw_fix(uint32_t *tsens_out)
{
    ESP_RETURN_ON_FALSE(tsens_out != NULL, ESP_ERR_INVALID_ARG, TAG, "no tsens_out specified");
    SENS.sar_tctrl.tsens_dump_out = 1;
    //this is to break the infinit loop
    uint16_t tries = 1000;
    while (!SENS.sar_tctrl.tsens_ready)
    {
      if (--tries == 0)
        //Sensor fails to return the data
        return ESP_FAIL;
        
      vTaskDelay(1/portTICK_RATE_MS);
    }
    *tsens_out = SENS.sar_tctrl.tsens_out;
    SENS.sar_tctrl.tsens_dump_out = 0;
    return ESP_OK;
}


static void read_delta_t_from_efuse(void)
{
    uint32_t version = esp_efuse_rtc_calib_get_ver();
    if (version == 1) {
        // fetch calibration value for temp sensor from eFuse
        s_deltaT = esp_efuse_rtc_calib_get_cal_temp(version);
    } else {
        // no value to fetch, use 0.
        s_deltaT = 0;
    }
    ESP_LOGD(TAG, "s_deltaT = %f", s_deltaT);
}

static float parse_temp_sensor_raw_value(uint32_t tsens_raw, const int dac_offset)
{
    if (isnan(s_deltaT)) { //suggests that the value is not initialized
        read_delta_t_from_efuse();
    }
    float result = (TSENS_ADC_FACTOR * (float)tsens_raw - TSENS_DAC_FACTOR * dac_offset - TSENS_SYS_OFFSET) - s_deltaT / 10.0;
    return result;
}

esp_err_t temp_sensor_read_celsius_fix(float *celsius)
{
    ESP_RETURN_ON_FALSE(celsius != NULL, ESP_ERR_INVALID_ARG, TAG, "celsius points to nothing");
    temp_sensor_config_t tsens;
    uint32_t tsens_out = 0;
    esp_err_t ret = temp_sensor_get_config(&tsens);
    if (ret == ESP_OK) {
        ret = temp_sensor_read_raw_fix(&tsens_out);
        ESP_RETURN_ON_FALSE(ret == ESP_OK, ret, TAG, "failed to read raw data");
        const tsens_dac_offset_t *dac = &dac_offset[tsens.dac_offset];
        *celsius = parse_temp_sensor_raw_value(tsens_out, dac->offset);
        if (*celsius < dac->range_min || *celsius > dac->range_max) {
            ESP_LOGW(TAG, "Exceeding the temperature range!");
            ret = ESP_ERR_INVALID_STATE;
        }
    }
    return ret;
}

esp_err_t temperatureRead_fix(float* result)
{
    esp_err_t ret;
    temp_sensor_config_t tsens = TSENS_CONFIG_DEFAULT();
    temp_sensor_set_config(tsens);
    temp_sensor_start();
    ret = temp_sensor_read_celsius_fix(result); 
    temp_sensor_stop();

    return ret;
}