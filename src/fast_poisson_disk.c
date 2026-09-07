/* poissondisk.c - Mike Meyer 2026

   Generate SVGs with points distributed using the Fast Poisson Disk method by Bridson
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"
#undef WRITE_SVG_IMPLEMENTATION

#define FAST_POISSON_DISK_IMPLEMENTATION
#include "fast_poisson_disk.h"
#undef FAST_POISSON_DISK_IMPLEMENTATION

enum {
    WIDTH = 240,
    HEIGHT = 180,
    MIN_DIST = 11,
    POINT_RADIUS = 2,
    MAX_POINTS = 250,
    MAX_TRIES = 30,
};
static const char* PRIMARY_COLOR = "blue"; /* interior color of the points */
static char *OUT_PATH = "./dist/poisson_disk_test.svg";


/** entry point */
int main() {
    Pt *data = NULL;
    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", OUT_PATH);
        goto error;
    }
    
    srand(9899);
    
    data = calloc(MAX_POINTS, sizeof(Pt));
    if (NULL == data) {
        fprintf(stderr, "could not allocate space for \"%d\" points\n", MAX_POINTS);
        goto error;
    }
    
    size_t num_points = 0;
    if (!fast_poisson_disk(
        data,
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
    
    begin_svg(f, WIDTH, HEIGHT);
    write_points(f, data, num_points, POINT_RADIUS, PRIMARY_COLOR);
    end_svg(f);
    fclose(f);
    free(data);

    return 0;

error:
    if (NULL != f) {
        end_svg(f);
        fclose(f);
    }
    if (NULL != data) {
        free(data);
        data = NULL;
    }
    return 1;
}