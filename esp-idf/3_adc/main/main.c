#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF 1100

void app_main()
{
    static const adc_unit_t         unit = ADC_UNIT_1;
    static const adc_channel_t      channel = ADC_CHANNEL_0;
    static const adc_atten_t        atten = ADC_ATTEN_DB_11;
    static const adc_bits_width_t   width = ADC_WIDTH_BIT_12;

    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

    while (1) {
        uint32_t adc_reading = 0;
        static const int NO_OF_SAMPLES = 64;

        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }
        adc_reading /= NO_OF_SAMPLES;

        uint32_t pin_voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        uint32_t supply_voltage = pin_voltage * 2;
        printf("Raw: %d\tPinVoltage: %dmV \tSupplyVoltage: %dmV\n",
                adc_reading, pin_voltage, supply_voltage);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
