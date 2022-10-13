#include <WiFi.h>

int const led_pin = 2;

void setup() {
    pinMode(led_pin, OUTPUT);
    Serial.begin(115200);

    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    Serial.println("Setup done");
}

void test_wifi() {
    Serial.println("Scanning Wifi");
    // WiFi.scanNetworks will return the number of networks found
    int const n = WiFi.scanNetworks();
    if (n == 0) {
        Serial.println("No Wifi networks found");
    } else {
        Serial.print(n);
        Serial.println(" Wifi networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");
}
void test_led() {
    digitalWrite(led_pin, HIGH);
    delay(1000);
    digitalWrite(led_pin, LOW);
    delay(1000);
}

void loop()
{
    test_led();
    test_wifi();
}
