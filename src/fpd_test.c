/* poissondisk.c - Mike Meyer 2026

   Generate SVGs with points distributed using the Fast Poisson Disk method by Bridson
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>

#define FAST_POISSON_DISK_IMPLEMENTATION
#include "fast_poisson_disk.h"
#undef FAST_POISSON_DISK_IMPLEMENTATION

enum {
    WIDTH = 1024,
    HEIGHT = 1024,
    MIN_DIST = 12,
    MAX_POINTS = 1000000,
    MAX_TRIES = 30,
    N_BUCKETS = 500,
    BAR_LEN = 40,
};
static char *POINTS_OUT_PATH = "./fpd_test_points.svg";
static char *POINT_DATA_OUT_PATH = "./fpd_test_points.dat";
static char *IMAGE_OUT_PATH = "./fpd_test_image.ppm";
static float PI = 3.1415926535897932384626433832795f;

static void dump_points(const Pt *points, size_t num_points) {
    FILE *f = fopen(POINTS_OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", POINTS_OUT_PATH);
        return;
    }
    fprintf(f, "<svg width=\"%d\"", WIDTH);
    fprintf(f, " height=\"%d\"", HEIGHT);
    fprintf(f, " fill=\"blue\" xmlns=\"http://www.w3.org/2000/svg\"> ");
    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"#f9f9f9\" />", WIDTH, HEIGHT);
    for (size_t i=0;i<num_points;++i) {
        Pt p = points[i];
        fprintf(f, "\n<circle cx=\"%f\" cy=\"%f\" r=\"5\" />", p.x, p.y);
    }
    fprintf(f, "\n</svg>");
    fclose(f);
}

/* https://en.wikipedia.org/wiki/Netpbm#File_formats */
static void dump_image(const float *samples, size_t width, size_t height) {
    FILE *f = fopen(IMAGE_OUT_PATH, "wb");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", IMAGE_OUT_PATH);
        return;
    }
    fprintf(f, "P6\n%zu %zu\n255\n", width, height);
    for (size_t row=0;row<height;++row) {
        for (size_t col=0;col<width; ++col) {
            unsigned char byte = (unsigned char)(samples[row * width + col] * 255.f);
            static unsigned char color[3];
            color[0] = byte;
            color[1] = byte;
            color[2] = byte;
            fwrite(color, 1, 3, f);
        }
    }
    fclose(f);
}

static void dump_points_for_numpy(const Pt *points, size_t num_points) {
    FILE *f = fopen(POINT_DATA_OUT_PATH, "wb");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", POINT_DATA_OUT_PATH);
        return;
    }
    for (size_t i=0;i<num_points;++i) {
        Pt p = points[i];
        fprintf(f, "%f, %f\n", p.x, p.y);
    }
    fclose(f);
}


/** entry point */
int main() {
    Pt *points = NULL;
    
    srand(9899);
    
    points = calloc(MAX_POINTS, sizeof(Pt));
    if (NULL == points) {
        fprintf(stderr, "could not allocate space for \"%d\" points\n", MAX_POINTS);
        goto error;
    }
    
    size_t num_points = 0;
    if (!fast_poisson_disk(
        points,
        MAX_POINTS,
        WIDTH,
        HEIGHT,
        MIN_DIST,
        MAX_TRIES,
        &num_points)
    ) {
        fprintf(stderr, "fast poisson disk failed\n");
        goto error;
    }
    dump_points(points, num_points);
    dump_points_for_numpy(points, num_points);
    
    /* the signal we are analyzing is 1 at each point and 0 elsewhere */
    size_t n_samples = WIDTH * HEIGHT;
    float *samples = calloc(n_samples, sizeof(float));
    if (NULL == samples) {
        goto error;
    }
    for (size_t i=0;i<num_points;++i) {
        Pt p1 = points[i];
        size_t row = (size_t)p1.y;
        size_t col = (size_t)p1.x;
        samples[row * WIDTH + col] = 1.f;
    }
    dump_image(samples, WIDTH, HEIGHT);

    /* done with point data */
    free(points);
    

    /* subtract out the mean because the spike at frequency zero wouldn't be interesting */
    float mean = (float)((double)num_points / (double)n_samples);
    for (size_t row=0;row<HEIGHT;++row) {
        for (size_t col=0;col<WIDTH;++col) {
            samples[row * WIDTH + col] -= mean;
        }
    }

    /* done with sample data */
    free(samples);

    return 0;
error:
    return 1;
}