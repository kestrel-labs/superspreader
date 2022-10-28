# Input Parameters
from random import randrange
import matplotlib.pyplot as plt

class State_bounds():
    IMMUNE = 1
    SUPER_HEALTHY = 2
    HEALTHY = 10
    INFECTED_ASYM = 40
    INFECTED_SYM = 70
    INFECTED_SYM_LATE = 90
    ZOMBIE = 99

class Progress_rate():
    SUPER_HEALTHY = 4
    INFECTED = 8

class Infection_rate():
    CAT = 5
    HUMAN = 2

class Blink_rate():
    VARIABLE = 5
    MAX = 10000


# Define health states

def is_immune(health):
    return health <= State_bounds.IMMUNE

def is_super_healthy(health):
    return State_bounds.SUPER_HEALTHY <= health and health < State_bounds.HEALTHY

def is_healthy(health):
    return State_bounds.HEALTHY <= health and health < State_bounds.INFECTED_ASYM

def is_infected(health): #Covers all infected states before ZOMBIE
    return State_bounds.INFECTED_ASYM <= health

def is_infected_asym(health):
    return State_bounds.INFECTED_ASYM <= health and health < State_bounds.INFECTED_SYM

def is_infected_sym(health):
    return State_bounds.INFECTED_SYM <= health and health < State_bounds.INFECTED_SYM_LATE

def is_infected_sym_late(health):
    return State_bounds.INFECTED_SYM_LATE <= health and health < State_bounds.ZOMBIE

def is_zombie(health):
    return State_bounds.ZOMBIE <= health


# Determine health count
def health_count(health,exposures_human,exposures_cat):
    if is_zombie(health):
        return State_bounds.ZOMBIE
    if is_immune(health):
        return State_bounds.IMMUNE
    health_current = health
    health += progress(health_current) + exposure(health_current,exposures_human,exposures_cat)
    if is_zombie(health):
        return State_bounds.ZOMBIE
    return health

def progress(health): #Checks health status, returns a 0 or constant value to increment h by to track disease progress
    if is_super_healthy(health):
        sum = health + Progress_rate.SUPER_HEALTHY
        remainder = sum % State_bounds.HEALTHY
        quotient = sum // State_bounds.HEALTHY
        return  Progress_rate.SUPER_HEALTHY - remainder*quotient
    if is_infected(health):
        return Progress_rate.INFECTED
    return 0

def exposure(health,exposures_human,exposures_cat):
    if is_immune(health) or is_infected(health):
        return 0
    return exposures_human*Infection_rate.HUMAN + exposures_cat*Infection_rate.CAT


"""
def main_health_test():
    health = 3

    health1 = health
    health2 = health
    health3 = health
    health4 = health

    n = 20
    H1 = []
    H2 = []
    H3 = []
    H4 = []

    for i in range(1,n):

        health1 = health_count(health1,0,0)
        H1.append(health1)

        health2 = health_count(health2,1,0)
        H2.append(health2)

        health3 = health_count(health3,0,1)
        H3.append(health3)

        health4 = health_count(health4,3,3)
        H4.append(health4)

    I = range(1,n)

    plt.plot(I,H1)
    plt.plot(I,H2)
    plt.plot(I,H3)
    plt.plot(I,H4)
    plt.show()

    print I
    print H1
    print H2
    print H3
    print H4

if __name__ == '__main__':
    main_health_test()

"""

















# Electric boogaloo
