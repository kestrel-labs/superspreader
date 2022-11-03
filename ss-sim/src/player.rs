// Data
use std::convert::From;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Player {
    pub tick: u32,
    pub health: u32,
    cat_resistance: bool,
}

// health starts at 2 for the starting condition of player
impl Default for Player {
    fn default() -> Self {
        Self {
            tick: u32::default(),
            health: 2,
            cat_resistance: bool::default(),
        }
    }
}

#[derive(Debug, Default)]
pub struct ExposureEvent {
    pub human: u32,
    pub cat: u32,
}

impl Player {
    pub fn exposure_update(&mut self, exposure: &ExposureEvent) {
        let state = HealthState::from(self.health);

        // Update health
        match state {
            HealthState::Zombie | HealthState::Immune => (),
            _ => {
                self.health +=
                    self.time_increase() - self.time_decrease() + self.exposure_increase(exposure);
            }
        }

        // Update cat resistance
        if !self.cat_resistance && state.is_infected() {
            self.cat_resistance = true;
        }

        // Pin zombie as maximum possible state
        if state == HealthState::Zombie {
            self.health = HealthState::Zombie.lower_limit();
        }
    }

    pub fn treatment_update(&mut self) {
        self.health = match HealthState::from(self.health) {
            HealthState::InfectedSymLate => HealthState::Immune.lower_limit(),
            HealthState::InfectedSym => HealthState::Healthy.lower_limit(),
            HealthState::InfectedAsym => self.health - 35,
            HealthState::Healthy => HealthState::SuperHealthy.lower_limit(),
            _ => self.health,
        }
    }

    fn exposure_increase(&self, exposure: &ExposureEvent) -> u32 {
        let state: HealthState = self.health.into();

        const HUMAN_INFECTION_RATE: u32 = 3;
        const CAT_INFECTION_RATE: u32 = 8;

        if state == HealthState::Immune || state.is_infected() {
            0
        } else {
            exposure.human * HUMAN_INFECTION_RATE
                + if self.cat_resistance {
                    0
                } else {
                    exposure.cat * CAT_INFECTION_RATE
                }
        }
    }

    fn time_increase(&self) -> u32 {
        // Checks health status, returns a 0 or constant value to
        // increment h by to track disease progress
        let state = HealthState::from(self.health);
        if state == HealthState::SuperHealthy {
            let sum = self.health + state.progress_rate();
            let remainder = sum % HealthState::Healthy.lower_limit();
            let quotent = sum / HealthState::Healthy.lower_limit();
            return state.progress_rate() - remainder * quotent;
        } else if state.is_infected() {
            return state.progress_rate();
        }
        0
    }

    fn time_decrease(&self) -> u32 {
        // checks health status, returns a 0 or a constant value to
        // decrement h by to track health desease
        let state = HealthState::from(self.health);
        if state == HealthState::Healthy {
            // preview the result
            let sum = self.health - state.progress_rate();

            // if the result is too healthy, do nothing
            return if sum > HealthState::Healthy.lower_limit() {
                state.progress_rate()
            } else {
                0
            };
        }
        0
    }
}

#[derive(Debug, PartialEq, Eq)]
enum HealthState {
    Immune,
    SuperHealthy,
    Healthy,
    InfectedAsym,
    InfectedSym,
    InfectedSymLate,
    Zombie,
}

impl From<u32> for HealthState {
    fn from(health: u32) -> Self {
        match health {
            0..=1 => HealthState::Immune,
            2..=9 => HealthState::SuperHealthy,
            10..=39 => HealthState::Healthy,
            40..=69 => HealthState::InfectedAsym,
            70..=89 => HealthState::InfectedSym,
            90..=99 => HealthState::InfectedSymLate,
            _ => HealthState::Zombie,
        }
    }
}

impl HealthState {
    fn progress_rate(&self) -> u32 {
        match self {
            HealthState::Immune => 0,
            HealthState::SuperHealthy => 2,
            _ => 1,
        }
    }

