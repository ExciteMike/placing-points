import math
from random import random
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

NUM_BUCKETS = 500
SUBPLOT_MOSAIC= '''
ABCDE
FGHI.
'''

img = Image.open('./temp/Fourier.jpg')
img.load()
rgb = np.astype(np.asarray(img, dtype='int32'), float)
grayscale = np.dot(rgb[...,:3], [0.299, 0.587, 0.114])

fig = plt.figure()
axes = fig.subplot_mosaic(SUBPLOT_MOSAIC)
ax = axes['A']
ax.imshow(grayscale)
ax.set_title('original')
ax.set_axis_off()

grayscale_mean = np.mean(grayscale)
samples = grayscale - grayscale_mean
fft = np.fft.fftshift(np.fft.fft2(samples))
phases = np.angle(fft)
magnitudes = np.abs(fft)
power = magnitudes ** 2

size = power.shape
frequencies_horizontal = np.fft.fftshift(np.fft.fftfreq(size[0]))
frequencies_vertical = np.fft.fftshift(np.fft.fftfreq(size[1]))
freqsx, freqsy = np.meshgrid(frequencies_horizontal, frequencies_vertical, indexing='ij')

print(rgb.shape)
print(power.shape)
print(freqsx.shape)
print(freqsy.shape)

# draw phase
ax = axes['B']
ax.imshow(phases, interpolation='nearest', cmap='hsv')
ax.set_title('phase')
ax.set_axis_off()

# amplitudes
ax = axes['C']
ax.imshow(np.abs(fft), norm='log')
ax.set_title('amplitudes')
ax.set_axis_off()

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

# reconstructed
imag_parts = np.multiply(magnitudes, np.sin(phases))
real_parts = np.multiply(magnitudes, np.cos(phases))
fft2 = real_parts + 1J * imag_parts
image2 = np.fft.ifft2(np.fft.ifftshift(fft2))
ax = axes['D']
ax.imshow(np.real(image2))
ax.set_title('reconstructed')
ax.set_axis_off()

# imaginary part
ax = axes['E']
ax.imshow(np.imag(image2))
ax.set_title('imaginary part of reconstructed')
ax.set_axis_off()

# jittered phases
rng = np.random.default_rng()
new_phases = phases + rng.uniform(-0.25 * np.pi, 0.25 * np.pi, size=power.shape)
imag_parts = np.multiply(magnitudes, np.sin(new_phases))
real_parts = np.multiply(magnitudes, np.cos(new_phases))
fft2 = real_parts + 1J * imag_parts
image2 = np.fft.ifft2(np.fft.ifftshift(fft2))
ax = axes['H']
ax.imshow(np.real(image2))
ax.set_title('slightly messed up phases')
ax.set_axis_off()

# jittered amplitudes
new_magnitudes = 0.995 * magnitudes + 0.005 * np.max(magnitudes) * rng.uniform(size=power.shape)
imag_parts = np.multiply(new_magnitudes, np.sin(phases))
real_parts = np.multiply(new_magnitudes, np.cos(phases))
fft2 = real_parts + 1J * imag_parts
image2 = np.fft.ifft2(np.fft.ifftshift(fft2))
ax = axes['I']
ax.set_title('slightly messed up amplitudes')
ax.imshow(np.real(image2))
ax.set_axis_off()

# randomized phase
rng = np.random.default_rng()
new_phases = 2.0 * np.pi * rng.uniform(size=power.shape)
imag_parts = np.multiply(magnitudes, np.sin(new_phases))
real_parts = np.multiply(magnitudes, np.cos(new_phases))
fft2 = real_parts + 1J * imag_parts
image2 = np.fft.ifft2(np.fft.ifftshift(fft2))
ax = axes['F']
ax.imshow(np.real(image2))
ax.set_title('random phases')
ax.set_axis_off()

# randomized amplitude
new_magnitudes = np.max(magnitudes) * rng.uniform(size=power.shape)
imag_parts = np.multiply(new_magnitudes, np.sin(phases))
real_parts = np.multiply(new_magnitudes, np.cos(phases))
fft2 = real_parts + 1J * imag_parts
image2 = np.fft.ifft2(np.fft.ifftshift(fft2))
ax = axes['G']
ax.set_title('random amplitudes')
ax.imshow(np.real(image2))
ax.set_axis_off()

plt.subplots_adjust(left=0.01,bottom=0.01,right=0.99,top=0.96,wspace=0.01,hspace=0.08)
plt.show()
plt.clf()