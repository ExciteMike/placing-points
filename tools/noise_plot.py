"""
generates plots for the colors of noise section
"""

from random import random
import math
import numpy as np

NUM_SAMPLES = 2000000
HIGH_PASS_WINDOW = 3
MARGIN = 0
GAP = 8
PLOT_WIDTH = 240 - 2 * MARGIN
PLOT_HEIGHT = 180
FULL_WIDTH = 2 * MARGIN + PLOT_WIDTH
FULL_HEIGHT = 2 * PLOT_HEIGHT + 2 * MARGIN + GAP
PLOT_LEFT = MARGIN
PLOT_RIGHT = FULL_WIDTH - MARGIN
PLOT2_TOP = PLOT_HEIGHT + MARGIN + GAP
PLOT2_BOTTOM = FULL_HEIGHT - MARGIN
SAMPLE_PLOT_LIMIT = PLOT_WIDTH
NUM_BINS = int(PLOT_WIDTH / 4)

def moving_average(a, n):
    return np.convolve(a, np.ones(n), 'valid') / n

def gen_white_noise():
    return np.random.rand(NUM_SAMPLES)

def gen_brownian_noise():
    white = np.random.rand(NUM_SAMPLES) - 0.5
    return moving_average(white, 32)

def gen_blue_noise():
    white = np.random.randn(NUM_SAMPLES)
    smoothed = moving_average(white, HIGH_PASS_WINDOW)
    size_diff = white.size - smoothed.size
    if size_diff%2 != 0:
        print(f"HIGH_PASS_WINDOW should be odd. was {HIGH_PASS_WINDOW}")
        exit(1)
    offset = int(size_diff / 2)
    without_ends = white[offset:-offset]
    return without_ends - smoothed

def normalize(a):
    lo = np.min(a)
    hi = np.max(a)
    return (a - lo) / (hi - lo)

def make_noise_graphs(path, samples):
    samples = normalize(samples)

    amp_spec = np.fft.fft(2.0 * samples - 1.0)
    power_spectrum = np.abs(amp_spec)**2

    # the spike at zero is not interesting
    power_spectrum = power_spectrum[1:]
    power_spectrum = normalize(power_spectrum)
    frequencies = np.fft.fftfreq(samples.size)[1:]

    # fft stuff above doesn't put them in the most reasonable order for plotting
    #indices = np.argsort(np.abs(frequencies))
    #frequencies = frequencies[indices]
    #power_spectrum = power_spectrum[indices]

    with open(path, "w") as f:
        f.write(f"""<svg width="{FULL_WIDTH}" height="{FULL_HEIGHT}" stroke="blue" stroke-width="1" fill="none" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg">""")
        f.write(f"""<rect x="{MARGIN - 1}" y="{MARGIN - 1}" width="{PLOT_WIDTH + 2}" height="{PLOT_HEIGHT + 2}" fill="white" stroke="none"/>""")
        f.write(f"""<rect x="{MARGIN - 1}" y="{PLOT2_TOP - 1}" width="{PLOT_WIDTH + 2}" height="{PLOT_HEIGHT + 2}" fill="white" stroke="none"/>""")
        f.write("<path d=\"")
        truncated_samples = normalize(samples[:SAMPLE_PLOT_LIMIT])
        for index, value in enumerate(np.nditer(truncated_samples)):
            x = MARGIN + (index / (SAMPLE_PLOT_LIMIT-1)) * PLOT_WIDTH
            y = MARGIN + (1 - value) * PLOT_HEIGHT
            if index==0:
                f.write(" M ")
            else:
                f.write(" L ")
            f.write(f"{x:.2f} {y:.2f}")
        f.write(f"\"/>")

        abs_freq = np.abs(frequencies)
        log_freq = np.power(math.e, abs_freq)
        bin_counts = np.histogram(log_freq, NUM_BINS)[0]
        bin_counts_safe = np.where(bin_counts==0, 1, bin_counts)
        power_spectrum_log = normalize(np.log(power_spectrum + 1))
        bin_sums = np.histogram(log_freq, NUM_BINS, weights=power_spectrum_log)[0]
        bin_means = bin_sums / bin_counts_safe
        normalized = bin_means / bin_means.max()
        n = normalized.size
        prev_x = PLOT_LEFT
        f.write(f"<path d=\"M {PLOT_LEFT} {PLOT2_BOTTOM}")
        for index, value in enumerate(np.nditer(normalized)):
            x = PLOT_LEFT + ((index+1) / n) * PLOT_WIDTH
            y = PLOT2_BOTTOM - value * PLOT_HEIGHT
            f.write(f" L {prev_x:.2f} {y:.2f}")
            f.write(f" L {x:.2f} {y:.2f}")
            prev_x = x
        f.write(f""" L {PLOT_RIGHT} {PLOT2_BOTTOM} z\" fill="blue" stroke="none"/></svg>""")

make_noise_graphs("../dist/white_noise.svg", gen_white_noise())

# blue noise - ideally, has power spectrum density proportional to frequency
make_noise_graphs("../dist/blue_noise.svg", gen_blue_noise())

# brownian noise aka red noise - power spectrum density proportional to one over the frequency squared
make_noise_graphs("../dist/red_noise.svg", gen_brownian_noise())