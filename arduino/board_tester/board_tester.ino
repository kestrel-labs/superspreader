#include <BLEDevice.h>
#include <string>

constexpr auto device_name   = "HM - Board tester";
constexpr auto blue_led      = 27;
constexpr auto green_led     = 14;
constexpr auto red_led       = 12;
constexpr auto treatment_pin = 4;
bool treatment_received      = false;

void receive_treatment() {
    treatment_received = true;
    detachInterrupt(treatment_pin);
}

void setup() {
    Serial.begin(115200);
    BLEDevice::init(device_name);
    BLEDevice::startAdvertising();
    pinMode(blue_led, OUTPUT);
    pinMode(green_led, OUTPUT);
    pinMode(red_led, OUTPUT);
    pinMode(treatment_pin, INPUT);
    digitalWrite(blue_led, HIGH);
    digitalWrite(green_led, HIGH);
    digitalWrite(red_led, HIGH);
    attachInterrupt(treatment_pin, receive_treatment, RISING);
}

void loop() {
    if (treatment_received) {
        Serial.println("treatment received");
        treatment_received = false;
        attachInterrupt(treatment_pin, receive_treatment, RISING);
    }
}
