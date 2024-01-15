import math
import random

curx = 0
for i in range(10000000):
    curx += random.uniform(0.001, 10)
    cury = random.uniform(0.001, 100) + 200 * math.sin(i / 2000)
    cury += 400 * math.sin(i / 20000)
    cury += 800 * math.sin(i / 80000)
    print(f"{curx},{cury}")
