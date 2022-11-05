#include <BLEDevice.h>
#include <cmath>

#include "health_monitor_core.h"
#include "static_ring_buffer.h"

namespace globals {

//// Persistent State ///////////////////////////////////////////////////

// TODO: DO NOT MERGE: as is the state is not persistent!!!
RTC_DATA_ATTR PlayerState player_state_persistent;

//// Temporary State ////////////////////////////////////////////////////

bool treament_received_interrupt_fired = false;

}  // namespace globals

//// Device stuff ///////////////////////////////////////////////////////

constexpr auto uS_TO_S_FACTOR   = 1000000; /* Conversion factor for micro seconds to seconds */
constexpr auto TIME_TO_SLEEP    = 5;       /* Time ESP32 will go to sleep (in seconds) */
constexpr auto RSSI_LOWER_BOUND = -77;     // rssi less than this ignored
constexpr auto SCAN_SECONDS     = 3;

constexpr auto PREFIX_STR        = "HM - ";
constexpr auto CAT_STR           = "Zombie -1";
constexpr auto IMMUNE_STR        = "Immune";
constexpr auto SUPER_STR         = "Super Healthy";
constexpr auto HEALTY_STR        = "Healthy";
constexpr auto A_SYMPTOMATIC_STR = "Asymptomatic";
constexpr auto SICK_STR          = "Sick";
constexpr auto ZOMBIE_STR        = "Zombie";

constexpr auto green_led_pin = 16;
constexpr auto red_led_pin   = 17;
constexpr auto treatment_pin = GPIO_NUM_4;

bool is_monitor(std::string const& name) { return name.rfind(PREFIX_STR, 0) == 0; }

bool is_close(int rssi) { return rssi >= RSSI_LOWER_BOUND; }

bool is_infected_human(std::string const& name) {
    auto const state = name.substr(std::string(PREFIX_STR).length());
    return state == A_SYMPTOMATIC_STR || state == SICK_STR || state == ZOMBIE_STR;
}

bool is_cat(std::string const& name) {
    auto const state = name.substr(std::string(PREFIX_STR).length());
    return state == CAT_STR;
}

BLEScanResults scan_ble() {
    auto* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    scan->start(SCAN_SECONDS);
    delay(1000 * SCAN_SECONDS);
    scan->stop();
    return scan->getResults();
}

ExposureEvent make_exposure_event_from_scan(BLEScanResults& results) {
    ExposureEvent exposure;
    exposure.human = 0;
    exposure.cat   = 0;
    for (auto i = 0; i < results.getCount(); ++i) {
        auto device     = results.getDevice(i);
        auto const rssi = device.getRSSI();
        auto const name = device.getName();

        if (is_monitor(name) && is_close(rssi)) {
            if (is_infected_human(name)) {
                exposure.human += 1;
            }
            if (is_cat(name)) {
                exposure.cat += 1;
            }
        }
    }
    return exposure;
}

