#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int const led_pin = 2;
BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

void setup() {
    pinMode(led_pin, OUTPUT);
    Serial.begin(115200);
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
    Serial.println("Setup done");
}

void test_led() {
    digitalWrite(led_pin, HIGH);
    delay(1000);
    digitalWrite(led_pin, LOW);
    delay(1000);
}

void test_ble() {
  int const scan_time = 5; //In seconds
  Serial.println("Scanning BLE");
  BLEScanResults foundDevices = pBLEScan->start(scan_time, false);
  Serial.print("BLE devices found: ");
  Serial.println(foundDevices.getCount());
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);
}

void loop()
{
    test_led();
    test_ble();
}
