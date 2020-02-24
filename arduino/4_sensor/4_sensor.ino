#include "driver/adc.h"
#include "esp_adc_cal.h"

#define GPIO_IR_LED 32

void setup() {
    Serial.begin(115200);
    pinMode(GPIO_IR_LED, OUTPUT);
}

void loop() {
    static const adc_unit_t         unit = ADC_UNIT_1;
    static const adc1_channel_t     channel = ADC1_CHANNEL_0;
    static const adc_atten_t        atten = ADC_ATTEN_DB_11;
    static const adc_bits_width_t   width = ADC_WIDTH_BIT_12;
    static const uint32_t DEFAULT_VREF = 1100;
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    esp_adc_cal_characteristics_t *adc_chars = 
        (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

    while (1) {
        uint32_t adc_reading = 0;
        uint32_t sense_millivolts = 0;

        digitalWrite(GPIO_IR_LED, 1);
        delay(1);
        adc_reading= adc1_get_raw(channel);
        digitalWrite(GPIO_IR_LED, 0);

        sense_millivolts =
            esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

        Serial.print("Raw: ");
        Serial.print(adc_reading);
        Serial.print("\t");
        Serial.print("SupplyVoltage: ");
        Serial.print(sense_millivolts);
        Serial.print("mV\n");
        delay(100);
    }
}
