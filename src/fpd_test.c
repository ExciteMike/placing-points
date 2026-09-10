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
    MIN_DIST = 80,
    MAX_POINTS = 1000000,
    MAX_TRIES = 30,
    N_BUCKETS = 500,
    BAR_LEN = 40
};
static char *POINTS_OUT_PATH = "./fpd_test_points.svg";
static char *DISTANCES_OUT_PATH = "./fpd_test_distances";
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
    
    /* calculate all distances, so we can make a histogram based on it */
    size_t n_distances = (num_points * (num_points-1)) / 2;
    float *distances = calloc(n_distances, sizeof(float));
    if (NULL == distances) {
        goto error;
    }
    float *write_head = distances;
    float highest = 0.f;
    for (size_t i=0;i<num_points;++i) {
        Pt p1 = points[i];
        for (size_t j=i+1;j<num_points;++j) {
            Pt p2 = points[j];
            float dx = p2.x-p1.x;
            float dy = p2.y-p1.y;
            float d = sqrtf(dx*dx + dy*dy);
            *(write_head++) = d;
            if (d > highest) {
                highest = d;
            }
        }
    }
    
    dump_points(points, num_points);

    /* done with point data */
    free(points);

    float bucket_width = highest / (float)N_BUCKETS;
    float *bucket_vals = calloc(N_BUCKETS, sizeof(float));
    if (NULL == bucket_vals) {
        goto error;
    }
    float max_bucket = 0.f;
    for (size_t i=0;i<n_distances;++i) {
        float d = distances[i];
        size_t bucket = (size_t)(d / bucket_width);
        float r1 = (float)bucket * bucket_width;
        float r2 = r1 + bucket_width;
        float area = PI* (r2*r2 - r1*r1);
        bucket_vals[bucket] += 1.f / area;
        if (bucket_vals[bucket] > max_bucket) {
            max_bucket = bucket_vals[bucket];
        }
    }
    /* done with distance data */
    free(distances);

    #ifdef DEBUG_PRINT_BUCKETS
    printf("\n");
    for (size_t i=0;i<N_BUCKETS;++i) {
        size_t bar_len = (size_t)((float)BAR_LEN * bucket_vals[i] / max_bucket);
        printf("%6.1f - %6.1f|", (float)i * bucket_width, (float)(i+1) * bucket_width);
        for (size_t j=0;j<bar_len;++j) {
            printf("*");
        }
        printf("\n");
    }
    printf("\n");
    #endif // DEBUG_PRINT_BUCKETS

    FILE *f = fopen(DISTANCES_OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", DISTANCES_OUT_PATH);
        goto error;
    }
    for (size_t i=0;i<N_BUCKETS;++i) {
        fprintf(f, "%f\n", bucket_vals[i]);
    }
    fclose(f);
    free(bucket_vals);

    return 0;
error:
    return 1;
}