// Defines core player update logic for health monitor device

#ifndef HEALTH_MONITOR_CORE_H
#define HEALTH_MONITOR_CORE_H

// C++ Standard Library
#include <cmath>
#include <type_traits>
#include <utility>

// health is tracked with an unsigned int
using health_t = unsigned int;

// these are the bounds of the different types of health
enum struct StateBounds : health_t {
    IMMUNE            = 1,
    SUPER_HEALTHY     = 2,
    HEALTHY           = 10,
    INFECTED_ASYM     = 40,
    INFECTED_SYM      = 70,
    INFECTED_SYM_LATE = 90,
    ZOMBIE            = 99,
};

enum struct ProgressRate : health_t {
    SUPER_HEALTHY = 2,
    HEALTHY       = 1,
    INFECTED      = 1,
};

enum struct InfectionRate : health_t {
    CAT   = 8,
    HUMAN = 3,
};

template <typename T>
constexpr health_t to_health(T bounds) {
    static_assert(std::is_same<typename std::underlying_type<T>::type, health_t>());
    return static_cast<health_t>(bounds);
}

constexpr bool is_immune(health_t health) { return health <= to_health(StateBounds::IMMUNE); }

constexpr bool is_super_healthy(health_t health) {
    return to_health(StateBounds::SUPER_HEALTHY) <= health &&
           health < to_health(StateBounds::HEALTHY);
}

constexpr bool is_healthy(health_t health) {
    return to_health(StateBounds::HEALTHY) <= health &&
           health < to_health(StateBounds::INFECTED_ASYM);
}

constexpr bool is_infected(health_t health) {
    return to_health(StateBounds::INFECTED_ASYM) <= health &&
           health < to_health(StateBounds::ZOMBIE);
}

constexpr bool is_infected_asym(health_t health) {
    return to_health(StateBounds::INFECTED_ASYM) <= health &&
           health < to_health(StateBounds::INFECTED_SYM);
}

constexpr bool is_infected_sym(health_t health) {
    return to_health(StateBounds::INFECTED_SYM) <= health &&
           health < to_health(StateBounds::INFECTED_SYM_LATE);
}

constexpr bool is_infected_sym_late(health_t health) {
    return to_health(StateBounds::INFECTED_SYM_LATE) <= health &&
           health < to_health(StateBounds::ZOMBIE);
}

constexpr bool is_zombie(health_t health) { return to_health(StateBounds::ZOMBIE) <= health; }

struct HealthState {
    // using a function resets the value after every boot
    health_t health     = 2;  // super healthy
    bool cat_resistance = false;
};

struct PlayerState {
    /// Current game tick
    int tick = 0;

    /// Current player health
    HealthState health;
};

struct ExposureEvent {
    health_t human = 0;
    health_t cat   = 0;
};

struct TreatmentEvent {};

struct Event {
    enum class Type {
        NIL       = 0,
        EXPOSURE  = 1,
        TREATMENT = 2,
    };

    Type type;

    union EventData {
        ExposureEvent exposure;
        TreatmentEvent treatment;
    };

    EventData data;

    Event() : type{Type::NIL}, data{} {}
    explicit Event(ExposureEvent _data) : type{Type::EXPOSURE}, data{.exposure = _data} {}
    explicit Event(TreatmentEvent _data) : type{Type::TREATMENT}, data{.treatment = _data} {}

    constexpr bool is_valid() const { return type != Type::NIL; }
};

template <typename ExposureEventHandler, typename TreatmentEventHandler>
void visit(Event const& event,
           ExposureEventHandler exposure_handler,
           TreatmentEventHandler treatment_handler) {
    switch (event.type) {
        case Event::Type::NIL:
            break;
        case Event::Type::EXPOSURE:
            exposure_handler(event.data.exposure);
            break;
        case Event::Type::TREATMENT:
            treatment_handler(event.data.treatment);
            break;
    }
}

// takes current health state (count and cat resistance), count of humans and cats nearby
HealthState exposure_update(HealthState health_state, ExposureEvent const exposures);

// updates health according to treatment rules
HealthState treament_update(HealthState health_state);

void game_reset(struct PlayerState& player);

template <typename GetNextEventT, typename OnExposureT, typename OnTreatmentT>
void game_update(struct PlayerState& player,
                 GetNextEventT get_next_event,
                 OnExposureT on_exposure,
                 OnTreatmentT on_treatment) {
    // Tick time alive
    ++player.tick;

    // Latching treatment flag
    bool treatment_previously_received = false;

    // Handle all events
    while (true) {
        // Get next enqueue event
        auto const event = get_next_event();

        // If there are no more events, stop processing
        if (!event.is_valid()) {
            return;
        }

        // Run core game event logic
        visit(
            event,
            [&on_exposure, &player](ExposureEvent const& exposure) {
                // Do normal player health update
                player.health = exposure_update(player.health, exposure);

                // Call any context-specific callbacks
                on_exposure(exposure);
            },
            [&on_treatment, &player, &treatment_previously_received](
                TreatmentEvent const& treatment) {
                // Skip if player has already been treated on this tick
                if (treatment_previously_received) {
                    return;
                }

                // Apply treatment modifier to player health
                player.health = treament_update(player.health);

                // Set latching flag to indicate treatment already happened on this tick
                treatment_previously_received = true;

                // Call any context-specific callbacks
                on_treatment(treatment);
            });
    }
}

#endif  // HEALTH_MONITOR_CORE_H
