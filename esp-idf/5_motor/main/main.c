#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"

static const mcpwm_unit_t UNIT = MCPWM_UNIT_0;
static const mcpwm_timer_t TIMER = MCPWM_TIMER_0;
static const mcpwm_operator_t OPR_IN1 = MCPWM_OPR_A;
static const mcpwm_operator_t OPR_IN2 = MCPWM_OPR_B;
static const mcpwm_duty_type_t DUTY_MODE = MCPWM_DUTY_MODE_0;

static void drive_forward(float duty){
    mcpwm_set_signal_low(UNIT, TIMER, OPR_IN2);
    mcpwm_set_duty(UNIT, TIMER, OPR_IN1, duty);
    mcpwm_set_duty_type(UNIT, TIMER, OPR_IN1, DUTY_MODE);
}

static void drive_backward(float duty){
    mcpwm_set_signal_low(UNIT, TIMER, OPR_IN1);
    mcpwm_set_duty(UNIT, TIMER, OPR_IN2, duty);
    mcpwm_set_duty_type(UNIT, TIMER, OPR_IN2, DUTY_MODE);
}

static void drive_brake(void){
    mcpwm_set_signal_high(UNIT, TIMER, OPR_IN1);
    mcpwm_set_signal_high(UNIT, TIMER, OPR_IN2);
}

static void motor_drive(const float duty, const int go_back){
    if(go_back){
        drive_backward(duty);
    }else{
        drive_forward(duty);
    }
}

void app_main(){
    mcpwm_gpio_init(UNIT, MCPWM0A, GPIO_NUM_33);
    mcpwm_gpio_init(UNIT, MCPWM0B, GPIO_NUM_25);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = 10*1000; // frequency = 10kHz,
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = DUTY_MODE;

    mcpwm_init(UNIT, TIMER, &pwm_config);
    drive_brake();

    const int WAIT_TIME_MS = 100;
    const int DUTY_STEP = 1;
    const int DUTY_MAX = 100;
    const int DUTY_MIN = 50;
    while (1) {
        for(int go_back=0; go_back<=1; go_back++){
            for(int duty=DUTY_MIN; duty<DUTY_MAX; duty+=DUTY_STEP){
                motor_drive(duty, go_back);
                vTaskDelay(WAIT_TIME_MS / portTICK_RATE_MS);
            }
        
            for(int duty=DUTY_MAX; duty>=DUTY_MIN; duty-=DUTY_STEP){
                motor_drive(duty, go_back);
                vTaskDelay(WAIT_TIME_MS / portTICK_RATE_MS);
            }
        }
    }
}

