import brplot
import random

for set in range(10):
  value = 1.1
  time = 0
  dt = 0.1
  for i in range(100000):
    value = value + value * random.uniform(-0.08, 0.0801) * dt
    time += dt
    brplot.plot(time, value, set)

