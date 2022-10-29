#include <BLEDevice.h>
#include <string>
#include <cmath>

constexpr auto uS_TO_S_FACTOR = 1000000; /* Conversion factor for micro seconds to seconds */
constexpr auto TIME_TO_SLEEP = 5;        /* Time ESP32 will go to sleep (in seconds) */
constexpr auto RSSI_LOWER_BOUND = -80;   // rssi less than this ignored
constexpr auto SCAN_SECONDS = 3;

constexpr auto PREFIX_STR        = "HM - ";
constexpr auto CAT_STR           = "Zombie -1";
constexpr auto IMMUNE_STR        = "Immune";
constexpr auto SUPER_STR         = "Super Healthy";
constexpr auto HEALTY_STR        = "Healthy";
constexpr auto A_SYMPTOMATIC_STR = "Asymptomatic";
constexpr auto SICK_STR          = "Sick";
constexpr auto ZOMBIE_STR        = "Zombie";

constexpr auto green_led_pin = 16;
constexpr auto red_led_pin = 17;
constexpr auto enable_pin = 18;
constexpr auto treatment_pin = GPIO_NUM_4;

//// From game_rules_2.py //////////////////////////////////////////

// health is tracked with an unsigned int
using health_t = unsigned int;

// these are the bounds of the different types of health
enum struct StateBounds : health_t {
    IMMUNE = 1,
    SUPER_HEALTHY = 2,
    HEALTHY = 10,
    INFECTED_ASYM = 40,
    INFECTED_SYM = 70,
    INFECTED_SYM_LATE = 90,
    ZOMBIE = 99,
};

enum struct ProgressRate : health_t {
    SUPER_HEALTHY = 2,
    INFECTED = 6,
};

enum struct InfectionRate : health_t {
    CAT = 2,
    HUMAN = 1,
};

health_t to_h(StateBounds bounds) {
    return static_cast<health_t>(bounds);
}
health_t to_h(ProgressRate rate) {
    return static_cast<health_t>(rate);
}
health_t to_h(InfectionRate rate) {
    return static_cast<health_t>(rate);
}

bool is_immune(health_t health) {
    return health <= to_h(StateBounds::IMMUNE);
}
bool is_super_healthy(health_t health) {
    return to_h(StateBounds::SUPER_HEALTHY) <= health && health < to_h(StateBounds::HEALTHY);
}
bool is_healthy(health_t health) {
    return to_h(StateBounds::HEALTHY) <= health && health < to_h(StateBounds::INFECTED_ASYM);
}
bool is_infected(health_t health) { 
    // Covers all infected states before ZOMBIE
    return to_h(StateBounds::INFECTED_ASYM) <= health;
}
bool is_infected_asym(health_t health) {
    return to_h(StateBounds::INFECTED_ASYM) <= health && health < to_h(StateBounds::INFECTED_SYM);
}
bool is_infected_sym(health_t health) {
    return to_h(StateBounds::INFECTED_SYM) <= health && health < to_h(StateBounds::INFECTED_SYM_LATE);
}
bool is_infected_sym_late(health_t health) {
    return to_h(StateBounds::INFECTED_SYM_LATE) <= health && health < to_h(StateBounds::ZOMBIE);
}
bool is_zombie(health_t health) {
    return to_h(StateBounds::ZOMBIE) <= health;
}

struct HealthState {
    health_t health = to_h(StateBounds::SUPER_HEALTHY);
    bool cat_resistance = false;
};

struct Exposure {
    health_t human = 0;
    health_t cat = 0;
};

health_t exposure_increase(struct HealthState health_state, struct Exposure exposures) {
    if (is_immune(health_state.health) || is_infected(health_state.health))
        return 0;
 
    // resistant to cats when cat_resistance = 1
    return exposures.human*to_h(InfectionRate::HUMAN) 
        + exposures.cat*to_h(InfectionRate::CAT)*(health_state.cat_resistance ? 0 : 1);
}

