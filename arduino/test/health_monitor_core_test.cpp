// GTest
#include <gtest/gtest.h>

// Superspreader
#include "health_monitor_core.h"

TEST(TreatmentUpdateTests, SuperHealthyAbsorbing) {
    const HealthState init_health{.health         = to_health(StateBounds::SUPER_HEALTHY),
                                  .cat_resistance = false};
    auto const next_health = treament_update(init_health);
    EXPECT_EQ(next_health.health, init_health.health);
}

TEST(TreatmentUpdateTests, HealthyToSuperHealthy) {
    const HealthState init_health{.health         = to_health(StateBounds::HEALTHY),
                                  .cat_resistance = false};
    auto const next_health = treament_update(init_health);
    ASSERT_LT(next_health.health, init_health.health);
    EXPECT_EQ(next_health.health, to_health(StateBounds::SUPER_HEALTHY));
}

TEST(TreatmentUpdateTests, InfectedAsymToHealthy) {
    const HealthState init_health{.health         = to_health(StateBounds::INFECTED_ASYM),
                                  .cat_resistance = false};
    auto const next_health = treament_update(init_health);
    ASSERT_LT(next_health.health, init_health.health);
    EXPECT_LT(next_health.health, to_health(StateBounds::HEALTHY));
}

TEST(TreatmentUpdateTests, InfectedSymToHealthy) {
    const HealthState init_health{.health         = to_health(StateBounds::INFECTED_SYM),
                                  .cat_resistance = false};
    auto const next_health = treament_update(init_health);
    ASSERT_LT(next_health.health, init_health.health);
    EXPECT_EQ(next_health.health, to_health(StateBounds::HEALTHY));
}

TEST(TreatmentUpdateTests, InfectedSymLateToImmune) {
    const HealthState init_health{.health         = to_health(StateBounds::INFECTED_SYM_LATE),
                                  .cat_resistance = false};
    auto const next_health = treament_update(init_health);
    ASSERT_LT(next_health.health, init_health.health);
    EXPECT_EQ(next_health.health, to_health(StateBounds::IMMUNE));
}

TEST(TreatmentUpdateTests, ZombieStaysZombie) {
    const HealthState init_health{.health         = to_health(StateBounds::ZOMBIE),
                                  .cat_resistance = false};
    auto const next_health = treament_update(init_health);
    ASSERT_EQ(next_health.health, init_health.health);
    EXPECT_EQ(next_health.health, to_health(StateBounds::ZOMBIE));
}

TEST(ExposureUpdateTests, NoExposureCausesNoChange) {
    const HealthState init_health{.health         = to_health(StateBounds::HEALTHY),
                                  .cat_resistance = false};
    auto const next_health = exposure_update(init_health,
                                             ExposureEvent{
                                                 .human = 0,
                                                 .cat   = 0,
                                             });
    ASSERT_EQ(next_health.health, init_health.health);
    ASSERT_EQ(next_health.cat_resistance, init_health.cat_resistance);
    EXPECT_EQ(next_health.health, to_health(StateBounds::HEALTHY));
}

TEST(ExposureUpdateTests, NoExposureHealthyDecay) {
    const HealthState init_health{.health         = to_health(StateBounds::HEALTHY) + 5,
                                  .cat_resistance = false};
    auto const next_health = exposure_update(init_health,
                                             ExposureEvent{
                                                 .human = 0,
                                                 .cat   = 0,
                                             });
    ASSERT_LT(next_health.health, init_health.health);
    ASSERT_EQ(next_health.cat_resistance, init_health.cat_resistance);
    EXPECT_GT(next_health.health, to_health(StateBounds::HEALTHY));
}

