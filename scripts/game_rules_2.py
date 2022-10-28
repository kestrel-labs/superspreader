# Input Parameters
from random import randrange
import matplotlib.pyplot as plt

class HealthState():
    def __init__(self, health, cat_resistance):
        self.health = health
        self.cat_resistance = cat_resistance #resistant to cats when cat_resistance = 1

class Exposure():
    def __init__(self, human, cat):
        self.human = human
        self.cat = cat

class StateBounds():
    IMMUNE = 1
    SUPER_HEALTHY = 2
    HEALTHY = 10
    INFECTED_ASYM = 40
    INFECTED_SYM = 70
    INFECTED_SYM_LATE = 90
    ZOMBIE = 99

class ProgressRate():
    SUPER_HEALTHY = 2
    INFECTED = 6

class InfectionRate():
    CAT = 2
    HUMAN = 1


# Define health states

def is_immune(health):
    return health <= StateBounds.IMMUNE

def is_super_healthy(health):
    return StateBounds.SUPER_HEALTHY <= health and health < StateBounds.HEALTHY

def is_healthy(health):
    return StateBounds.HEALTHY <= health and health < StateBounds.INFECTED_ASYM

def is_infected(health): #Covers all infected states before ZOMBIE
    return StateBounds.INFECTED_ASYM <= health

def is_infected_asym(health):
    return StateBounds.INFECTED_ASYM <= health and health < StateBounds.INFECTED_SYM

def is_infected_sym(health):
    return StateBounds.INFECTED_SYM <= health and health < StateBounds.INFECTED_SYM_LATE

def is_infected_sym_late(health):
    return StateBounds.INFECTED_SYM_LATE <= health and health < StateBounds.ZOMBIE

def is_zombie(health):
    return StateBounds.ZOMBIE <= health



# Determine health count


def health_update(health_state,exposures): #takes current health state (count and cat resistance), count of humans and cats nearby
    if is_zombie(health_state.health):
        health_state.health = StateBounds.ZOMBIE
    if is_immune(health_state.health):
        health_state.health = StateBounds.IMMUNE
    health_state.health = health_state.health + time_increase(health_state.health) + exposure_increase(health_state,exposures)
    health_state.cat_resistance = is_infected(health_state.health)
    if is_zombie(health_state.health):
        health_state.health = StateBounds.ZOMBIE
    return health_state

def time_increase(health): #Checks health status, returns a 0 or constant value to increment h by to track disease progress
    if is_super_healthy(health):
        sum = health + ProgressRate.SUPER_HEALTHY
        remainder = sum % StateBounds.HEALTHY
        quotient = sum // StateBounds.HEALTHY
        return  ProgressRate.SUPER_HEALTHY - remainder*quotient
    if is_infected(health):
        return ProgressRate.INFECTED
    return 0

def exposure_increase(health_state,exposures):
    if is_immune(health_state.health) or is_infected(health_state.health):
        return 0
    return exposures.human*InfectionRate.HUMAN + exposures.cat*InfectionRate.CAT*(not health_state.cat_resistance) #resistant to cats when cat_resistance = 1











# Electric boogaloo
