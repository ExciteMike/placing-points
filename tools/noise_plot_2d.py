"""
2d colors of noise
"""
import math
from random import random
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

SOURCE_DATA_PATH = 'fpd_test_points.dat'
POWER_SPECTRUM_OUT_PATH = "fpd_test_powerspectrum.svg"
IMAGE_SIZE = 1024
PLOT_WIDTH = 500
PLOT_HEIGHT = 500
NUM_BUCKETS = 256

def gen_white_noise():
    # white noise has flat power spectrum density (which means beta = 0)
    return gen_power_law(0, IMAGE_SIZE)

def gen_brownian_noise():
    # red AKA Brownian noise has spectrum density proportional to the inverse of frequency squared (which means beta = 2)
    return gen_power_law(2, IMAGE_SIZE)

def gen_pink_noise():
    # pink noise has spectrum density proportional to the inverse of frequency (which means beta = 1)
    return gen_power_law(1, IMAGE_SIZE)

def gen_blue_noise():
    # blue noise has power spectrum density proportional to frequency (which means beta = -1)
    return gen_power_law(-1, IMAGE_SIZE)

def gen_violet_noise():
    # blue noise has power spectrum density proportional to frequency squared (which means beta = -2)
    return gen_power_law(-2, IMAGE_SIZE)


def mine(x):
    #return np.piecewise(x,
    #    [x<=0.25,(x>0.25) & (x<=0.4167)],
    #    [0,1,0.5])
    a = 2.0 / 12.0
    b = 3.0 / 12.0
    c = 4.0 / 12.0
    d = 5.0 / 12.0
    return np.piecewise(
        x,
        [ x<=a,
         (x>a) & (x<=b),
         (x>b) & (x<=c),
         (x>c) & (x<=d)],
        [0, lambda x: 12*x - 2, 1, lambda x: 3 - 6*x, 0.5]
        )


def gen_mine():
    return gen_f(mine, IMAGE_SIZE)


def gen_power_law(exponent: float, n_samples_per_dim: int):
    r"""generate noise whose power spectrum is proportional to 1 / (frequency^exponent).
    
    The variance/standard deviation are arbitrary. See the paper 
        Timmer, J. and Koenig, M.:
        On generating power law noise.
        Astron. Astrophys. 300, 707-710 (1995)
    for a better algorithm."""
    f = lambda x: x**(-0.5 * exponent)
    return gen_f(f, IMAGE_SIZE)


def gen_f(f, n_samples_per_dim: int):
    frequencies = np.fft.rfftfreq(n_samples_per_dim)
    
    # cut off frequencies below 1/n_samples_per_dim
    scaling_factors = frequencies
    f_min = 1.0 / n_samples_per_dim
    cutoff = np.sum(frequencies < f_min)
    if 0 < cutoff < len(frequencies):
        scaling_factors[:cutoff] = scaling_factors[cutoff]
    scaling_factors = f(scaling_factors)
    scaling_factors = scaling_factors.reshape((-1,1)) * scaling_factors.reshape((1,-1))

    rng = np.random.default_rng()
    size = scaling_factors.shape
    real_parts = rng.normal(scale=scaling_factors, size=size)
    imag_parts = rng.normal(scale=scaling_factors, size=size)

    # for reasons I don't understand,
    # if the number of samples is even, the last value must be real
    if n_samples_per_dim % 2 == 0:
        real_parts[-1] *= math.sqrt(2)
        imag_parts[-1] = 0

    # this part must be real for reasons I don't understand
    real_parts[0] *= math.sqrt(2)
    imag_parts[0] = 0

    frequency_domain_values = real_parts + 1J * imag_parts

    # inverse Fourier xform
    return np.fft.irfft2(frequency_domain_values)


def normalize(a):
    lo = np.min(a)
    hi = np.max(a)
    return (a - lo) / (hi - lo)


def save_as_img(samples, fname):
    samples = normalize(samples)
    im = Image.new('L', (W,H))
    pixels = im.load()
    for y in range(H):
        for x in range(W):
            pixels[x, y] = int(samples[x,y] * 255)
    im.save(fname)


def normalize(a):
    lo = np.min(a)
    hi = np.max(a)
    return (a - lo) / (hi - lo)

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


points = np.loadtxt(SOURCE_DATA_PATH, delimiter=',')
#points = np.asarray([(random() * IMAGE_SIZE, random() * IMAGE_SIZE) for _ in range(2000)], dtype=float)

fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2,2)
ax1.scatter(points[:,0], points[:,1], s=4)

rounded_points = np.floor(points).astype(int) % IMAGE_SIZE
samples = np.zeros((IMAGE_SIZE, IMAGE_SIZE), dtype=float)
for [x,y] in rounded_points[:,]:
    samples[x,y] = 1
samples -= np.mean(samples)
fft = np.fft.fftshift(np.fft.fft2(samples))
power = np.abs(fft) ** 2
frequencies1d = np.fft.fftshift(np.fft.fftfreq(IMAGE_SIZE, d=1/IMAGE_SIZE))
freqsx, freqsy = np.meshgrid(frequencies1d, frequencies1d)

# that gets us the power spectrum in 2d
contour = ax2.contourf(freqsx, freqsy, power)
ax2.axis('scaled')
plt.colorbar(contour, ax=ax2)

# from that we can make a histogram based on distances
distances = np.sqrt(freqsx**2 + freqsy**2)
buckets = np.linspace(0, np.max(distances) + 1, NUM_BUCKETS)
bucket_indices = np.digitize(np.ravel(distances), buckets) # for each pixel, the radial-frequency bucket it goes into
power_1d = power.ravel()
radial_power = np.zeros(NUM_BUCKETS)
for i in range(NUM_BUCKETS):
    values_in_bucket = power_1d[bucket_indices == i]
    if values_in_bucket.size > 0:
        radial_power[i] = np.mean(values_in_bucket)
    else:
        radial_power[i] = 0

radial_frequencies = 0.5 * (buckets[:-1] + buckets[1:]) # center of each bucket

ax3.plot(radial_frequencies, radial_power[1:])
plt.show()