TEST(ExposureUpdateTests, NoExposureSuperHealthyDecay) {
    const HealthState init_health{.health         = to_health(StateBounds::SUPER_HEALTHY),
                                  .cat_resistance = false};
    auto const next_health = exposure_update(init_health,
                                             ExposureEvent{
                                                 .human = 0,
                                                 .cat   = 0,
                                             });
    ASSERT_GT(next_health.health, init_health.health);
    ASSERT_EQ(next_health.cat_resistance, init_health.cat_resistance);
    EXPECT_GT(next_health.health, to_health(StateBounds::SUPER_HEALTHY));
    EXPECT_LT(next_health.health, to_health(StateBounds::HEALTHY));
}

TEST(ExposureUpdateTests, ConsistentExposureCausesInfection) {
    HealthState curr_health{.health = to_health(StateBounds::HEALTHY), .cat_resistance = false};

    static constexpr int kExpectedIterToInfectedAsym = 15;
    for (int i = 0; i < kExpectedIterToInfectedAsym; ++i) {
        auto const next_health = exposure_update(curr_health,
                                                 ExposureEvent{
                                                     .human = 1,
                                                     .cat   = 0,
                                                 });
        ASSERT_GT(next_health.health, curr_health.health);
        curr_health = next_health;
        if (is_infected(next_health.health)) {
            break;
        }
    }

    ASSERT_GT(curr_health.health, to_health(StateBounds::HEALTHY));
    EXPECT_GE(curr_health.health, to_health(StateBounds::INFECTED_ASYM));
}

TEST(ExposureUpdateTests, ConsistentExposureToManyCausesInfectionFaster) {
    HealthState curr_health{.health = to_health(StateBounds::HEALTHY), .cat_resistance = false};

    static constexpr int kExpectedIterToInfectedAsym = 4;
    for (int i = 0; i < kExpectedIterToInfectedAsym; ++i) {
        auto const next_health = exposure_update(curr_health,
                                                 ExposureEvent{
                                                     .human = 3,
                                                     .cat   = 0,
                                                 });
        ASSERT_GT(next_health.health, curr_health.health);
        curr_health = next_health;
        if (is_infected(next_health.health)) {
            break;
        }
    }

    ASSERT_GT(curr_health.health, to_health(StateBounds::HEALTHY));
    EXPECT_GE(curr_health.health, to_health(StateBounds::INFECTED_ASYM));
}

TEST(ExposureUpdateTests, ConsistentExposureAfterInfectionCausesZombie) {
    HealthState curr_health{.health         = to_health(StateBounds::INFECTED_ASYM),
                            .cat_resistance = false};

    static constexpr int kExpectedIterToZombie = 59;
    for (int i = 0; i < kExpectedIterToZombie; ++i) {
        auto const next_health = exposure_update(curr_health,
                                                 ExposureEvent{
                                                     .human = 3,
                                                     .cat   = 0,
                                                 });
        ASSERT_GT(next_health.health, curr_health.health);
        curr_health = next_health;
        if (is_zombie(next_health.health)) {
            break;
        }
    }

    ASSERT_GT(curr_health.health, to_health(StateBounds::INFECTED_ASYM));
    EXPECT_GE(curr_health.health, to_health(StateBounds::ZOMBIE));
}

TEST(ExposureUpdateTests, CatExposureCausesCatResistance) {
    HealthState curr_health{.health = to_health(StateBounds::HEALTHY), .cat_resistance = false};

    static constexpr int kMaxIterToCatResist = 10;
    for (int i = 0; i < kMaxIterToCatResist; ++i) {
        auto const next_health = exposure_update(curr_health,
                                                 ExposureEvent{
                                                     .human = 0,
                                                     .cat   = 1,
                                                 });
        ASSERT_GT(next_health.health, curr_health.health);
        bool const cat_resistance_changed =
            next_health.cat_resistance != curr_health.cat_resistance;
        curr_health = next_health;
        if (cat_resistance_changed) {
            break;
        }
    }

    ASSERT_TRUE(curr_health.cat_resistance);
    ASSERT_GT(curr_health.health, to_health(StateBounds::HEALTHY));
    EXPECT_GE(curr_health.health, to_health(StateBounds::INFECTED_ASYM));
}

