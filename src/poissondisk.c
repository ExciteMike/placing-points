/* generate svgs demonstrating the algorithm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "raylib.h"

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"

static float _width    = 320.f; // width of the generated SVG. may be overridden with a command-line arg
static float _height   = 180.f; // height of the generated SVG. may be overridden with a command-line arg
static float _min_dist = 10.f; // minimum allowed distance between sample points. may be overridden with a command-line arg
static float _point_radius = 1.f; // radius of the circle we draw to indicate the points
static size_t _max_points = 625; // maximum allowed number of points
static size_t _max_tries = 30; // maximum tries per point
static const char* _point_color = "black"; // color of the points
static char *_out_path = "poisson_disk.svg";

// unordered list of active point
typedef struct ActiveList {
    Vector2 **data;
    size_t cap;
    size_t len;
} ActiveList;
bool list_alloc(ActiveList *list, size_t capacity); // initialize a list
bool list_push(ActiveList *list, Vector2 *item); // add a point to the list
Vector2 * list_peek(ActiveList *list); // look at top item in list
bool list_pop(ActiveList *list); // remove an item
bool list_free(ActiveList *list); // clean up

// grid of points, no more than one per cell
typedef struct Grid {
    Vector2 **data;
    float cell_size;
    size_t num_rows;
    size_t num_cols;
} Grid;
bool grid_alloc(Grid *grid, float cell_size, size_t num_rows, size_t num_cols); // prepare the grid. must use grid_free after
bool grid_insert(Grid *grid, Vector2 *point); // add a point to the grid
Vector2 * grid_get(Grid *grid, size_t row, size_t col); // get a point from the grid space, if any
bool grid_free(Grid *grid); // release resources

float rand01(); // random number from 0 to 1 inclusive
int max2(int a, int b); // highest among the parameters
int min2(int a, int b); // lowest among the parameters
int clamp(int x, int lowest, int highest); // clamp x to a range
bool do_fpd(Vector2 *buf, size_t buf_size, size_t *count); // generate points. set the number generated, return true if successful
bool distance_check(Vector2 candidate, Grid *grid); // how far would the candidate be from the nearest point already in the grid?


// print usage message to stdout
void print_usage(const char* name) {
    printf("USAGE: %s [options]\n", name);
    printf("Options:\n");
    printf("  --help   \tshow this message\n");
    printf("  -w WIDTH     \twidth of the generated SVG\n");
    printf("  -h HEIGHT    \theight of the generated SVG\n");
    printf("  -d MIN_DIST  \tset a minimum distance between sample points\n");
    printf("  -r RADIUS    \tradius of the dots used to indicate points\n");
    printf("  -c COLOR     \tColor to draw the dots with. Can be a color name like \"pink\" or a hex value like \"#e68b93\".\n");
    printf("  -k MAX_TRIES \tmaximum number of attempts before deactivating a point\n");
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
        if (0 == strcmp(arg, "--help")) {
            printf("Generate an SVG displaying sample points with a \"blue noise\" distribution. that is, random, yet avoiding excessive clustering.\n");
            return false;
        } else if (0 == strcmp(arg, "-w")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"%s\"\n", arg);
                return false;
            }
            const char* next_arg = argv[i+1];
            _width = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-h")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"%s\"\n", arg);
                return false;
            }
            const char* next_arg = argv[i+1];
            _height = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-d")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"%s\"\n", arg);
                return false;
            }
            const char* next_arg = argv[i+1];
            _min_dist = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-n")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"%s\"\n", arg);
                return false;
            }
            const char* next_arg = argv[i+1];
            _max_points = (size_t)(max2(0, atoi(next_arg)));
            i += 2;
        } else if (0 == strcmp(arg, "-r")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"%s\"\n", arg);
                return false;
            }
            const char* next_arg = argv[i+1];
            _point_radius = strtof(next_arg, NULL);
            i += 2;
        } else if (0 == strcmp(arg, "-k")) {
            if (argc <= i+1) {
                printf("ERROR: expected a number after \"%s\"\n", arg);
                return false;
            }
            const char* next_arg = argv[i+1];
            _max_tries = (size_t)(max2(0, atoi(next_arg)));
            i += 2;
        } else if (0 == strcmp(arg, "-c")) {
            if (argc <= i+1) {
                printf("ERROR: expected a color after \"%s\"\n", arg);
                return false;
            }
            _point_color = argv[i+1];
            i += 2;
        } else if (0 == strcmp(arg, "-o")) {
            if (argc <= i+1) {
                printf("ERROR: expected a file path after \"%s\"\n", arg);
                return false;
            }
            _out_path = argv[i+1];
            i += 2;
        } else {
            printf("ERROR: unrecognized option \"%s\"\n", arg);
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

    FILE *f = fopen(_out_path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", _out_path);
        return 1;
    }
    
    srand(time(NULL));
    
    Vector2 *data = calloc(_max_points, sizeof(Vector2));
    if (NULL == data) {
        fprintf(stderr, "could not allocate space for \"%lld\" points\n", _max_points);
        return 1;
    }
    
    size_t num_points = 0;
    if (!do_fpd(data, (size_t)_max_points, &num_points)) {
        fprintf(stderr, "fast poisson disk failed\n");
        return 1;
    }
    
    begin_svg(f, _width, _height);
    write_points(f, (float*)data, num_points*2, _point_radius, _point_color);
    end_svg(f);
    fclose(f);
    free(data);

    return 0;
}

// generate points
bool do_fpd(Vector2 *buf, size_t buf_size, size_t *count) {
    const float TAU = 2.f * PI;
    if (buf_size < 1) {
        return false;
    }
    float cell_size = _min_dist / sqrt(2.f);
    *count = 0;
    Grid grid = {0};
    if (!grid_alloc(&grid, cell_size, (size_t)ceil(_height / cell_size), (size_t)ceil(_width / cell_size))) {
        fprintf(stderr, "grid alloc failed\n");
        return false;
    }
    
    ActiveList active_list = {0};
    if (!list_alloc(&active_list, _max_points)) {
        fprintf(stderr, "list alloc failed\n");
        grid_free(&grid);
        return false;
    }
    
    // insert initial point
    buf[(*count)++] = (Vector2) {
        .x = rand01() * _width,
        .y = rand01() * _height,
    };
    if (!grid_insert(&grid, &(buf[*count-1]))) {
        fprintf(stderr, "error in grid_insert: invalid point\n");
        return false;
    }
    if (!list_push(&active_list, &(buf[*count-1]))) {
        fprintf(stderr, "error in list_push\n");
        return false;
    }
    
    // keep placing more points until we can't
    while ((*count < buf_size) && (*count < _max_points) && (active_list.len != 0)) {
        Vector2 *current_point = list_peek(&active_list);
        size_t tries = 0;
        while (true) {
            float a = rand01() * TAU;
            float r = _min_dist + rand01() * _min_dist;
            Vector2 candidate = {
                .x = current_point->x + r * cosf(a),
                .y = current_point->y + r * sinf(a),
            };
            if ((0.f <= candidate.x) &&
                (candidate.x <= _width) && 
                (0.f <= candidate.y) &&
                (candidate.y <= _height) &&
                distance_check(candidate, &grid)
            ) {
                buf[(*count)++] = candidate;
                if (!grid_insert(&grid, &(buf[*count-1]))) {
                    fprintf(stderr, "error in grid_insert: invalid point\n");
                    return false;
                }
                if (!list_push(&active_list, &(buf[*count-1]))) {
                    fprintf(stderr, "error in list_push\n");
                    return false;
                }
                break;
            } else if (tries++ >= _max_tries) {
                list_pop(&active_list);
                break;
            }
        }
    }
    
    grid_free(&grid);
    list_free(&active_list);
    return true;
}

#undef FPD_INSERT

float rand01() {
    return ((float)rand())/((float)RAND_MAX);
}

// highest among the parameters
int max2(int a, int b) {
    return (a>=b) ? a : b;
}

// lowest among the parameters
int min2(int a, int b) {
    return (a<=b) ? a : b;
}

// clamp x to a range
int clamp(int x, int lowest, int highest) {
    return max2(lowest, min2(highest, x));
}

// how far would the candidate be from the nearest point already in the grid?
bool distance_check(Vector2 candidate, Grid *grid) {
    float min_dist_sq = _min_dist*_min_dist;
    int row = (int)floor(candidate.y / grid->cell_size);
    int col = (int)floor(candidate.x / grid->cell_size);
    size_t min_row = (size_t) max2(0, row - 1);
    size_t max_row = (size_t) min2(row + 1, grid->num_cols - 1);
    size_t min_col = (size_t) max2(0, col - 1);
    size_t max_col = (size_t) min2(col + 1, grid->num_cols - 1);
    for (size_t row=min_row;row<=max_row;++row) {
        for (size_t col=min_col;col<=max_col;++col) {
            Vector2 *test_point = grid_get(grid, row, col);
            if (NULL == test_point) {
                continue;
            }
            float dx = test_point->x - candidate.x;
            float dy = test_point->y - candidate.y;
            float d_sq = dx*dx + dy*dy;
            if (d_sq < min_dist_sq) {
                return false;
            }
        }
    }
    return true;
}

// prepare the grid. must use grid_free after
bool grid_alloc(Grid *grid, float cell_size, size_t num_rows, size_t num_cols) {
    grid->data = calloc(num_rows * num_cols, sizeof(grid->data[0]));
    if (NULL == grid->data) {
        return false;
    }
    grid->cell_size = cell_size;
    grid->num_rows = num_rows;
    grid->num_cols = num_cols;
    return true;
}

// add a point to the grid
bool grid_insert(Grid *grid, Vector2 *point) {
    int irow = (int)floor(point->y / grid->cell_size);
    if (irow < 0) {
        return false;
    }
    if (irow >= grid->num_rows) {
        return false;
    }
    size_t row = (size_t)irow;
    int icol = (int)floor(point->x / grid->cell_size);
    if (icol < 0) {
        return false;
    }
    if (icol >= grid->num_cols) {
        return false;
    }
    size_t col = (size_t)icol;
    Vector2 *found = grid->data[row * grid->num_cols + col];
    if (NULL != found) {
        return false;
    }
    grid->data[row * grid->num_cols + col] = point;
    return true;
}

// get a point from the grid space, if any
Vector2 * grid_get(Grid *grid, size_t row, size_t col) {
    if ((0 <= row) && (row < grid->num_rows) &&
        (0 <= col) && (col < grid->num_cols)
    ) {
        return grid->data[row * grid->num_cols + col];
    }
    return NULL;
}

// release resources
bool grid_free(Grid *grid) {
    free(grid->data);
    return true;
}

// initialize a list
bool list_alloc(ActiveList *list, size_t capacity) {
    list->data = calloc(capacity, sizeof(list->data[0]));
    if (NULL == list->data) {
        return false;
    }
    list->cap = capacity;
    list->len = 0;
    return true;
}

// add a point to the list
bool list_push(ActiveList *list, Vector2 *item) {
    if (list->len < list->cap) {
        list->data[list->len++] = item;
        return true;
    }
    return false;
}

// look at top item in list
Vector2 * list_peek(ActiveList *list) {
    return list->data[list->len - 1];
}

// remove an item
bool list_pop(ActiveList *list) {
    if (list->len < 1) {
        return false;
    }
    list->len -= 1;
    return true;
}

// clean up
bool list_free(ActiveList *list) {
    free(list->data);
    return true;
}