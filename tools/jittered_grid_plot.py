"""
generate an svg to illustrate a jittered approach to distributing points
"""

import math
import numpy as np

GAP = 8
PLOT_WIDTH = 240
PLOT_HEIGHT = 180
CELL_SIZE = 12
MAX_JITTER = 0.35 * CELL_SIZE
ROWS = PLOT_HEIGHT//CELL_SIZE - 1
COLUMNS = PLOT_WIDTH//CELL_SIZE - 1
LEFT = CELL_SIZE//2
RIGHT = PLOT_WIDTH - CELL_SIZE//2 + 1
TOP = CELL_SIZE//2
BOTTOM = PLOT_HEIGHT - CELL_SIZE//2 + 1
SEED = 9902

rng = np.random.default_rng(SEED)


def cartesian_product(a, b):
    a, b = np.meshgrid(a, b)
    return np.column_stack([a.ravel(), b.ravel()])


def gen_grid():
    from_xs = np.arange(LEFT, RIGHT, CELL_SIZE)
    from_ys = np.arange(TOP, BOTTOM, CELL_SIZE)
    return cartesian_product(from_xs, from_ys)


def gen_jittered(grid):
    count = len(grid)
    directions = rng.uniform(0.0, 2.0 * math.pi, size=count)
    distances = np.sqrt(rng.uniform(0.0, MAX_JITTER*MAX_JITTER, size=count))
    x_offsets = distances * np.cos(directions)
    y_offsets = distances * np.sin(directions)
    offsets = np.column_stack([x_offsets, y_offsets])
    return grid + offsets


def gen_lines():
    from_xys = gen_grid()
    to_xys = gen_jittered(from_xys)
    return np.column_stack([from_xys, to_xys])

data = gen_lines()

with open("../dist/jittered_grid_square.svg", "w") as f:
    f.write(f"""<svg width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" stroke="none" stroke-width="1" fill="none" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg">""")
    f.write(f"""<rect x="0" y="0" width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" fill="#f9f9f9" stroke="none"/>""")

    # final points
    f.write(f"""<g fill="blue">""")
    for [x1, y1, x2, y2] in data:
        f.write(f"""<circle cx="{x2:.2f}" cy="{y2:.2f}" r="2">""")
        f.write(f"""<animate attributeName="cx" values="{x1:.2f};{x2:.2f};{x2:.2f};{x2:.2f};{x2:.2f}" dur="5s" repeatCount="indefinite"/>""")
        f.write(f"""<animate attributeName="cy" values="{y1:.2f};{y2:.2f};{y2:.2f};{y2:.2f};{y2:.2f}" dur="5s" repeatCount="indefinite"/>""")
        f.write(f"""</circle>""")
    f.write(f"""</g>""")

    f.write(f"""</svg>""")
