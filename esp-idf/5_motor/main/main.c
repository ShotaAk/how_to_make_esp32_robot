#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/mcpwm.h"


#define GPIO_PWM0A_OUT 32
#define GPIO_PWM0B_OUT 33

static mcpwm_timer_t MC_TIMER;

static void mcpwm_example_gpio_initialize()
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);
}

/**
 * @brief motor moves in forward direction, with duty cycle = duty %
 */
static void brushed_motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
}

/**
 * @brief motor moves in backward direction, with duty cycle = duty %
 */
static void brushed_motor_backward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0); 
}

/**
 * @brief motor stop
 */
static void brushed_motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
}

static void motorDrive(float duty){
    // dutyを±100に加工する
    if(duty < -100){
        duty = -100;
    }else if(duty > 100){
        duty = 100;
    }
    
    if(duty < 0){
        // dutyがマイナスのときは逆回転
        duty *= -1.0; // dutyの符号を正にする
        brushed_motor_backward(MCPWM_UNIT_0, MC_TIMER, duty);
    }else{
        brushed_motor_forward(MCPWM_UNIT_0, MC_TIMER, duty);
    }
}

void app_main()
{
    //1. mcpwm gpio initialization
    mcpwm_example_gpio_initialize();

    //2. initial mcpwm configuration
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 100*1000;    //frequency = 100kHz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;

    MC_TIMER = MCPWM_TIMER_0;

    mcpwm_init(MCPWM_UNIT_0, MC_TIMER, &pwm_config);    //Configure PWM0A & PWM0B with above settings

    while (1) {
        const int WAIT_TIME = 3000;

        brushed_motor_stop(MCPWM_UNIT_0, MC_TIMER);
        vTaskDelay(WAIT_TIME / portTICK_RATE_MS);

        motorDrive(80);
        vTaskDelay(WAIT_TIME / portTICK_RATE_MS);

        motorDrive(0);
        vTaskDelay(WAIT_TIME / portTICK_RATE_MS);

        motorDrive(-80);
        vTaskDelay(WAIT_TIME / portTICK_RATE_MS);
    }
}