void print_scan_results(BLEScanResults& results) {
    Serial.println("---<< scan results >>--");
    for (auto i = 0; i < results.getCount(); ++i) {
        auto device     = results.getDevice(i);
        auto address    = BLEAddress(device.getAddress());
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
    if (is_immune(health)) {
        return IMMUNE_STR;
    } else if (is_super_healthy(health)) {
        return SUPER_STR;
    } else if (is_healthy(health)) {
        return HEALTY_STR;
    } else if (is_infected_asym(health)) {
        return A_SYMPTOMATIC_STR;
    } else if (is_infected(health)) {
        return SICK_STR;
    }
    return ZOMBIE_STR;
}

void IRAM_ATTR receive_treatment() {
    globals::treament_received_interrupt_fired = true;
    // treatment can only happen once per wakeup
    detachInterrupt(treatment_pin);
}

hw_timer_t* led_timer = NULL;

uint64_t get_blink_period(health_t health) {
    if (is_infected_sym(health)) {
        return 1.5e6;
    } else if (is_infected_sym_late(health)) {
        return 1e5;
    }
    return 1e9;
}

void IRAM_ATTR on_timer() { digitalWrite(red_led_pin, !digitalRead(red_led_pin)); }

void configure_led_timer(uint64_t period_us) {
    led_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(led_timer, &on_timer, true);
    // wait time in microseconds
    timerAlarmWrite(led_timer, period_us, true);
    timerAlarmEnable(led_timer);  // Just Enable
}

void configure_hw(const struct PlayerState& player) {
    pinMode(green_led_pin, OUTPUT);
    pinMode(red_led_pin, OUTPUT);
    pinMode(treatment_pin, INPUT);
    attachInterrupt(treatment_pin, receive_treatment, RISING);

    if (is_immune(player.health.health)) {
        digitalWrite(green_led_pin, HIGH);
    } else if (is_zombie(player.health.health)) {
        digitalWrite(red_led_pin, HIGH);
    } else if (is_infected(player.health.health)) {
        auto const period = get_blink_period(player.health.health);
        configure_led_timer(period);
    }

    // start serial
    Serial.begin(115200);
}

template <std::size_t N>
void poll_wakeup_events(static_ring_buffer<Event, N>& event_queue) {
    auto const wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: {
            Serial.println("Wakeup caused by external signal using RTC_IO");
            event_queue.emplace_back(TreatmentEvent{});
            break;
        }
        case ESP_SLEEP_WAKEUP_EXT1: {
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER: {
            Serial.println("Wakeup caused by timer");
            break;
        }
        case ESP_SLEEP_WAKEUP_TOUCHPAD: {
            Serial.println("Wakeup caused by touchpad");
            break;
        }
        case ESP_SLEEP_WAKEUP_ULP: {
            Serial.println("Wakeup caused by ULP program");
            break;
        }
        default: {
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
        }
    }
}

template <std::size_t N>
void poll_bt_events(static_ring_buffer<Event, N>& event_queue) {
    // Scan for nearby devices and count infected
    auto devices = scan_ble();
    print_scan_results(devices);

    // Create exposure data from BT scan results
    auto const exposure = make_exposure_event_from_scan(devices);
    Serial.println("Infected human exposure: " + String(exposure.human));
    Serial.println("Infected cat exposure: " + String(exposure.cat));

    // Add exposure event to queue for this update
    event_queue.emplace_back(exposure);
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

void setup() {
    configure_hw(globals::player_state_persistent);

    Serial.println("Start Health: " + String(globals::player_state_persistent.health.health));

    // String representing our state (used by other devices to observe us)
    auto const display_state = to_display_state(globals::player_state_persistent.health.health);
    Serial.println(display_state.c_str());
    Serial.println("Health: " + String(globals::player_state_persistent.health.health));
    Serial.println("Cat Resistance: " +
                   String(globals::player_state_persistent.health.cat_resistance));

    // Start bluetooth to advertise our state
    BLEDevice::init(PREFIX_STR + display_state);
    BLEDevice::startAdvertising();

    // Event queue will hold semi-dynamic events to process on this tick
    static_ring_buffer<Event, 4> event_queue;

    // Get any events caused by device wakeup events
    poll_wakeup_events(event_queue);

    // Get any events from BT scan
    poll_bt_events(event_queue);

    // Run game update for all enqueued events
    game_update(
        globals::player_state_persistent,
        // get next event
        [&event_queue]() -> Event {
            if (globals::treament_received_interrupt_fired) {
                event_queue.emplace_back(TreatmentEvent{});
                globals::treament_received_interrupt_fired = false;
            }

            Event next_event;  // null state
            if (!event_queue.empty()) {
                next_event = event_queue.front();
                event_queue.pop_front();
            }

            return next_event;
        },
        // on exposure
        [](ExposureEvent const&) { Serial.println("Player exposed to virus"); },
        // on treatment
        [](TreatmentEvent const&) {
            Serial.println("Player administered treatment");

            // Do some beep boops
            show_treatment_animation();
        });

    // Flush the serial buffer
    Serial.flush();

    Serial.println("End Health: " + String(globals::player_state_persistent.health.health));

    // Enable waking up in some amount of time and sleep
    // Tests confirm that random does not produce the same number after reset
    esp_sleep_enable_timer_wakeup(random(1, TIME_TO_SLEEP) * uS_TO_S_FACTOR);

    // Wakeup when treatment pin goes high
    esp_sleep_enable_ext0_wakeup(treatment_pin, 1);
    esp_deep_sleep_start();
}

void loop() {
    // This is not going to be called
}
