const int PULSE_PIN = 27;
const int CONTROL_PIN = 14;
int count;

void setup(){
    Serial.begin(115200);
    pinMode(PULSE_PIN, INPUT);
    pinMode(CONTROL_PIN, INPUT);
    // パルスピンの立ち上がりで割り込みを発生させる
    attachInterrupt(PULSE_PIN, count_pulse, RISING);

    const int GPIO_IN1 = 33;
    const int GPIO_IN2 = 25;
    pinMode(GPIO_IN1, OUTPUT);
    pinMode(GPIO_IN2, OUTPUT);
    digitalWrite(GPIO_IN1, LOW);
    digitalWrite(GPIO_IN2, HIGH);
}

void loop(){
    static const int PPMR = 3;
    static const int PPSR = 1300;
    static const int WAIT_TIME_MS = 100;
    static const float WAIT_TIME_SEC = WAIT_TIME_MS * 0.001;

    float motor_speed = ((float)count / PPMR) / (WAIT_TIME_SEC);
    float shaft_speed = ((float)count / PPSR) / (WAIT_TIME_SEC);

    Serial.print("Count:");
    Serial.print(count);
    Serial.print("\t");
    Serial.print("MoterSpeed:");
    Serial.print(motor_speed);
    Serial.print("RPS\t");
    Serial.print("ShaftSpeed:");
    Serial.print(shaft_speed);
    Serial.print("RPS\n");
    count=0;
    delay(WAIT_TIME_MS);
}

void count_pulse(){
    if(digitalRead(CONTROL_PIN)){
        count--;
    }else{
        count++;
    }
}
