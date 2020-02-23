// Ref: https://docs.espressif.com/projects/esp-idf/en/v3.3.1/api-reference/peripherals/gpio.html
//      https://github.com/espressif/esp-idf/blob/release/v3.3/components/driver/include/driver/gpio.h
//      https://github.com/espressif/esp-idf/blob/release/v3.3/components/driver/gpio.c

#include "driver/gpio.h"

void app_main()
{
    static const gpio_num_t GPIO_OUTPUT_IO = GPIO_NUM_23;
    static const gpio_num_t GPIO_INPUT_IO = GPIO_NUM_13;

    gpio_config_t io_conf;
    // 割り込みをしない
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 出力モード
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 設定したいピンのビットマスク
    // 1ULLはunsigned long long(64bit)で1を表現する
    // 23ピンを設定するので、ビットマスクは下記のようになる
    // 上位32ビット：MSB|0000 0000|0000 0000|0000 0000|0000 0000|
    // 下位32ビット：    0000 0000|1000 0000|0000 0000|0000 0000|LSB
    io_conf.pin_bit_mask = (1ULL<<GPIO_OUTPUT_IO);
    // 内部プルダウンしない
    io_conf.pull_down_en = 0;
    // 内部プルダウンしない
    io_conf.pull_up_en = 0;
    // 設定をセットする
    gpio_config(&io_conf);

    // 入力ピンの設定
    io_conf.mode = GPIO_MODE_INPUT;
    // 設定したいピンのビットマスク
    io_conf.pin_bit_mask = (1ULL<<GPIO_INPUT_IO);
    // 内部プルダウンする
    // ESP32外部にプルダウン抵抗を接続しているので、
    // 内部プルダウンはしなくてもOK
    io_conf.pull_down_en = 1;
    // 設定をセットする
    gpio_config(&io_conf);

    while(1) {
        gpio_set_level(GPIO_OUTPUT_IO, gpio_get_level(GPIO_INPUT_IO));
    }
}