TEST(HealthMonitorCoreTests, NewPlayer) {
    const PlayerState player = new_player_state();
    EXPECT_EQ(player.tick, 0);
    EXPECT_EQ(player.health.health, to_health(StateBounds::SUPER_HEALTHY));
    EXPECT_FALSE(player.health.cat_resistance);
}

TEST(HealthMonitorCoreTests, SingleExposureEvent) {
    const PlayerState player_init = new_player_state();
    PlayerState player            = player_init;

    int iterations              = 0;
    int exposure_events_handled = 0;
    int treament_events_handled = 0;
    game_update(
        player,
        [&iterations]() -> Event {
            const auto it = iterations++;
            if (it < 1) {
                return Event{ExposureEvent{.human = 3, .cat = 1}};
            } else {
                return Event{};
            }
        },
        [&exposure_events_handled](ExposureEvent const& exposure_event) {
            ++exposure_events_handled;
        },
        [&treament_events_handled](TreatmentEvent const& exposure_event) {
            ++treament_events_handled;
        });

    ASSERT_EQ(exposure_events_handled, 1);
    ASSERT_EQ(treament_events_handled, 0);

    ASSERT_GT(player.tick, player_init.tick);
    EXPECT_EQ(player.tick, 1);

    ASSERT_GT(player.health.health, player_init.health.health);
    EXPECT_GT(player.health.health, to_health(StateBounds::HEALTHY));
}

TEST(HealthMonitorCoreTests, SingleExposureEventThenSingleTreatmentEvent) {
    const PlayerState player_init = new_player_state();
    PlayerState player            = player_init;

    int iterations              = 0;
    int exposure_events_handled = 0;
    int treament_events_handled = 0;
    game_update(
        player,
        [&iterations]() -> Event {
            const auto it = iterations++;
            if (it < 1) {
                return Event{ExposureEvent{.human = 3, .cat = 1}};
            } else if (it < 2) {
                return Event{TreatmentEvent{}};
            } else {
                return Event{};
            }
        },
        [&exposure_events_handled](ExposureEvent const& exposure_event) {
            ++exposure_events_handled;
        },
        [&treament_events_handled](TreatmentEvent const& exposure_event) {
            ++treament_events_handled;
        });

    ASSERT_EQ(exposure_events_handled, 1);
    ASSERT_EQ(treament_events_handled, 1);

    ASSERT_GT(player.tick, player_init.tick);
    EXPECT_EQ(player.tick, 1);

    ASSERT_EQ(player.health.health, player_init.health.health);
    EXPECT_EQ(player.health.health, to_health(StateBounds::SUPER_HEALTHY));
}

TEST(HealthMonitorCoreTests, ManyExposureEventsToZombie) {
    const PlayerState player_init = new_player_state();
    PlayerState player            = player_init;

    int exposure_events_handled = 0;
    int treament_events_handled = 0;
    game_update(
        player,
        [&player]() -> Event {
            if (!is_zombie(player.health.health)) {
                return Event{ExposureEvent{.human = 3, .cat = 1}};
            } else {
                return Event{};
            }
        },
        [&exposure_events_handled](ExposureEvent const& exposure_event) {
            ++exposure_events_handled;
        },
        [&treament_events_handled](TreatmentEvent const& exposure_event) {
            ++treament_events_handled;
        });

    ASSERT_EQ(exposure_events_handled, 49);
    ASSERT_EQ(treament_events_handled, 0);

    ASSERT_GT(player.tick, player_init.tick);
    EXPECT_EQ(player.tick, 1);

    ASSERT_TRUE(player.health.cat_resistance);
    ASSERT_GT(player.health.health, player_init.health.health);
    EXPECT_EQ(player.health.health, to_health(StateBounds::ZOMBIE));
}
