// Originally from game_rules_2.py

// Game
#include "health_monitor_core.h"

namespace {

/** @returns amount to increase @p health_state value by, given @p exposures */
health_t exposure_increase(HealthState const health_state, ExposureEvent const exposures) {
    if (is_immune(health_state.health) || is_infected(health_state.health)) return 0;

    // resistant to cats when cat_resistance = 1
    return exposures.human * to_health(InfectionRate::HUMAN) +
           exposures.cat * to_health(InfectionRate::CAT) * (health_state.cat_resistance ? 0 : 1);
}

/** @returns amount to increase @p health by, given current progression */
health_t time_increase(health_t health) {
    if (is_super_healthy(health)) {
        auto const sum       = health + to_health(ProgressRate::SUPER_HEALTHY);
        auto const remainder = sum % to_health(StateBounds::HEALTHY);
        auto const quotient  = std::floor(sum / to_health(StateBounds::HEALTHY));
        return to_health(ProgressRate::SUPER_HEALTHY) - remainder * quotient;
    }
    if (is_infected(health)) return to_health(ProgressRate::INFECTED);
    return 0;
}

/** @returns amount to decrease @p health by, given current progression */
health_t time_decrease(health_t health) {
    if (is_healthy(health)) {
        // Preview the result
        auto const sum = health - to_health(ProgressRate::HEALTHY);
        // If the result is too healthy, do nothing
        return sum > to_health(StateBounds::HEALTHY) ? to_health(ProgressRate::HEALTHY) : 0;
    }
    return 0;
}

}  // namespace

HealthState exposure_update(HealthState health_state, ExposureEvent const exposures) {
    if (is_zombie(health_state.health)) {
        health_state.health = to_health(StateBounds::ZOMBIE);
        return health_state;
    }
    if (is_immune(health_state.health)) {
        health_state.health = to_health(StateBounds::IMMUNE);
        return health_state;
    }
    health_state.health = health_state.health + time_increase(health_state.health) -
                          time_decrease(health_state.health) +
                          exposure_increase(health_state, exposures);
    // Cat resistance is permanent
    health_state.cat_resistance |= is_infected(health_state.health);
    if (is_zombie(health_state.health)) {
        health_state.health = to_health(StateBounds::ZOMBIE);
        return health_state;
    }
    return health_state;
}

HealthState treament_update(HealthState health_state) {
    if (is_infected_sym_late(health_state.health)) {
        health_state.health = to_health(StateBounds::IMMUNE);
    } else if (is_infected_sym(health_state.health)) {
        health_state.health = to_health(StateBounds::HEALTHY);
    } else if (is_infected_asym(health_state.health)) {
        health_state.health -= 35;  // sometimes you go healthy, other times super
    } else if (is_healthy(health_state.health)) {
        health_state.health = to_health(StateBounds::SUPER_HEALTHY);
    }

    return health_state;
}

PlayerState new_player_state() {
    PlayerState player;
    player.tick                  = 0;
    player.health.health         = to_health(StateBounds::SUPER_HEALTHY);
    player.health.cat_resistance = false;
    return player;
}
