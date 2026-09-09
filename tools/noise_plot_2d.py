"""
2d colors of noise
"""

import math
import numpy as np
from PIL import Image

W=240
H=180
NUM_SAMPLES_PER_DIMENSION = 2*max(W,H)

def gen_white_noise():
    # white noise has flat power spectrum density (which means beta = 0)
    return gen_power_law(0, NUM_SAMPLES_PER_DIMENSION)

def gen_brownian_noise():
    # red AKA Brownian noise has spectrum density proportional to the inverse of frequency squared (which means beta = 2)
    return gen_power_law(2, NUM_SAMPLES_PER_DIMENSION)

def gen_pink_noise():
    # pink noise has spectrum density proportional to the inverse of frequency (which means beta = 1)
    return gen_power_law(1, NUM_SAMPLES_PER_DIMENSION)

def gen_blue_noise():
    # blue noise has power spectrum density proportional to frequency (which means beta = -1)
    return gen_power_law(-1, NUM_SAMPLES_PER_DIMENSION)

def gen_power_law(exponent: float, n_samples_per_dim: int):
    r"""generate noise whose power spectrum is proportional to 1 / (frequency^exponent).
    
    The variance/standard deviation are arbitrary. See the paper 
        Timmer, J. and Koenig, M.:
        On generating power law noise.
        Astron. Astrophys. 300, 707-710 (1995)
    for a better algorithm."""
    frequencies = np.fft.rfftfreq(n_samples_per_dim)
    
    # cut off frequencies below 1/n_samples_per_dim
    scaling_factors = frequencies
    f_min = 1.0 / n_samples_per_dim
    cutoff = np.sum(frequencies < f_min)
    if 0 < cutoff < len(frequencies):
        scaling_factors[:cutoff] = scaling_factors[cutoff]
    scaling_factors = scaling_factors**(-0.5 * exponent)
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

save_as_img(gen_white_noise(), 'whitenoise2d.png')
save_as_img(gen_blue_noise(), 'bluenoise2d.png')
save_as_img(gen_brownian_noise(), 'rednoise2d.png')
save_as_img(gen_pink_noise(), 'pinknoise2d.png')