from game_rules import *
from random import randrange

"""
Assuming I can import these functions using *

Immune: Green light
Super Healthy: No light
Healthy: No light
Infected (asymptomatic): No light
Infected (symptomatic): Red light blinking, increases with time until a max rate
Infectious (symptomatic, late stage): Red light blinking at max rate

"""

class LedStatus():
    def __init__(self, color, rate):
        self.color = color
        self.rate = rate

class LightColor():
    NONE = "No color"
    GREEN = "Green light"
    RED =  "Red light"

class BlinkFrequency():
    NONE = 0
    VARIABLE = 1 # rate to increase by as disease progresses
    MAX = 100  # max blink rate milliseconds


#take in health count, output color and blink rate

def indicator_light(health):
    if is_immune(health):
        return LedStatus(LightColor.GREEN,BlinkFrequency.NONE)
    if is_zombie(health):
        return LedStatus(LightColor.RED,BlinkFrequency.NONE)
    if is_infected_sym_late(health):
        return LedStatus(LightColor.RED,BlinkFrequency.MAX)
    if is_infected_sym(health):
        return LedStatus(LightColor.RED,health*BlinkFrequency.VARIABLE)
    return LedStatus(LightColor.NONE,BlinkFrequency.NONE) #if healthy, super healthy, or infected asymptomatic







    #booogalooo
