// Ref: https://github.com/espressif/arduino-esp32/blob/1.0.4/tools/sdk/include/driver/driver/mcpwm.h
#include <driver/mcpwm.h>

static const mcpwm_unit_t UNIT = MCPWM_UNIT_0;
static const gpio_num_t GPIO_IN1 = GPIO_NUM_32;
static const gpio_num_t GPIO_IN2 = GPIO_NUM_33;

static const mcpwm_io_signals_t SIGNAL_IN1 = MCPWM0A;
static const mcpwm_io_signals_t SIGNAL_IN2 = MCPWM0B;
static const mcpwm_timer_t TIMER = MCPWM_TIMER_0;

static const mcpwm_operator_t OPR_IN1 = MCPWM_OPR_A;
static const mcpwm_operator_t OPR_IN2 = MCPWM_OPR_B;

static const mcpwm_duty_type_t DUTY_MODE = MCPWM_DUTY_MODE_0; // Active high duty


static void drive_forward(mcpwm_timer_t timer_num , float duty)
{
    mcpwm_set_signal_low(UNIT, timer_num, OPR_IN2);
    mcpwm_set_duty(UNIT, timer_num, OPR_IN1, duty);
    mcpwm_set_duty_type(UNIT, timer_num, OPR_IN1, DUTY_MODE);
}

static void drive_backward(mcpwm_timer_t timer_num , float duty)
{
    mcpwm_set_signal_low(UNIT, timer_num, OPR_IN1);
    mcpwm_set_duty(UNIT, timer_num, OPR_IN2, duty);
    mcpwm_set_duty_type(UNIT, timer_num, OPR_IN2, DUTY_MODE);
}

static void drive_brake(mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_high(UNIT, timer_num, OPR_IN1);
    mcpwm_set_signal_high(UNIT, timer_num, OPR_IN2);
}

static void drive_stop(mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_low(UNIT, timer_num, OPR_IN1);
    mcpwm_set_signal_low(UNIT, timer_num, OPR_IN2);
}

static void motor_drive(const unsigned int go_back, const float duty)
{
    const float DUTY_MAX = 100;
    const float DUTY_MIN = 5;

    float target_duty = duty;
    if(target_duty > DUTY_MAX){
        target_duty = DUTY_MAX;
    }

    if(target_duty < DUTY_MIN){
        drive_brake(TIMER);
    }else{
        if(go_back){
            drive_backward(TIMER, target_duty);
        }else{
            drive_forward(TIMER, target_duty);
        }
    }
}

static void motor_off(void)
{
    drive_stop(TIMER);
}


void setup() {

    mcpwm_gpio_init(UNIT, SIGNAL_IN1, GPIO_IN1);
    mcpwm_gpio_init(UNIT, SIGNAL_IN2, GPIO_IN2);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = 100*1000; // frequency = 100kHz,
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = DUTY_MODE;

    mcpwm_init(UNIT, TIMER, &pwm_config);

    motor_off();
}

void loop() {
    const int WAIT_TIME = 200;
    const float TARGET_DUTY_MAX = 100;
    const float TARGET_DUTY_MIN = 50;

    for(int go_back=0; go_back<=1; go_back++){
        for(int duty=TARGET_DUTY_MIN; duty<TARGET_DUTY_MAX; duty++){
            motor_drive(go_back, duty);
            vTaskDelay(WAIT_TIME / portTICK_RATE_MS);
        }

        for(int duty=TARGET_DUTY_MAX; duty>TARGET_DUTY_MIN; duty--){
            motor_drive(go_back, duty);
            vTaskDelay(WAIT_TIME / portTICK_RATE_MS);
        }
    }
}