// Checks health status, returns a 0 or constant value to increment h by to track disease progress
health_t time_increase(health_t health) {
    if (is_super_healthy(health)) {
        auto const sum = health + to_h(ProgressRate::SUPER_HEALTHY);
        auto const remainder = sum % to_h(StateBounds::HEALTHY);
        auto const quotient = std::floor(sum / to_h(StateBounds::HEALTHY));
        return to_h(ProgressRate::SUPER_HEALTHY) - remainder*quotient;
    }
    if (is_infected(health))
        return to_h(ProgressRate::INFECTED);
    return 0;
}

// takes current health state (count and cat resistance), count of humans and cats nearby
HealthState health_update(HealthState health_state, Exposure exposures) {
    if (is_zombie(health_state.health))
        health_state.health = to_h(StateBounds::ZOMBIE);
    if (is_immune(health_state.health))
        health_state.health = to_h(StateBounds::IMMUNE); 
    health_state.health = health_state.health + time_increase(health_state.health) + exposure_increase(health_state,exposures);
    health_state.cat_resistance = is_infected(health_state.health);
    if (is_zombie(health_state.health))
        health_state.health = to_h(StateBounds::ZOMBIE);
    return health_state;
}

HealthState apply_treatment(HealthState health_state) {
    if (is_infected_sym_late(health_state.health)) {
        health_state.health = to_h(StateBounds::IMMUNE);
    } else if (is_infected_sym(health_state.health)) {
        health_state.health = to_h(StateBounds::HEALTHY);
    } else if (is_infected_asym(health_state.health)) {
        health_state.health -= 35; // sometimes you go healthy, other times super 
    } else if (is_healthy(health_state.health)) {
        health_state.health = to_h(StateBounds::SUPER_HEALTHY);
    }

    return health_state;
}

//// Game State ///////////////////////////////////////////////////

RTC_DATA_ATTR static int g_boot_count = 0;
RTC_DATA_ATTR static bool g_treated = false;
RTC_DATA_ATTR static struct HealthState g_health_state;
RTC_DATA_ATTR static health_t g_health = to_h(StateBounds::SUPER_HEALTHY);
RTC_DATA_ATTR static bool g_cat_resistance = false;

//// Bluetooth ////////////////////////////////////////////////////

bool is_monitor(std::string const& name)
{
    return name.rfind(PREFIX_STR, 0) == 0;
}

bool is_close(int rssi)
{
    return rssi >= RSSI_LOWER_BOUND;
}