    fn is_infected(&self) -> bool {
        !matches!(
            self,
            HealthState::Immune | HealthState::Healthy | HealthState::SuperHealthy
        )
    }

    fn lower_limit(&self) -> u32 {
        match self {
            Self::Immune => 0,
            Self::SuperHealthy => 2,
            Self::Healthy => 10,
            Self::InfectedAsym => 40,
            Self::InfectedSym => 70,
            Self::InfectedSymLate => 90,
            Self::Zombie => 100,
        }
    }
}

// TESTS ///////////////////////////////////////////////////

#[cfg(test)]
mod tests {
    use super::*;
    use matches::assert_matches;

    #[test]
    fn default_player() {
        let default_player = Player::default();
        assert_eq!(default_player.tick, 0);
        assert_eq!(default_player.health, 2);
        assert!(!default_player.cat_resistance);
    }

    #[test]
    fn default_exposure_event() {
        assert_eq!(ExposureEvent::default().human, 0);
        assert_eq!(ExposureEvent::default().cat, 0);
    }

    #[test]
    fn time_decrease() {
        let very_healthy_player = Player {
            health: 10,
            ..Player::default()
        };
        assert_eq!(
            HealthState::from(very_healthy_player.health),
            HealthState::Healthy
        );

        let late_healthy_player = Player {
            health: 30,
            ..Player::default()
        };
        assert_eq!(
            HealthState::from(late_healthy_player.health),
            HealthState::Healthy
        );

        let not_healthy_player = Player {
            health: 70,
            ..Player::default()
        };
        assert_ne!(
            HealthState::from(not_healthy_player.health),
            HealthState::Healthy
        );

        let healthy_progress_rate = HealthState::Healthy.progress_rate();

        assert_eq!(very_healthy_player.time_decrease(), 0);
        assert_eq!(late_healthy_player.time_decrease(), healthy_progress_rate);
        assert_eq!(not_healthy_player.time_decrease(), 0);
    }

    #[test]
    fn increase_exposure_how_much() {
        let default_exposure = ExposureEvent::default();

        assert_eq!(
            Player::default().exposure_increase(&default_exposure),
            0,
            "default constructed player and exposure should not increase"
        );

        // create some more interesting cases
        let double_exposure = ExposureEvent { human: 2, cat: 2 };
        let cat_exposure = ExposureEvent { human: 0, cat: 1 };
        let imume_player = Player {
            health: 1, // immune
            ..Player::default()
        };
        assert_eq!(HealthState::from(imume_player.health), HealthState::Immune);
        let healthy_player = Player {
            health: 15,
            ..Player::default()
        };
        assert_eq!(
            HealthState::from(healthy_player.health),
            HealthState::Healthy
        );
        let infected_asym_player = Player {
            health: 45,
            ..Player::default()
        };
        assert_eq!(
            HealthState::from(infected_asym_player.health),
            HealthState::InfectedAsym
        );
        let healthy_cat_resistant_player = Player {
            health: 11,
            cat_resistance: true,
            ..Player::default()
        };
        assert_eq!(
            HealthState::from(healthy_cat_resistant_player.health),
            HealthState::Healthy
        );

        assert_eq!(
            imume_player.exposure_increase(&double_exposure),
            0,
            " immune player does not increase when exposed"
        );
        assert_eq!(
            healthy_player.exposure_increase(&double_exposure),
            22,
            "healthy player increases by 22 when exposed to two infected humans and two cats"
        );
        assert_eq!(
            infected_asym_player.exposure_increase(&double_exposure),
            0,
            "infected player does not increase when exposed to cats or infected humans"
        );
        assert_eq!(
            healthy_cat_resistant_player.exposure_increase(&cat_exposure),
            0,
            "healthy player with cat resistance does not increase when exposed to cats"
        );
    }

