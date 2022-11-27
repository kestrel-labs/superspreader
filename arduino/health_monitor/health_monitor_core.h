// Defines core player update logic for health monitor device

#pragma once

// C++ Standard Library
#include <cmath>
#include <type_traits>
#include <utility>

/** @file **/

/**< health is tracked with an unsigned int */
using health_t = unsigned int;

/**
 * @brief Set of health states and their bounds.
 *        For example,
 * @code
 *      HEALTHY = [10, 40)
 *      ZOMBIE = 99
 * @endcode
 *
 */
enum struct StateBounds : health_t {
    IMMUNE            = 1,  /**< Cannot be infected */
    SUPER_HEALTHY     = 2,  /**< Susceptible to infection, but takes longer */
    HEALTHY           = 10, /**< Susceptible to infection */
    INFECTED_ASYM     = 40, /**< Contagious, treatable, asymptomatic */
    INFECTED_SYM      = 70, /**< Contagious, treatable, symptomatic */
    INFECTED_SYM_LATE = 90, /**< Contagious, curable, symptomatic */
    ZOMBIE            = 99, /**< Contagious, incurable, symptomatic */
};

/**
 * @brief Rate of health progression per tick, given a health state
 */
enum struct ProgressRate : health_t {
    SUPER_HEALTHY = 2,
    HEALTHY       = 1,
    INFECTED      = 1,
};

/**
 * @brief Rate of health progression per exposure type
 */
enum struct InfectionRate : health_t {
    CAT   = 8,
    HUMAN = 3,
};

/**
 * @brief Casting function to use enums for math
 * @tparam Type to convert, such at @c StateBounds, @c ProgressRate, or @p InfectionRate
 * @param bounds Enum to convert
 * @returns health value for doing math
 */
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

/**
 * @brief Contains values related to player health and susceptibility
 */
struct HealthState {
    /**
     * @brief Numerical health value. Lower is more healthy.
     * @note If a function such as @p to_health is used to initialize
     *       then the value after every boot on RTC memory
     */
    health_t health = 2;  // super healthy
};

/** @brief Contains health and game state */
struct PlayerState {
    int tick = 0;       /**< Current game tick */
    HealthState health; /**< Current player health */
};

/** @brief Contains number of exposures from various sources */
struct ExposureEvent {
    health_t human = 0;
    health_t cat   = 0;
};

/** @brief Indicates a treatment event has occurred */
struct TreatmentEvent {};

/** @brief Encapsulates closed set of event types for queueing */
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

/**
 * @brief Executes different handlers based on the event type
 * @param event to process
 * @param exposure_handler called for exposure events
 * @param treatment_handler called for treatment events
 */
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

/**
 * @brief Computes a new health value as a function of current health, progression, and exposures
 * @param health_state Current health
 * @param exposures Possible sources of infection
 * @returns the new health affected by exposure
 */
HealthState exposure_update(HealthState health_state, ExposureEvent const exposures);

// updates health according to treatment rules
/**
 * @brief Computes a new health value given a treatment is applied
 * @param health_state Current health
 * @returns new health given treatment
 */
HealthState treament_update(HealthState health_state);

/** @brief Factory for building a new player */
PlayerState new_player_state();

/**
 * @brief Updates player health while also polling for new events
 * @param player to change health
 * @param get_next_event Closure that polls for new events to process
 * @param on_exposure Context specific function to call after exposure update
 * @param on_treatment Context specific function to call after treatment update
 */
template <typename GetNextEventT, typename OnExposureT, typename OnTreatmentT>
void game_update(PlayerState& player,
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
