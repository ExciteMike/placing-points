"""
generate an svg to illustrate a jittered approach to distributing points
"""

import math
import numpy as np

GAP = 8
PLOT_WIDTH = 240
PLOT_HEIGHT = 180
CELL_RADIUS = 8
MAX_JITTER = 0.35 * CELL_RADIUS
CELL_WIDTH = 2 * CELL_RADIUS
CELL_YSTEP = CELL_RADIUS * 0.5 * math.sqrt(3)
LEFT = CELL_RADIUS
RIGHT = PLOT_WIDTH - CELL_RADIUS + 1
TOP = CELL_YSTEP
BOTTOM = PLOT_HEIGHT - CELL_YSTEP + 1
SEED = 9903

rng = np.random.default_rng(SEED)


def cartesian_product(a, b):
    a, b = np.meshgrid(a, b)
    return np.column_stack([a.ravel(), b.ravel()])


def gen_grid():
    from_xs1 = np.arange(LEFT, RIGHT, CELL_WIDTH)
    from_ys1 = np.arange(TOP, BOTTOM, 2*CELL_YSTEP)
    from_xs2 = np.arange(LEFT + CELL_RADIUS, RIGHT, CELL_WIDTH)
    from_ys2 = np.arange(TOP + CELL_YSTEP, BOTTOM, 2*CELL_YSTEP)
    return np.concatenate([
            cartesian_product(from_xs1, from_ys1),
            cartesian_product(from_xs2, from_ys2)
        ])


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

with open("../dist/jittered_grid_staggered.svg", "w") as f:
    f.write(f"""<svg width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" stroke="none" stroke-width="1" fill="none" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg">""")
    f.write(f"""<rect x="0" y="0" width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" fill="#f9f9f9" stroke="none"/>""")

    # final points
    f.write(f"""<g fill="blue">""")
    for [x1, y1, x2, y2] in data:
        f.write(f"""<circle cx="{x2:.2f}" cy="{y2:.2f}" r="2">""")
        cx_values = f"{x1:.2f}" + 9 * f";{x2:.2f}"
        cy_values = f"{y1:.2f}" + 9 * f";{y2:.2f}"
        f.write(f"""<animate attributeName="cx" values="{cx_values}" dur="5s" repeatCount="indefinite"/>""")
        f.write(f"""<animate attributeName="cy" values="{cy_values}" dur="5s" repeatCount="indefinite"/>""")
        f.write(f"""</circle>""")
    f.write(f"""</g>""")

    f.write(f"""</svg>""")
