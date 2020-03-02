#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pcnt.h"
#include "driver/gpio.h"

void app_main()
{
    static const pcnt_unit_t UNIT = PCNT_UNIT_0;
    pcnt_config_t pcnt_config = {
        // pulse_gpio_num, ctrl_gpio_num: 
        //   GPIOピンの割り当て
        .pulse_gpio_num = GPIO_NUM_27, // A相
        .ctrl_gpio_num = GPIO_NUM_14, // B相
        // pos_mode, neg_mode:
        //   パルスピンの立ち上がり、立ち下がりでカウントをどうするか
        .pos_mode = PCNT_COUNT_INC, // 立ち上がりでカウントアップ
        .neg_mode = PCNT_COUNT_DIS, // カウントダウン
        // lctrl_mode, hctrl_mode:
        //   パルスピンのトリガー時に、コントロールピンのhigh/lowによって
        //   カウントモードをどうするか
        .lctrl_mode = PCNT_MODE_KEEP, // lowならモードはそのまま
        .hctrl_mode = PCNT_MODE_REVERSE, // highならカウントダウン
        .counter_h_lim = 2000, // カウント最大数
        .counter_l_lim = -2000, // カウント最小数
        .unit = UNIT,
        .channel = PCNT_CHANNEL_0,
    };
    pcnt_unit_config(&pcnt_config);
    // フィルタ機能で、APBクロック（80MHz）の
    //   nカウント数以下のパルスを無視できる
    //   今回は、最大の1023カウントを設定するが、エンコーダのパルスは
    //   1023カウントよりも十分に大きいので意味がない。
    pcnt_set_filter_value(UNIT, 1023);
    pcnt_filter_enable(UNIT);

    // モータを回転させるためのGPIO設定
    static const gpio_num_t GPIO_IN1 = GPIO_NUM_33;
    static const gpio_num_t GPIO_IN2 = GPIO_NUM_25;
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_IN1) | (1ULL<<GPIO_IN2);
    gpio_config(&io_conf);
    gpio_set_level(GPIO_IN1, 1);
    gpio_set_level(GPIO_IN2, 0);

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
