"""
generates plots for the colors of noise section
"""

import math
import numpy as np

SOURCE_DATA_PATH = 'fpd_test_distances'
SAMPLES_OUT_PATH = 'fpd_test_distances.svg'
POWER_SPECTRUM_OUT_PATH = "fpd_test_powerspectrum.svg"
PLOT_WIDTH = 500
PLOT_HEIGHT = 500
NUM_BINS = PLOT_WIDTH // 4

def normalize(a):
    lo = np.min(a)
    hi = np.max(a)
    return (a - lo) / (hi - lo)

def samples_plot(path, samples):
    with open(path, "w") as f:
        plot_width = len(samples)
        f.write(f"""\n<svg width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" stroke="blue" stroke-width="1" fill="none" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg">""")
        f.write(f"""\n<rect x="0" y="0" width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" fill="#f9f9f9" stroke="none"/>""")
        f.write("<path d=\"")
        for index, value in enumerate(np.nditer(samples)):
            x = (index / (plot_width-1)) * plot_width
            y = (1 - value) * PLOT_HEIGHT
            if index==0:
                f.write(" M ")
            else:
                f.write(" L ")
            f.write(f"{x:.2f} {y:.2f}")
        f.write(f"\"/>")
        f.write("\n</svg>")


def power_spectrum_plot(path, samples):
    amp_spec = np.fft.fft(2.0 * samples - 1.0)
    power_spectrum = np.abs(amp_spec)**2

    # the spike at zero is not interesting
    power_spectrum = power_spectrum[1:]
    power_spectrum = normalize(power_spectrum)
    frequencies = np.fft.fftfreq(samples.size)[1:]

    with open(path, "w") as f:
        # top - the samples
        f.write(f"""\n<svg width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" stroke="blue" stroke-width="1" fill="none" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg">""")
        f.write(f"""\n<rect x="0" y="0" width="{PLOT_WIDTH}" height="{PLOT_HEIGHT}" fill="#f9f9f9" stroke="none"/>""")
        abs_freq = np.abs(frequencies)
        bin_counts = np.histogram(abs_freq, NUM_BINS)[0]
        bin_counts_safe = np.where(bin_counts==0, 1, bin_counts)
        ps_max = np.max(power_spectrum)
        bin_sums = np.histogram(abs_freq, NUM_BINS, weights=power_spectrum)[0]
        bin_means = bin_sums / bin_counts_safe
        normalized = bin_means / bin_means.max()
        n = normalized.size
        prev_x = 0
        f.write(f"\n<path d=\"M 0 {PLOT_HEIGHT}")
        for index, value in enumerate(np.nditer(normalized)):
            x = ((index+1) / n) * PLOT_WIDTH
            y = (1 - value) * PLOT_HEIGHT
            f.write(f" L {prev_x:.2f} {y:.2f}")
            f.write(f" L {x:.2f} {y:.2f}")
            prev_x = x
        f.write(f""" L {PLOT_WIDTH} {PLOT_HEIGHT} z\" fill="blue" stroke="none"/>""")
        f.write("\n</svg>")


samples = np.loadtxt(SOURCE_DATA_PATH)
samples = normalize(samples)
samples_plot(SAMPLES_OUT_PATH, samples)
power_spectrum_plot(POWER_SPECTRUM_OUT_PATH, samples)