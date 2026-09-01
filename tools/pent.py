from random import random
import math

a = 1
b = 0
c = 1
TRIANGLES = 8
STROKE = "#002C69"
FILL_COLOR = "#f9f9f9"
WIDTH=28
HEIGHT=16
STROKE_WIDTH = 1
SCALE=2.3

COS_TABLE = [math.cos(i * math.pi / 3) for i in range(6)]
SIN_TABLE = [math.sin(i * math.pi / 3) for i in range(6)]

with open("./dist/small_pent.svg", "w") as f:
    f.write(f"""<svg width="{WIDTH}" height="{HEIGHT}" stroke="{STROKE}" stroke-width="{STROKE_WIDTH}" fill="{FILL_COLOR}" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg"><g>""")
    x1 = 0.3 * WIDTH
    y1 = 0.57 * HEIGHT
    for i in range(TRIANGLES):
        direction = i % 6
        a, b, c = b, c, a+b
        x2 = x1 + SCALE * c * COS_TABLE[direction]
        y2 = y1 + SCALE * c * SIN_TABLE[direction]
        x3 = x1 + SCALE * c * COS_TABLE[(direction + 1) % 6]
        y3 = y1 + SCALE * c * SIN_TABLE[(direction + 1) % 6]
        f.write(f"""<path d="M {x1:.2f} {y1:.2f} L {x2:.2f} {y2:.2f} L {x3:.2f} {y3:.2f} z"/>""")
        x1, y1 = x2, y2
    f.write("</g></svg>")
