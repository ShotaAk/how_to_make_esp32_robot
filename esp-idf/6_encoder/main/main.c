#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pcnt.h"
#include "driver/gpio.h"

void app_main()
{
    static const pcnt_unit_t UNIT = PCNT_UNIT_0;
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = GPIO_NUM_27,
        .ctrl_gpio_num = GPIO_NUM_14,
        .channel = PCNT_CHANNEL_0,
        .unit = UNIT,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .counter_h_lim = 20000,
        .counter_l_lim = -20000,
    };
    pcnt_unit_config(&pcnt_config);
    pcnt_filter_enable(UNIT);

    static const gpio_num_t GPIO_IN1 = GPIO_NUM_33;
    static const gpio_num_t GPIO_IN2 = GPIO_NUM_25;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_IN1) | (1ULL<<GPIO_IN2);
    gpio_config(&io_conf);

    gpio_set_level(GPIO_IN1, 0);
    gpio_set_level(GPIO_IN2, 1);

    // Pulse Per Moter Revolution
    static const int PPMR = 3;
    // Pulse Per Shaft Revolution
    static const int PPSR = 1300;
    static const int WAIT_TIME_MS = 100;
    static const float WAIT_TIME_SEC = WAIT_TIME_MS * 0.001;
    while (1) {
        int16_t count = 0;
        pcnt_get_counter_value(UNIT, &count);

        float motor_speed = ((float)count / PPMR) / (WAIT_TIME_SEC);
        float shaft_speed = ((float)count / PPSR) / (WAIT_TIME_SEC);

        printf("Count:%d\tMotorSpeed:%fRPS\tShaftSpeed:%fRPS\n", 
                count, motor_speed, shaft_speed);
        pcnt_counter_clear(UNIT);
        vTaskDelay(pdMS_TO_TICKS(WAIT_TIME_MS));
    }
}
