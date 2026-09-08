/* generate svgs demonstrating the algorithm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"

#define COLIOP_IMPLEMENTATION
#include "coliop.h"


static float _width    = 320.f; // width of the generated SVG. may be overridden with a command-line arg
static float _height   = 180.f; // height of the generated SVG. may be overridden with a command-line arg
static float _point_radius = 1.f; // radius of the circle we draw to indicate the points
static int _max_points = 625;   // maximum allowed number of points
static const char* _point_color = "black"; // color of the points
static char *_out_path = "white_noise.svg";

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
};

float rand01() {
    return ((float)rand())/((float)RAND_MAX);
}
        
// random x coordinate
float rand_x() {
    return rand01() * _width;
}

// random x coordinate
float rand_y() {
    return rand01() * _height;
}

// entry point
int main(int argc, const char **argv) {
    ColiopConfig coliop = {
        .descrip =
            "Generate an SVG displaying sample points with a \"white noise\" distribution.",
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

    FILE *f = fopen(_out_path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", _out_path);
        return 1;
    }
    
    srand(time(NULL));
    
    begin_svg(f, _width, _height);
    for (int i=0;i<_max_points;++i) {
        Pt p = {.x=rand_x(), .y=rand_y()};
        write_point(f, p, _point_radius, _point_color);
    }
    end_svg(f);
    fclose(f);

    return 0;
}