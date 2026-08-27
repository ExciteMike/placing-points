/* generate svgs demonstrating the algorithm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"


static float _width    = 100.f; // width of the generated SVG. may be overridden with a command-line arg
static float _height   = 100.f; // height of the generated SVG. may be overridden with a command-line arg
static float _point_radius = 1.f; // radius of the circle we draw to indicate the points
static int _max_points = 625;   // maximum allowed number of points
static const char* _point_color = "black"; // color of the points
static char *_out_path = "white_noise.svg";


// print usage message to stdout
void print_usage(const char* name) {
    printf("USAGE: %s [options]\n", name);
    printf("Options:\n");
    printf("  --help   \tshow this message\n");
    printf("  -w WIDTH     \twidth of the generated SVG\n");
    printf("  -h HEIGHT    \theight of the generated SVG\n");
    printf("  -o PATH      \tpath to write the SVG file at\n");
    printf("  -r RADIUS    \tradius of the dots used to indicate points\n");
    printf("  -c COLOR     \tColor to draw the dots with. Can be a color name like \"pink\" or a hex value like \"#e68b93\".\n");
    printf("  -n NUM_POINTS\tThe number of points to generate\n");
    printf("               \tThere may be fewer based on what the other settings allow\n");
    printf("               \tand the stochastic nature of the algorithm\n");
}

// process command line args. return true if successful.
bool read_args(int argc, char **argv) {
    int i = 1;
    while (i < argc) {
        const char *arg = argv[i];
        if (0 == strcmp(arg, "--help")) {
            printf("Generate an SVG displaying sample points with a \"white noise\" distribution.\n");
            return false;
        } else if (0 == strcmp(arg, "-w")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"-w\"\n");
                return false;
            }
            const char* next_arg = argv[i+1];
            _width = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-h")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"-h\"\n");
                return false;
            }
            const char* next_arg = argv[i+1];
            _height = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-n")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"-n\"\n");
                return false;
            }
            const char* next_arg = argv[i+1];
            _max_points = atoi(next_arg);
            i += 2;
        } else if (0 == strcmp(arg, "-r")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"-r\"\n");
                return false;
            }
            const char* next_arg = argv[i+1];
            _point_radius = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-c")) {
            if (argc <= i+1) {
                printf("ERROR: expected a color after \"-c\"\n");
                return false;
            }
            _point_color = argv[i+1];
            i += 2;
        } else if (0 == strcmp(arg, "-o")) {
            if (argc <= i+1) {
                printf("ERROR: expected a file path after \"-o\"\n");
                return false;
            }
            _out_path = argv[i+1];
            i += 2;
        } else {
            return false;
        }
    }
    return true;
}

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
int main(int argc, char **argv) {
    if (!read_args(argc, argv)) {
        print_usage(argv[0]);
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
        Vector2 p = {.x=rand_x(), .y=rand_y()};
        write_point(f, p, _point_radius, _point_color);
    }
    end_svg(f);
    fclose(f);

    return 0;
}