bool is_infected_human(std::string const& name)
{
    auto const state = name.substr(std::string(PREFIX_STR).length());
    return state == A_SYMPTOMATIC_STR
     || state == SICK_STR
     || state == ZOMBIE_STR;
}
bool is_cat(std::string const &name)
{
    auto const state = name.substr(std::string(PREFIX_STR).length());
    return state == CAT_STR;
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

Exposure count_exposure(BLEScanResults &results)
{
    Exposure exposure;
    exposure.human = 0;
    exposure.cat = 0;
    for (auto i = 0; i < results.getCount(); ++i)
    {
        auto device = results.getDevice(i);
        auto const rssi = device.getRSSI();
        auto const name = device.getName();

        if (is_monitor(name) && is_close(rssi)) {
            if (is_infected_human(name))
                exposure.human += 1;
            if (is_cat(name))
                exposure.cat += 1;
        }
    }
    return exposure;
}

void print_scan_results(BLEScanResults &results)
{
    Serial.println("---<< scan results >>--");
    for (auto i = 0; i < results.getCount(); ++i)
    {
        auto device = results.getDevice(i);
        auto address = BLEAddress(device.getAddress());
        auto const rssi = device.getRSSI();
        auto const name = device.getName();

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

std::string to_display_state(health_t health) {
    if (is_immune(health))
    {
        return IMMUNE_STR;
    }
    else if (is_super_healthy(health))
    {
        return SUPER_STR;
    }
    else if (is_healthy(health))
    {
        return HEALTY_STR;
    }
    else if (is_infected_asym(health))
    {
        return A_SYMPTOMATIC_STR;
    }
    else if (is_infected(health))
    {
        return SICK_STR;
    }
    return ZOMBIE_STR;
}

void IRAM_ATTR receive_treatment() {
    g_treated = true;
    // treatment can only happen once per wakeup
    detachInterrupt(treatment_pin);
}

void configure_hw() {
    pinMode(enable_pin, INPUT);
    pinMode(green_led_pin, OUTPUT);
    pinMode(red_led_pin, OUTPUT);
    pinMode(treatment_pin, INPUT);
    attachInterrupt(treatment_pin, receive_treatment, RISING);

    // start serial
    Serial.begin(115200);
}

bool is_monitor_enabled() {
    return !digitalRead(enable_pin);
}

void reset_state() {
    g_boot_count = 0;
    g_health_state.health = to_h(StateBounds::SUPER_HEALTHY);
    g_health_state.cat_resistance = false;
}

void print_wakeup_reason(){
  auto const wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :
      Serial.println("Wakeup caused by external signal using RTC_IO");
      g_treated = true;
      break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void show_treatment_animation() {
    // Two slow blinks
    for (int n = 0; n < 2; ++n) {
        digitalWrite(green_led_pin, HIGH);
        digitalWrite(red_led_pin, HIGH);
        delay(1000);
        digitalWrite(green_led_pin, LOW);
        digitalWrite(red_led_pin, LOW);
        delay(1000);
    }
    // Eight fast blinks
    for (int n = 0; n < 8; ++n) {
        digitalWrite(green_led_pin, HIGH);
        digitalWrite(red_led_pin, HIGH);
        delay(100);
        digitalWrite(green_led_pin, LOW);
        digitalWrite(red_led_pin, LOW);
        delay(100);
    }
}

void setup()
{
    configure_hw();

    g_health_state.health = g_health;
    g_health_state.cat_resistance = g_cat_resistance;

    if (!is_monitor_enabled()) {
        Serial.println("Health Monitor disabled");
        reset_state();
    }
    else if (g_treated) {
        Serial.println("Treatment received");
        show_treatment_animation();
        g_treated = false;
        
        // apply treatment to the game state
        g_health_state = apply_treatment(g_health_state);
    }
    else {
        print_wakeup_reason();
        // Increment boot number and print it every reboot
        ++g_boot_count;
        Serial.println("Boot number: " + String(g_boot_count));

        // String representing our state (used by other devices to observe us)
        auto const display_state = to_display_state(g_health_state.health);
        Serial.println(display_state.c_str());
        Serial.println("Health: " + String(g_health_state.health));

        // Start bluetooth to advertise our state
        BLEDevice::init(PREFIX_STR + display_state);
        BLEDevice::startAdvertising();

        // Scan for nearby devices and count infected
        auto devices = scan_ble();
        print_scan_results(devices);
        auto const exposure = count_exposure(devices);
        Serial.println("Infected human exposure: " + String(exposure.human));
        Serial.println("Infected cat exposure: " + String(exposure.cat));

        // Update our health value
        g_health_state = health_update(g_health_state, exposure);
    }
    // Flush the serial buffer
    Serial.flush();

    g_health = g_health_state.health;
    g_cat_resistance = g_health_state.cat_resistance;

    // Enable waking up in some amount of time and sleep
    // Tests confirm that random does not produce the same number after reset
    esp_sleep_enable_timer_wakeup(random(1, TIME_TO_SLEEP) * uS_TO_S_FACTOR);
    // Wakeup when treatment pin goes high
    esp_sleep_enable_ext0_wakeup(treatment_pin, 1);
    esp_deep_sleep_start();
}

void loop()
{
    // This is not going to be called
}