    #[test]
    fn health_time_increase() {
        let super_healthy_player = Player {
            health: 5,
            ..Player::default()
        };
        assert!(super_healthy_player.time_increase() > 0);
        let healthy_player = Player {
            health: 11,
            ..Player::default()
        };
        assert_eq!(healthy_player.time_increase(), 0);
        let infected_player = Player {
            health: 75,
            ..Player::default()
        };
        assert!(infected_player.time_increase() > 0);
    }

    #[test]
    fn player_exposure_update() {
        let double_exposure = ExposureEvent { human: 2, cat: 2 };
        let cat_exposure = ExposureEvent { human: 0, cat: 1 };
        let mut immune_player = Player {
            health: 0,
            ..Player::default()
        };
        let mut healthy_player_with_cat_resistance = Player {
            health: 11,
            cat_resistance: true,
            ..Player::default()
        };
        let mut infected_player = Player {
            health: 75,
            ..Player::default()
        };
        let mut late_infected_player = Player {
            health: 99,
            ..Player::default()
        };
        let mut super_zombie_player = Player {
            health: 110,
            ..Player::default()
        };

        let pre_exposure = immune_player.clone();
        immune_player.exposure_update(&double_exposure);
        assert_eq!(immune_player, pre_exposure);

        let pre_exposure = healthy_player_with_cat_resistance.clone();
        healthy_player_with_cat_resistance.exposure_update(&cat_exposure);
        assert_eq!(healthy_player_with_cat_resistance, pre_exposure);

        let pre_exposure = healthy_player_with_cat_resistance.clone();
        healthy_player_with_cat_resistance.exposure_update(&double_exposure);
        assert_ne!(healthy_player_with_cat_resistance, pre_exposure);
        assert!(healthy_player_with_cat_resistance.health > pre_exposure.health);

        let pre_exposure = infected_player.clone();
        infected_player.exposure_update(&cat_exposure);
        assert_ne!(infected_player, pre_exposure);

        let pre_exposure = late_infected_player.clone();
        late_infected_player.exposure_update(&double_exposure);
        assert_ne!(late_infected_player, pre_exposure);
        assert_eq!(
            late_infected_player.health,
            HealthState::Zombie.lower_limit()
        );

        let pre_exposure = super_zombie_player.clone();
        super_zombie_player.exposure_update(&double_exposure);
        assert_ne!(super_zombie_player, pre_exposure);
        assert_eq!(
            super_zombie_player.health,
            HealthState::Zombie.lower_limit()
        );
    }

    #[test]
    fn player_treatment_update() {
        let mut late_infected_player = Player {
            health: HealthState::InfectedSymLate.lower_limit(),
            ..Player::default()
        };
        let mut symtomatic_player = Player {
            health: HealthState::InfectedSym.lower_limit(),
            ..Player::default()
        };
        let mut asymtomatic_player = Player {
            health: HealthState::InfectedAsym.lower_limit(),
            ..Player::default()
        };
        let mut healthy_player = Player {
            health: HealthState::Healthy.lower_limit(),
            ..Player::default()
        };
        let mut zombie_player = Player {
            health: HealthState::Zombie.lower_limit(),
            ..Player::default()
        };

        late_infected_player.treatment_update();
        assert_eq!(
            HealthState::from(late_infected_player.health),
            HealthState::Immune,
            "late infected players become imune after being treated"
        );
        symtomatic_player.treatment_update();
        assert_eq!(
            HealthState::from(symtomatic_player.health),
            HealthState::Healthy,
            "symtomatic players becomes healthy after being treated"
        );
        asymtomatic_player.treatment_update();
        // asymtomatic players becomes healthy or super healthy after being treated
        assert_matches!(
            HealthState::from(asymtomatic_player.health),
            HealthState::Healthy | HealthState::SuperHealthy
        );
        healthy_player.treatment_update();
        assert_eq!(
            HealthState::from(healthy_player.health),
            HealthState::SuperHealthy,
            "healthy players becomes super healthy after being treated"
        );
        zombie_player.treatment_update();
        assert_eq!(
            HealthState::from(zombie_player.health),
            HealthState::Zombie,
            "zombie players are still zombies after being treated"
        );
    }

