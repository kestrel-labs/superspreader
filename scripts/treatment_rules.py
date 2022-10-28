from game_rules import *
from random import randrange

class LightColor():
    NONE = "No color"
    GREEN = "Green light"
    RED =  "Red light"

class BlinkFrequency():
    NONE = 0
    VARIABLE = 1 # rate to increase by as disease progresses
    MAX = 100  # max blink rate milliseconds



""" Treatment rules:
-

"""


def indicator_light(health):
    current_light = [LightColor.NONE, BlinkFrequency.NONE] #if healthy, super healthy, or infected asymptomatic
    if is_immune(health):
        ---
    if is_zombie(health):
        ---
    if is_infected_sym_late(health):
        current_light = [LightColor.RED, BlinkFrequency.MAX]
    #    return current_light
    if is_infected_sym(health):
        current_light = [LightColor.RED, health*BlinkFrequency.VARIABLE]
    #    return current_light
    return  current_light
