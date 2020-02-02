int SENSOR_PIN = 36;

void setup() {
    Serial.begin(115200);
}

void loop() {
    uint32_t adc_reading = 0;
    static const int NO_OF_SAMPLES = 64;

    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += analogRead(SENSOR_PIN);
    }
    adc_reading /= NO_OF_SAMPLES;

    uint32_t pin_voltage = 3300 * (float(adc_reading) / 4095);
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
