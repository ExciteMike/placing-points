/* generate svgs to demonstrate the Mitchell's Best Candidate */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"

#define COLIOP_IMPLEMENTATION
#include "coliop.h"


static float _width    = 320.f; /* width of the generated SVG. may be overridden with a command-line arg */
static float _height   = 180.f; /* height of the generated SVG. may be overridden with a command-line arg */
static float _point_radius = 1.f; /* radius of the circle we draw to indicate the points */
static int _max_points = 300;   /* maximum allowed number of points */
static int _num_candidates = 10; /* How many candidate points to test at each step. */
static const char* _point_color = "black"; /* color of the points */
static char *_out_path = "mitchells_best.svg";
static const size_t DIMENSIONS = 2;

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
    (ColiopOption) { .destination = &_point_radius,
                     .descrip = "Radius of the dots used to indicate points.",
                     .name = "radius",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'r' },
    (ColiopOption) { .destination = &_point_color,
                     .descrip = "Color to draw the dots with. Can be a color name like \"pink\" or a hex value like \"#e68b93\".",
                     .name = "color",
                     .type = COLIOP_OPTTYPE_STRING,
                     .letter = 'c' },
    (ColiopOption) { .destination = &_max_points,
                     .descrip = "The number of points to generate.",
                     .name = "num-points",
                     .type = COLIOP_OPTTYPE_INT,
                     .letter = 'n' },
    (ColiopOption) { .destination = &_num_candidates,
                     .descrip = "How many candidate points to test at each step.",
                     .name = "num-candidates",
                     .type = COLIOP_OPTTYPE_INT,
                     .letter = 'N' },
};

float rand01() {
    return ((float)rand())/((float)RAND_MAX);
}
        
/* random x coordinate */
float rand_x() {
    return rand01() * _width;
}

/* random x coordinate */
float rand_y() {
    return rand01() * _height;
}

/* calculate the distance squared between two points */
float distance_sq(float x1, float y1, float x2, float y2) {
    float dx = x2-x1;
    float dy = y2-y1;
    return dx*dx + dy*dy;
}

/* finds the distance squared from the candidate point to the nearest point in the provided array */
float candidate_distance_sq(float candidate_x, float candidate_y, float *begin, float *end) {
    float lowest = FLT_MAX;
    for (float *p=begin; p<end; p+=DIMENSIONS) {
        float d = distance_sq(candidate_x, candidate_y, p[0], p[1]);
        if (d < lowest) {
            lowest = d;
        }
    }
    return lowest;
}

/* entry point */
int main(int argc, const char **argv) {
    ColiopConfig coliop = {
        .descrip =
            "Generate an SVG displaying sample points generated with the \"Mitchell's Best Candidate\" algorithm.",
        .options = &(options[0]),
        .num_options = sizeof(options) / sizeof(options[0]),
        .max_positional_args = 0
    };
    ColiopResult *args_result = coliop_execute(&coliop, argc, argv, stdout);
    if (NULL == args_result) {
        return 1;
    }
    if (!args_result->success) {
        coliop_free_results(args_result);
        return 1;
    }

    /* restrict to sane values */
    if (_num_candidates < 1)    { _num_candidates = 1; }
    if (_num_candidates > 1024) { _num_candidates = 1024; }

    srand(time(NULL));

    /* where to store generated points*/
    float *buf = calloc(DIMENSIONS * _max_points, sizeof(float));
    if (NULL == buf) {
        fprintf(stderr, "allocation failed");
        return 1;
    }
    size_t write_pos = 0;
    size_t end_write_pos = DIMENSIONS * _max_points;

    FILE *f = fopen(_out_path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", _out_path);
        return 1;
    }
    
    begin_svg(f, _width, _height);

    /* generate first point */
    if (write_pos < end_write_pos) {
        buf[write_pos++] = rand_x();
        buf[write_pos++] = rand_y();
        write_point(f, buf[0], buf[1], _point_radius, _point_color);
    }

    /* step algorithm */
    while (write_pos < end_write_pos) {
        float best_x = -1.f;
        float best_y = -1.f;
        float highest = -1.f;
        for (size_t i=0;i<_num_candidates;++i) {
            float candidate_x = rand_x();
            float candidate_y = rand_y();
            float d = candidate_distance_sq(candidate_x, candidate_y, buf, &(buf[write_pos]));
            if (d > highest) {
                highest = d;
                best_x = candidate_x;
                best_y = candidate_y;
            }
        }
        buf[write_pos++] = best_x;
        buf[write_pos++] = best_y;
        write_point(f, best_x, best_y, _point_radius, _point_color);
    }

    end_svg(f);
    fclose(f);
    free(buf);

    return 0;
}