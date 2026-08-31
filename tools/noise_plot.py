"""
generates plots for the colors of noise section
"""

import math
import numpy as np

NUM_SAMPLES = 1_000_000
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
NUM_BINS = PLOT_WIDTH//4

np.random.seed(9901)

def moving_average(a, n):
    return np.convolve(a, np.ones(n), 'valid') / n

def gen_white_noise():
    # white noise has flat power spectrum density (which means beta = 0)
    return gen_power_law(0, NUM_SAMPLES)

def gen_brownian_noise():
    # red AKA Brownian noise has spectrum density proportional to the inverse of frequency squared (which means beta = 2)
    return gen_power_law(2, NUM_SAMPLES)
    #white = np.random.rand(NUM_SAMPLES) - 0.5
    #return moving_average(white, 32)

def gen_pink_noise():
    # pink noise has spectrum density proportional to the inverse of frequency (which means beta = 1)
    return gen_power_law(1, NUM_SAMPLES)

def gen_blue_noise():
    # blue noise has power spectrum density proportional to frequency (which means beta = -1)
    return gen_power_law(-1, NUM_SAMPLES)

def gen_power_law(exponent: float, n_samples: int):
    r"""generate noise whose power spectrum is proportional to 1 / (frequency^exponent).
    
    The variance/standard deviation are arbitrary. See the paper 
        Timmer, J. and Koenig, M.:
        On generating power law noise.
        Astron. Astrophys. 300, 707-710 (1995)
    for a better algorithm."""
    frequencies = np.fft.rfftfreq(n_samples)
    
    # cut off frequencies below 1/n_samples
    scaling_factors = frequencies
    f_min = 1.0 / n_samples
    cutoff = np.sum(frequencies < f_min)
    if 0 < cutoff < len(frequencies):
        scaling_factors[:cutoff] = scaling_factors[cutoff]
    scaling_factors = scaling_factors**(-0.5 * exponent)

    rng = np.random.default_rng()
    size = len(scaling_factors)
    real_parts = rng.normal(scale=scaling_factors, size=size)
    imag_parts = rng.normal(scale=scaling_factors, size=size)

    # for reasons I don't understand,
    # if the number of samples is even, the last value must be real
    if n_samples % 2 == 0:
        real_parts[-1] *= math.sqrt(2)
        imag_parts[-1] = 0

    # this part must be real for reasons I don't understand
    real_parts[0] *= math.sqrt(2)
    imag_parts[0] = 0

    frequency_domain_values = real_parts + 1J * imag_parts

    # inverse Fourier xform
    return np.fft.irfft(frequency_domain_values, n=n_samples)


def normalize(a):
    lo = np.min(a)
    hi = np.max(a)
    return (a - lo) / (hi - lo)


def make_noise_graphs(path, samples, output_range_min=0.01, output_range_max=0.99):
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
        # top - the samples
        f.write(f"""<svg width="{FULL_WIDTH}" height="{FULL_HEIGHT}" stroke="blue" stroke-width="1" fill="none" stroke-linejoin="round" xmlns="http://www.w3.org/2000/svg">""")
        f.write(f"""<rect x="{MARGIN - 1}" y="{MARGIN - 1}" width="{PLOT_WIDTH + 2}" height="{PLOT_HEIGHT + 2}" fill="#f9f9f9" stroke="none"/>""")
        f.write(f"""<rect x="{MARGIN - 1}" y="{PLOT2_TOP - 1}" width="{PLOT_WIDTH + 2}" height="{PLOT_HEIGHT + 2}" fill="#f9f9f9" stroke="none"/>""")
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

        # bottom - power spectrum
        abs_freq = np.abs(frequencies)
        bin_counts = np.histogram(abs_freq, NUM_BINS)[0]
        bin_counts_safe = np.where(bin_counts==0, 1, bin_counts)
        ps_max = np.max(power_spectrum)
        bin_sums = np.histogram(abs_freq, NUM_BINS, weights=power_spectrum)[0]
        bin_means = bin_sums / bin_counts_safe
        normalized = bin_means / bin_means.max()
        n = normalized.size
        prev_x = PLOT_LEFT
        f.write(f"<path d=\"M {PLOT_LEFT} {PLOT2_BOTTOM}")
        for index, value in enumerate(np.nditer(normalized)):
            x = PLOT_LEFT + ((index+1) / n) * PLOT_WIDTH
            mapped_value = output_range_min + value * (output_range_max - output_range_min)
            y = PLOT2_BOTTOM - mapped_value * PLOT_HEIGHT
            f.write(f" L {prev_x:.2f} {y:.2f}")
            f.write(f" L {x:.2f} {y:.2f}")
            prev_x = x
        f.write(f""" L {PLOT_RIGHT} {PLOT2_BOTTOM} z\" fill="blue" stroke="none"/></svg>""")

make_noise_graphs("../dist/colors_of_noise_white.svg", gen_white_noise(), output_range_max=0.6)
make_noise_graphs("../dist/colors_of_noise_blue.svg", gen_blue_noise())
make_noise_graphs("../dist/colors_of_noise_red.svg", gen_brownian_noise())
make_noise_graphs("../dist/colors_of_noise_pink.svg", gen_pink_noise())