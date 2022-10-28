from game_rules_2 import *
from light_rules import *
from random import randrange
import matplotlib.pyplot as plt


def main_health_test():

    health = HealthState(3,0) #HealthState(initial health count, 0 - not resistance to cats)
    exposure = Exposure(0,0) #initial exposures (human, cat)
    a = 0
    b = 3

    H_health = []
    H_LED = []
    H_exposure = []

    n = 20

    for i in range(1,n):
        exposure = Exposure(random.randint(a, b),random.randint(a, b)) #human, cat
        health = health_update(health,exposure) # current health count and cat resistance (1 - resistanct),
        led_state = indicator_light(health)
        H_health.append(health)
        H_LED.append(health)


    I = range(1,n)

    plt.plot(I,H)
    plt.show()

    print I
    print H


if __name__ == '__main__':
    main_health_test()
