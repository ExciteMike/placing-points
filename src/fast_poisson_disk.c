/* poissondisk.c - Mike Meyer 2026

   Generate SVGs with points distributed using the Fast Poisson Disk method by Bridson
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "raylib.h"

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"
#undef WRITE_SVG_IMPLEMENTATION

#define COLIOP_IMPLEMENTATION
#include "coliop.h"
#undef COLIOP_IMPLEMENTATION

#define FAST_POISSON_DISK_IMPLEMENTATION
#include "fast_poisson_disk.h"
#undef FAST_POISSON_DISK_IMPLEMENTATION

static float _width    = 320.f; // width of the generated SVG. may be overridden with a command-line arg
static float _height   = 180.f; // height of the generated SVG. may be overridden with a command-line arg
static float _min_dist = 10.f; // minimum allowed distance between sample points. may be overridden with a command-line arg
static float _point_radius = 1.f; // radius of the circle we draw to indicate the points
static size_t _max_points = 625; // maximum allowed number of points
static size_t _max_tries = 30; // maximum tries per point
static const char* _point_color = "black"; // interior color of the points
static char *_out_path = "poisson_disk.svg";

ColiopOption options[] = {
    (ColiopOption) { .destination = &_width,
                     .descrip = "Width of the generated SVG.",
                     .name = "width",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'W' },
    (ColiopOption) { .destination = &_height,
                     .descrip = "Height of the generated SVG.",
                     .name = "height",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'H' },
    (ColiopOption) { .destination = &_out_path,
                     .descrip = "Path to write the SVG file at.",
                     .name = "out",
                     .type = COLIOP_OPTTYPE_STRING,
                     .letter = 'o' },
    (ColiopOption) { .destination = &_min_dist,
                     .descrip = "Minimum distance between points.",
                     .name = "min-dist",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'd' },
    (ColiopOption) { .destination = &_point_radius,
                     .descrip = "Radius of the dots used to indicate points.",
                     .name = "radius",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'r' },
    (ColiopOption) { .destination = &_point_color,
                     .descrip = "Color for the vertices. Can be a color name like \"pink\" or a hex value like \"#e68b93\".",
                     .name = "color",
                     .type = COLIOP_OPTTYPE_STRING,
                     .letter = 'c' },
    (ColiopOption) { .destination = &_max_points,
                     .descrip = "The number of points to generate.",
                     .name = "num-points",
                     .type = COLIOP_OPTTYPE_INT,
                     .letter = 'n' },
    (ColiopOption) { .destination = &_max_tries,
                     .descrip = "Limit on retries before giving up on an \"active\" point.",
                     .name = "max-tries",
                     .type = COLIOP_OPTTYPE_INT,
                     .letter = 'k' },
};

// entry point
int main(int argc, const char **argv) {
    ColiopConfig coliop = {
        .descrip =
            "Generate an SVG displaying sample points with a \"blue noise\" distribution.",
        .options = &(options[0]),
        .num_options = sizeof(options) / sizeof(options[0]),
        .max_positional_args = 0
    };
    ColiopResult *args_result = coliop_execute(&coliop, argc, argv, stdout);
    if (NULL == args_result) {
        return 1;
    }

    FILE *f = fopen(_out_path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", _out_path);
        return 1;
    }
    
    srand(time(NULL));
    
    float *data = calloc(2 * _max_points, sizeof(float));
    if (NULL == data) {
        fprintf(stderr, "could not allocate space for \"%lld\" points\n", _max_points);
        return 1;
    }
    
    size_t num_points = 0;
    if (!fast_poisson_disk(
        (float*)data,
        2 * (size_t)_max_points,
        _width,
        _height,
        _min_dist,
        _max_tries,
        &num_points)
    ) {
        fprintf(stderr, "fast poisson disk failed\n");
        return 1;
    }
    
    begin_svg(f, _width, _height);
    write_points(f, data, num_points, _point_radius, _point_color);
    end_svg(f);
    fclose(f);
    free(data);

    return 0;
}