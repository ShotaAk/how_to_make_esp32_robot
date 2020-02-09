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
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_IR_LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    static const adc_unit_t         UNIT = ADC_UNIT_1;
    static const adc_channel_t      CHANNEL = ADC_CHANNEL_0;
    static const adc_atten_t        ATTEN = ADC_ATTEN_DB_11;
    static const adc_bits_width_t   WIDTH = ADC_WIDTH_BIT_12;
    static const uint32_t DEFAULT_VREF = 1100;
    adc1_config_width(WIDTH);
    adc1_config_channel_atten(CHANNEL, ATTEN);
    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(UNIT, ATTEN, WIDTH, DEFAULT_VREF, adc_chars);

    uint32_t adc_reading = 0;
    uint32_t adc_offset = 0;
    uint32_t sense_millivolts = 0;
    while (1) {
        // 反射する前のセンサ値を取得(オフセット)
        adc_offset = adc1_get_raw(CHANNEL);
        gpio_set_level(GPIO_IR_LED, 1);
        vTaskDelay(10 / portTICK_RATE_MS);
        adc_reading= adc1_get_raw(CHANNEL);
        gpio_set_level(GPIO_IR_LED, 0);

        adc_offset = 0;

       if(adc_reading < adc_offset){
           adc_reading = 0;
       }else{
           adc_reading -= adc_offset;
       }

       sense_millivolts =
           esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

        printf("Raw: %d\tSenseVoltage: %dmV\n",
                adc_reading, sense_millivolts);

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
