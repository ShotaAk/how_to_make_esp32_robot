// Ref: https://github.com/espressif/arduino-esp32/blob/1.0.4/tools/sdk/include/driver/driver/mcpwm.h
#include <driver/mcpwm.h>

void setup() {

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 32);
}

void loop() {
}
