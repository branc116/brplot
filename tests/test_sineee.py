import math

curx = 0
print("--zoomx 5")
print("--zoomy 20")
for i in range(100000000):
    curx += 0.01
    cury = math.sin(curx**0.4)
    print(f"{curx},{cury}")
