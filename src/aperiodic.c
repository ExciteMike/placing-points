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

static const float WIDTH = 240.f;
static const float HEIGHT = 180.f;
static const float POINT_RADIUS = 3.f;
static const char* POINT_COLOR = "blue";
static const char *OUT_PATH = "./dist/aperiodic.svg";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const char* KITE_GRID_COLOR = "dimgray";
static const float SQRT3 = 1.7320508075688772935274463415059f;
static const float KITE_SHORT_SIDE = 10.f;
static const float KITE_LONG_SIDE = SQRT3 * KITE_SHORT_SIDE;
static const float HEX_GRID_X_STEP = 3 * KITE_SHORT_SIDE;
static const float HEX_GRID_Y_STEP = KITE_LONG_SIDE;
static const float ANIM_DURATION = 30.f;
static const size_t EXTRA_FRAMES = 50;

#define DIMENSIONS (2)
#define POINTS_PER_TILE (6)
#define FLOATS_PER_TILE (POINTS_PER_TILE * DIMENSIONS)

/* entry point */
int main(int argc, const char **argv) {
    srand(9910);
    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", OUT_PATH);
        return 1;
    }
    begin_svg(f, WIDTH, HEIGHT);
    fprintf(f, "<rect x=\"0\" y=\"0\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\"/>", WIDTH, HEIGHT, BACKGROUND_COLOR);

    /*

     O_       (0,0)
     | `-._
     |     `O._  (1.5 x SHORT, 0.5 x LONG)
     |     /   `._
     O----X       _O ({0, SHORT, 3 x SHORT}, LONG)
     |     \  _.-` |
     |     _O<     | (1.5 x SHORT, 1.5 x LONG)
     | _.-`  \     |
     O<       O----O ({0, 2 x SHORT, 3 x SHORT}, 2 x LONG)
       `-._  /     |
           `O_     | (1.5 x SHORT, 2.5 x LONG)
              `-._ |
                  `O (3 x SHORT, 3 x LONG)
     */
    fprintf(f, "<g id=\"tileable-quad\" stroke=\"%s\">", BACKGROUND_COLOR);
    fprintf(f, "<path d=\"");
    fprintf(f, "M0,0 L%.2f,%.2f ", HEX_GRID_X_STEP, HEX_GRID_Y_STEP);
    fprintf(f, "M%.2f,%.2f l0,%.2f L0,%.2f ", HEX_GRID_X_STEP, -HEX_GRID_Y_STEP, 2.f*HEX_GRID_Y_STEP, 2.f*HEX_GRID_Y_STEP);
    fprintf(f, "M%.2f,%.2f L%.2f,0 L%.2f,0 ", 0.5f*HEX_GRID_X_STEP, -0.5f*HEX_GRID_Y_STEP, 2.f*HEX_GRID_X_STEP, HEX_GRID_X_STEP);
    fprintf(f, "M%.2f,0 L%.2f,0 L%.2f,0 ", 0.5f*HEX_GRID_X_STEP, -0.5f*HEX_GRID_Y_STEP, 2.f*HEX_GRID_X_STEP, HEX_GRID_X_STEP);
    fprintf(f, "\"/>");
    fprintf(f, "</g>");
    
    /* vertical lines */
    for (size_t x=0;x<WIDTH;x+=VERTICAL_LINE_STEP) {
        KITE_GRID_COLOR   
    }


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