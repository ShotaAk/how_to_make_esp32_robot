// Ref: https://github.com/espressif/arduino-esp32/blob/1.0.4/tools/sdk/include/driver/driver/adc.h
//      https://github.com/espressif/arduino-esp32/blob/1.0.4/tools/sdk/include/esp_adc_cal/esp_adc_cal.h

#include <driver/adc.h>
#include <esp_adc_cal.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    static const adc_unit_t         unit = ADC_UNIT_1;
    static const adc1_channel_t     channel = ADC1_CHANNEL_0;
    static const adc_atten_t        atten = ADC_ATTEN_DB_6;
    static const adc_bits_width_t   width = ADC_WIDTH_BIT_12;
    static const uint32_t DEFAULT_VREF = 1100;

    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    esp_adc_cal_characteristics_t *adc_chars = 
        (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

    while(1){
        uint32_t adc_reading = 0;
        static const int NO_OF_SAMPLES = 1;
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }
        adc_reading /= NO_OF_SAMPLES;

        uint32_t pin_voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        uint32_t supply_voltage = pin_voltage * 2;

        Serial.print("Raw: ");
        Serial.print(adc_reading);
        Serial.print("\t");
        Serial.print("PinVoltage: ");
        Serial.print(pin_voltage);
        Serial.print("mV \t");
        Serial.print("SupplyVoltage: ");
        Serial.print(supply_voltage);
        Serial.print("mV\n");
        delay(1000);
    }
}
