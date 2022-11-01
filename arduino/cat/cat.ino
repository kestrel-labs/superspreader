#include <BLEDevice.h>
#include <string>

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */
#define RSSI_LOWER_BOUND -100  // rssi less than this ignored
#define ALIVE_SECONDS 2

#define NAME "HM - Zombie -1"

int const led_pin = 2;
RTC_DATA_ATTR int bootCount = 0;

void setup()
{
    Serial.begin(115200);

    // Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    // Start bluetooth to advertise our state
    BLEDevice::init(NAME);
    BLEDevice::startAdvertising();
    delay(ALIVE_SECONDS * 1000);

    // Flush the serial buffer and go to sleep
    Serial.flush();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop()
{
    // This is not going to be called
}