    #[test]
    fn number_to_health_state() {
        assert_eq!(HealthState::from(0), HealthState::Immune);
        assert_eq!(HealthState::from(1), HealthState::Immune);
        assert_eq!(HealthState::from(2), HealthState::SuperHealthy);
        assert_eq!(HealthState::from(9), HealthState::SuperHealthy);
        assert_eq!(HealthState::from(10), HealthState::Healthy);
        assert_eq!(HealthState::from(39), HealthState::Healthy);
        assert_eq!(HealthState::from(40), HealthState::InfectedAsym);
        assert_eq!(HealthState::from(69), HealthState::InfectedAsym);
        assert_eq!(HealthState::from(70), HealthState::InfectedSym);
        assert_eq!(HealthState::from(89), HealthState::InfectedSym);
        assert_eq!(HealthState::from(90), HealthState::InfectedSymLate);
        assert_eq!(HealthState::from(99), HealthState::InfectedSymLate);
        assert_eq!(HealthState::from(100), HealthState::Zombie);
        assert_eq!(HealthState::from(999), HealthState::Zombie);
    }

    #[test]
    fn health_state_progress_rate() {
        assert_eq!(HealthState::Immune.progress_rate(), 0);
        assert_eq!(HealthState::SuperHealthy.progress_rate(), 2);
        assert_eq!(HealthState::Healthy.progress_rate(), 1);
        assert_eq!(HealthState::Zombie.progress_rate(), 1);
    }

    #[test]
    fn is_infected() {
        assert!(!HealthState::Immune.is_infected());
        assert!(!HealthState::SuperHealthy.is_infected());
        assert!(!HealthState::Healthy.is_infected());
        assert!(HealthState::InfectedAsym.is_infected());
        assert!(HealthState::Zombie.is_infected());
    }

    #[test]
    fn health_lower_limit() {
        assert_eq!(
            HealthState::from(HealthState::Immune.lower_limit()),
            HealthState::Immune
        );
        assert_ne!(
            HealthState::from(HealthState::SuperHealthy.lower_limit() - 1),
            HealthState::SuperHealthy
        );
        assert_eq!(
            HealthState::from(HealthState::SuperHealthy.lower_limit()),
            HealthState::SuperHealthy
        );
        assert_ne!(
            HealthState::from(HealthState::Healthy.lower_limit() - 1),
            HealthState::Healthy
        );
        assert_eq!(
            HealthState::from(HealthState::Healthy.lower_limit()),
            HealthState::Healthy
        );
        assert_ne!(
            HealthState::from(HealthState::InfectedAsym.lower_limit() - 1),
            HealthState::InfectedAsym
        );
        assert_eq!(
            HealthState::from(HealthState::InfectedAsym.lower_limit()),
            HealthState::InfectedAsym
        );
        assert_ne!(
            HealthState::from(HealthState::InfectedSym.lower_limit() - 1),
            HealthState::InfectedSym
        );
        assert_eq!(
            HealthState::from(HealthState::InfectedSym.lower_limit()),
            HealthState::InfectedSym
        );
        assert_ne!(
            HealthState::from(HealthState::InfectedSymLate.lower_limit() - 1),
            HealthState::InfectedSymLate
        );
        assert_eq!(
            HealthState::from(HealthState::InfectedSymLate.lower_limit()),
            HealthState::InfectedSymLate
        );
        assert_ne!(
            HealthState::from(HealthState::Zombie.lower_limit() - 1),
            HealthState::Zombie
        );
        assert_eq!(
            HealthState::from(HealthState::Zombie.lower_limit()),
            HealthState::Zombie
        );
    }
}
