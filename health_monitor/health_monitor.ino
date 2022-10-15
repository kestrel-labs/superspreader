#include <BLEDevice.h>
#include <string>

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */
#define RSSI_LOWER_BOUND -100  // rssi less than this ignored
#define SCAN_SECONDS 3

#define PREFIX "Health Monitor - "
#define CAT    "Zombie -1"
#define IMMUNE "Immune"
#define SUPER  "Super Healthy"
#define HEALTY "Healthy"
#define A_SYMPTOMATIC "Asymptomatic"
#define SICK "Sick"
#define ZOMBIE "Zombie"

int const led_pin = 2;
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned int health = 40;

bool is_monitor(std::string const& name)
{
    return name.rfind(PREFIX, 0) == 0;
}

bool is_close(int rssi)
{
    return rssi >= RSSI_LOWER_BOUND;
}

bool is_infected(std::string const &name)
{
    auto state = name.substr(std::string(PREFIX).length());
    return state == CAT
     || state == A_SYMPTOMATIC
     || state == SICK
     || state == ZOMBIE;
}

BLEScanResults scan_ble()
{
    auto *scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    scan->start(SCAN_SECONDS);
    delay(1000 * SCAN_SECONDS);
    scan->stop();
    return scan->getResults();
}

unsigned int count_infected(BLEScanResults &results)
{
    unsigned int infected = 0;
    for (auto i = 0; i < results.getCount(); ++i)
    {
        auto device = results.getDevice(i);
        auto rssi = device.getRSSI();
        auto name = device.getName();

        if (is_monitor(name) && 
            is_close(rssi) && 
            is_infected(name)) {
            ++infected;
        }
    }
    return infected;
}

void print_scan_results(BLEScanResults &results)
{
    Serial.println("---<< scan results >>--");
    for (auto i = 0; i < results.getCount(); ++i)
    {
        auto device = results.getDevice(i);
        auto address = BLEAddress(device.getAddress());
        auto rssi = device.getRSSI();
        auto name = device.getName();

        Serial.print("Addr: ");
        Serial.print(address.toString().c_str());
        Serial.print(", RSSI: ");
        Serial.print(rssi);
        Serial.print(", Name: ");
        Serial.print(name.c_str());
        Serial.println("");
    }
    Serial.println("---");
}

std::string to_state(unsigned int health) {
    if (health == 0)
    {
        return IMMUNE;
    }
    else if (health < 10)
    {
        return SUPER;
    }
    else if (health < 50)
    {
        return HEALTY;
    }
    else if (health < 60)
    {
        return A_SYMPTOMATIC;
    }
    else if (health < 99)
    {
        return SICK;
    }
    return ZOMBIE;
}

unsigned int update_health(
    unsigned int current_health,
    unsigned int infected)
{
    if (health == 0 || health == 100) {
        return current_health;
    }

    if (infected > 0) {
        return current_health + 1;
    }

    return current_health;
}

void setup()
{
    // start serial
    Serial.begin(115200);

    // Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    // String representing our state (used by other devices to observe us)
    auto state = to_state(health);
    Serial.println(state.c_str());
    Serial.println("Health: " + String(health));

    // Start bluetooth to advertise our state
    BLEDevice::init(PREFIX + state);
    BLEDevice::startAdvertising();

    // Scan for nearby devices and count infected
    auto devices = scan_ble();
    print_scan_results(devices);
    auto infected = count_infected(devices);
    Serial.println("Infected count: " + String(infected));

    // Update our health value
    health = update_health(health, infected);

    // Flush the serial buffer
    Serial.flush();

    // Enable waking up in some amount of time and sleep
    esp_sleep_enable_timer_wakeup(random(1, TIME_TO_SLEEP) * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop()
{
    // This is not going to be called
}
