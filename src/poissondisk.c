/* generate svgs demonstrating the algorithm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

static float _width    = 100.f; // width of the generated SVG. may be overridden with a command-line arg
static float _height   = 100.f; // height of the generated SVG. may be overridden with a command-line arg
static float _min_dist = 4.f;   // minimum allowed distance between sample points. may be overridden with a command-line arg
static int _max_points = 625;   // maximum allowed number of points
static char *_out_path = "poisson_disk.svg";

// print usage message to stdout
void print_usage(const char* name) {
    printf("Generate an SVG displaying sample points with a \"blue noise\" distribution. that is, random, yet avoiding excessive clustering.\n");
    printf("USAGE: %s [options]\n", name);
    printf("Options:\n");
    printf("  -h, --help   \tshow this message\n");
    printf("  -w WIDTH     \twidth of the generated SVG\n");
    printf("  -h HEIGHT    \theight of the generated SVG\n");
    printf("  -d MIN_DIST  \tset a minimum distance between sample points\n");
    printf("  -o PATH      \tpath to write the SVG file at\n");
    printf("  -n MAX_POINTS\tSet a maximum number of points to generate\n");
    printf("               \tThere may be fewer based on what the other settings allow\n");
    printf("               \tand the stochastic nature of the algorithm\n");
}

// process command line args. return true if successful.
bool read_args(int argc, char **argv) {
    int i = 1;
    while (i < argc) {
        const char *arg = argv[i];
        if (0 == strcmp(arg, "-h")) {
            return false;
        } else if (0 == strcmp(arg, "--help")) {
            return false;
        } else if (0 == strcmp(arg, "-w")) {
            if (argc <= i+1) {
                return false;
            }
            const char* next_arg = argv[i+1];
            _width = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-h")) {
            if (argc <= i+1) {
                return false;
            }
            const char* next_arg = argv[i+1];
            _height = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-d")) {
            if (argc <= i+1) {
                return false;
            }
            const char* next_arg = argv[i+1];
            _min_dist = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-n")) {
            if (argc <= i+1) {
                return false;
            }
            const char* next_arg = argv[i+1];
            _max_points = atoi(next_arg);
            i += 2;
        } else if (0 == strcmp(arg, "-o")) {
            if (argc <= i+1) {
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

// entry point
int main(int argc, char **argv) {
    if (!read_args(argc, argv)) {
        print_usage(argv[0]);
        return 1;
    }

    // TODO: 

    return 0;
}