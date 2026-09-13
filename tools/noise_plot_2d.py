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
NUM_BUCKETS = 1024

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


SUBPLOT_MOSAIC= '''
AADDD
BCDDD
'''

def radial_power_to_figures(fname, title, radial_frequencies, radial_power, num_points):
    r"""Given ndarrays of frequencies and an ndarray of their powers, 
    generate some plots showing coming up with a point distribution 
    demonstrating that radial power spectrum.
    """
    plt.clf()
    fig = plt.figure(layout='constrained')
    fig.suptitle(title)
    fig.set_size_inches((18,12))
    axes = fig.subplot_mosaic(SUBPLOT_MOSAIC)

    # radial power plot
    ax = axes['A']
    ax.plot(radial_frequencies, radial_power)
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_title('Radial Power')

    # 2d power spectrum
    frequencies1d = np.fft.fftshift(np.fft.fftfreq(IMAGE_SIZE, d=1/IMAGE_SIZE))
    freqsx, freqsy = np.meshgrid(frequencies1d, frequencies1d)
    power = np.zeros((IMAGE_SIZE, IMAGE_SIZE))
    for row in range(IMAGE_SIZE):
        for col in range(IMAGE_SIZE):
            freq = np.sqrt(freqsx[col, row]**2 + freqsy[col, row]**2)
            search_index = np.searchsorted(radial_frequencies, freq)
            idx = min(len(radial_frequencies)-1, search_index)
            power[row, col] = radial_power[idx]
    ax = axes['B']
    #contour = ax.contourf(freqsx, freqsy, power)
    ax.imshow(power, interpolation='nearest')
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_title('Power')
    ax.set_aspect('equal')
    #plt.colorbar(contour, ax=ax)

    # image
    rng = np.random.default_rng()
    phase = 2.0 * np.pi * rng.uniform(size=power.shape)
    magnitudes = np.sqrt(power)
    imag_parts = np.multiply(magnitudes, np.sin(phase))
    real_parts = np.multiply(magnitudes, np.cos(phase))
    fft = real_parts + 1J * imag_parts
    image = np.fft.ifft2(np.fft.ifftshift(fft))
    image = np.real(image)
    ax = axes['C']
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_title('Inverse FFT\n(real part, randomized phase)', wrap=True)
    #contour = ax.contourf(np.real(image))
    ax.imshow(image, interpolation='nearest')
    ax.set_aspect('equal')
    #plt.colorbar(contour, ax=ax)

    # image = np.real(image)
    # ax = axes['c']
    # contour = plt.contourf(image)
    # ax.set_title('Inverse FFT (absolute value)')
    # ax.set_aspect('equal')
    # plt.colorbar(contour)

    image_flat = image.ravel()
    idx = np.argpartition(image_flat, -num_points)[-num_points]
    threshold = image_flat[idx]
    dots = np.zeros(image.shape)
    dots[image > threshold] = 1

    ax = axes['D']
    #contour = ax.contourf(dots)
    ax.imshow(dots, interpolation='nearest')
    ax.set_title(f'{num_points} points')
    ax.set_aspect('equal')

    plt.savefig(fname)
    plt.clf()
    print('saved', fname)


points = np.loadtxt(SOURCE_DATA_PATH, delimiter=',')
#points = np.asarray([(random() * IMAGE_SIZE, random() * IMAGE_SIZE) for _ in range(2000)], dtype=float)

num_points = len(points)
rounded_points = np.floor(points).astype(int) % IMAGE_SIZE
samples = np.zeros((IMAGE_SIZE, IMAGE_SIZE), dtype=float)
for [x,y] in rounded_points[:,]:
    samples[x,y] = 1
ax = plt.subplot(221)
contour = plt.contourf(samples)
ax.set_axis_off()
ax.set_aspect('equal')

samples -= np.mean(samples)
fft = np.fft.fftshift(np.fft.fft2(samples))
power = np.abs(fft) ** 2
power_max = np.max(power)
frequencies1d = np.fft.fftshift(np.fft.fftfreq(IMAGE_SIZE, d=1/IMAGE_SIZE))
freqsx, freqsy = np.meshgrid(frequencies1d, frequencies1d)

# that gets us the power spectrum in 2d
ax = plt.subplot(222)
contour = plt.contourf(freqsx, freqsy, power)
ax.set_axis_off()
ax.set_aspect('equal')
plt.colorbar(contour, ax=ax)

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
ax = plt.subplot(223)
ax.set_axis_off()
plt.plot(radial_frequencies, radial_power[1:])
ax = plt.subplot(224)
ax.set_axis_off()
plt.subplots_adjust(left=0,bottom=0.02,top=0.98,right=1,wspace=0,hspace=0.08)
plt.savefig('fpd_input.png', bbox_inches='tight')
plt.clf()

#
# Reverse the process
#
radial_power_to_figures('fpd.png', 'Power spectrum from a Poisson disk distribution', radial_frequencies, radial_power[1:], num_points)

#
# White noise
#
white_radial_power = np.ones(len(radial_power)) * np.mean(radial_power)
radial_power_to_figures('white.png', 'White noise', radial_frequencies, white_radial_power[1:], num_points)

#
# Blue noise
#
blue_radial_power = np.linspace(np.min(radial_power), np.max(radial_power), len(radial_power))
radial_power_to_figures('blue.png', 'Blue noise (power proportional to frequency)', radial_frequencies, blue_radial_power[1:], num_points)

#
# Pink noise
#
pink_radial_power = np.linspace(np.max(radial_power), np.min(radial_power), len(radial_power))
radial_power_to_figures('pink.png', 'Pink noise (power decreases with frequency)', radial_frequencies, pink_radial_power[1:], num_points)

#
# Step blue
#
blue_step_radial_power = 6000 * np.ones(len(radial_power))
up_to = np.nonzero(radial_frequencies > 50)[0][0]
blue_step_radial_power[:up_to] = 0
radial_power_to_figures('step_blue.png', 'Step blue noise', radial_frequencies, blue_step_radial_power[1:], num_points)