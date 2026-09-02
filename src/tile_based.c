/* generate svgs to demonstrate the Mitchell's Best Candidate */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"
#undef WRITE_SVG_IMPLEMENTATION

#define FAST_POISSON_DISK_IMPLEMENTATION
#include "fast_poisson_disk.h"
#undef FAST_POISSON_DISK_IMPLEMENTATION

static const float WIDTH = 320.f;
static const float HEIGHT = 180.f;
static const float MIN_DISTANCE = 10.f;
static const size_t MAX_FPD_TRIES = 30;
static const float TILE_SIZE = 30.f;
static const size_t NUM_TILE_TYPES = 4;
static const float POINT_RADIUS = 3.f;
static const char* POINT_COLOR = "blue";
static const char *OUT_PATH = "./dist/tile_based.svg";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const char* BORDER_COLOR = "dimgray";
static const float ANIM_DURATION = 30.f;
static const size_t EXTRA_FRAMES = 30;

#define DIMENSIONS (2)
#define POINTS_PER_TILE (6)
#define FLOATS_PER_TILE (POINTS_PER_TILE * DIMENSIONS)

void init_tile(FILE *f, size_t tile_idx) {
    float tile_x = (float)tile_idx * TILE_SIZE;
    float tile_data[FLOATS_PER_TILE] = {0.f};
    size_t n_points = 0;
    fast_poisson_disk(tile_data, FLOATS_PER_TILE, TILE_SIZE-0.25f*MIN_DISTANCE, TILE_SIZE-0.25f*MIN_DISTANCE, MIN_DISTANCE, MAX_FPD_TRIES, &n_points);

    fprintf(f, "<g id=\"tile%lld\" transform=\"translate(%.2f,0)\">", tile_idx, tile_x);
    fprintf(f, "<rect x=\"0\" y=\"0\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\"/>", TILE_SIZE, TILE_SIZE, BACKGROUND_COLOR);
    for (size_t i=0;i<n_points;++i) {
        float x = tile_data[DIMENSIONS * i    ] + 0.125f * MIN_DISTANCE;
        float y = tile_data[DIMENSIONS * i + 1] + 0.125f * MIN_DISTANCE;
        fprintf(f, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\"/>", x, y, POINT_RADIUS, POINT_COLOR);
    }
    fprintf(f, "</g>");
}

void init_tiles(FILE *f) {
    for (size_t tile_idx=0;tile_idx<NUM_TILE_TYPES;++tile_idx) {
        init_tile(f, tile_idx);
    }
}

float rand01() {
    return ((float)rand())/((float)RAND_MAX);
}

/* entry point */
int main(int argc, const char **argv) {
    srand(9908);
    size_t n_rows = (size_t)(ceilf(HEIGHT / TILE_SIZE));
    size_t n_columns = (size_t)(ceilf(WIDTH / TILE_SIZE));
    size_t num_tiles_to_place = n_rows * n_columns;
    size_t cur_frame = 0;
    size_t n_frames = num_tiles_to_place - NUM_TILE_TYPES + 1 + EXTRA_FRAMES;

    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", OUT_PATH);
        return 1;
    }
    begin_svg(f, WIDTH, HEIGHT);
    fprintf(f, "<rect x=\"0\" y=\"0\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\"/>", WIDTH, HEIGHT, BACKGROUND_COLOR);
    init_tiles(f);

    for (size_t i=NUM_TILE_TYPES;i<num_tiles_to_place;++i) {
        cur_frame++;
        int row = i / n_columns;
        int column = i % n_columns;
        int tile_choice = rand() % NUM_TILE_TYPES;
        float tile_move_x = (float)(column - tile_choice) * TILE_SIZE;
        float tile_move_y = (float)row * TILE_SIZE;
        int move_start_pct = (int)(100.f * (float)(cur_frame) / (float)(n_frames));
        int move_end_pct = (int)(100.f * (float)(cur_frame+1) / (float)(n_frames));
        if (move_end_pct == move_start_pct) {
            move_end_pct++;
        }
        fprintf(f, "<style type=\"text/css\">");
        fprintf(f, "#tile%lld{animation:tile%lld %.1fs linear infinite}", i, i, ANIM_DURATION);
        fprintf(f,
                "@keyframes tile%lld{0%%{transform:translate(0,0)} %d%%{transform:translate(0,0)} %d%%{transform:translate(%.2fpx,%.2fpx)} 99%%{transform:translate(%.2fpx,%.2fpx)}}",
                i,
                move_start_pct,
                move_end_pct,
                tile_move_x,
                tile_move_y,
                tile_move_x,
                tile_move_y);
        fprintf(f, "</style>");
        fprintf(f, "<use id=\"tile%lld\" href=\"#tile%d\"/>", i, tile_choice);
    }

    for (int i=0;i<NUM_TILE_TYPES;++i) {
        fprintf(f, "<rect x=\"%.2f\" y=\"0\" width=\"%.2f\" height=\"%.2f\" fill=\"none\" stroke=\"%s\"/>", (float)i * TILE_SIZE, TILE_SIZE, TILE_SIZE, BORDER_COLOR);
    }
    end_svg(f);
    fclose(f);

    return 0;
}