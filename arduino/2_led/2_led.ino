#define GPIO_INPUT_IO 13
#define GPIO_OUTPUT_IO 23

void setup() {
    pinMode(GPIO_INPUT_IO, INPUT);
    pinMode(GPIO_OUTPUT_IO, OUTPUT);
}

void loop() {
    digitalWrite(GPIO_OUTPUT_IO, digitalRead(GPIO_INPUT_IO));
}
