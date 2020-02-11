#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define GPIO_OUTPUT_IO 23
#define COMMAND_LED_ON 0x12
#define COMMAND_LED_OFF 0xab
static int flag_led_on = 0;

static void check_led_command(const std::string &command){
    if(command.length() == 1){
        if(command[0] == COMMAND_LED_ON){
            flag_led_on = 1;
        }else if(command[0] == COMMAND_LED_OFF){
            flag_led_on = 0;
        }
    }
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        check_led_command(value);
    }
};

void setup() {
    pinMode(GPIO_OUTPUT_IO, OUTPUT);

    BLEDevice::init("ESP_BLE_EXAMPLE");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
            );

    pCharacteristic->setCallbacks(new MyCallbacks());

    pCharacteristic->setValue("Hello World");
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
}

void loop() {
    digitalWrite(GPIO_OUTPUT_IO, flag_led_on);
    delay(100);
}
