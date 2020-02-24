#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"

void app_main()
{
    static const gpio_num_t GPIO_IR_LED = GPIO_NUM_32;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_IR_LED);
    gpio_config(&io_conf);

    static const adc_unit_t         unit = ADC_UNIT_1;
    static const adc_channel_t      channel = ADC_CHANNEL_6;
    static const adc_atten_t        atten = ADC_ATTEN_DB_11;
    static const adc_bits_width_t   width = ADC_WIDTH_BIT_12;
    static const uint32_t DEFAULT_VREF = 1100;
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    esp_adc_cal_characteristics_t *adc_chars = 
        calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

    while (1) {
        uint32_t adc_reading = 0;
        uint32_t sense_millivolts = 0;

        gpio_set_level(GPIO_IR_LED, 1);
        vTaskDelay(1 / portTICK_RATE_MS);
        adc_reading= adc1_get_raw(channel);
        gpio_set_level(GPIO_IR_LED, 0);

        sense_millivolts =
            esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        printf("Raw: %d\tSenseVoltage: %dmV\n",
                adc_reading, sense_millivolts);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
