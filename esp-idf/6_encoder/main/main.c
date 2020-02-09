#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pcnt.h"

void app_main()
{
    static const pcnt_unit_t UNIT = PCNT_UNIT_0;

    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = GPIO_NUM_32,
        .ctrl_gpio_num = GPIO_NUM_33,
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
