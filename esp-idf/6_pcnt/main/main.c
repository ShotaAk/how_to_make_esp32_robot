#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "soc/gpio_sig_map.h"

#define PCNT_TEST_UNIT      PCNT_UNIT_0
#define PCNT_INPUT_SIG_IO   32  // Pulse Input GPIO
#define PCNT_INPUT_CTRL_IO  35  // Control GPIO HIGH=count up, LOW=count down

static void pcnt_example_init(void)
{
    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
        // Set PCNT input signal and control GPIOs
        .pulse_gpio_num = PCNT_INPUT_SIG_IO,
        .ctrl_gpio_num = PCNT_INPUT_CTRL_IO,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_TEST_UNIT,
        // What to do on the positive / negative edge of pulse input?
        .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
        .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
        // What to do when control input is low or high?
        .lctrl_mode = PCNT_MODE_REVERSE, // Reverse counting direction if low
        .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
        // Set the maximum and minimum limit values to watch
        .counter_h_lim = 2000,
        .counter_l_lim = -2000,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(PCNT_TEST_UNIT, 10);
    pcnt_filter_enable(PCNT_TEST_UNIT);

    /* Enable events on zero, maximum and minimum limit values */
    // pcnt_event_enable(PCNT_TEST_UNIT, PCNT_EVT_H_LIM);
    // pcnt_event_enable(PCNT_TEST_UNIT, PCNT_EVT_L_LIM);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(PCNT_TEST_UNIT);
    pcnt_counter_clear(PCNT_TEST_UNIT);

    /* Register ISR handler and enable interrupts for PCNT unit */
    // pcnt_isr_register(pcnt_example_intr_handler, NULL, 0, &user_isr_handle);
    // pcnt_intr_enable(PCNT_TEST_UNIT);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(PCNT_TEST_UNIT);
}

void app_main()
{
    /* Initialize PCNT event queue and PCNT functions */
    pcnt_example_init();

    while (1) {
        int16_t count = 0;
        pcnt_get_counter_value(PCNT_TEST_UNIT, &count);
        printf("Current counter value :%d\n", count);

        pcnt_counter_clear(PCNT_TEST_UNIT);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
