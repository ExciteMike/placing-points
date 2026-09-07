/* poissondisk.c - Mike Meyer 2026

   Generate SVGs to demonstrate Fast Poisson Disk 
 */

 #include <math.h>
 #include <stddef.h>
 #include <stdlib.h>
 #include <stdio.h>

#define SQRT_ONE_HALF (0.707106781187)
#define CELL_SIZE ((float)MIN_DIST * SQRT_ONE_HALF)

enum {
    WIDTH = 240,
    HEIGHT = 180,
    MIN_DIST = 30,
    MAX_DIST = 2*MIN_DIST,
    MIN_DIST_SQ = MIN_DIST*MIN_DIST,
    MAX_DIST_SQ = MAX_DIST*MAX_DIST,
    POINT_RADIUS = 7,
    MAX_POINTS = 250,
    MAX_TRIES = 30,
    MAX_COLS = 7,
    MAX_ROWS = 6,
    N_ANIM_FRAMES = 2,
    PATH_BUF_LEN = 32,
    SEED = 20260907,
    ANIMATION_LENGTH = 5,
    MAX_CANDIDATES = MAX_POINTS * MAX_TRIES, /* a lot smaller in practice if I want to optimize */
};
static const char* POINT_COLOR = "blue";
static const char* ANNULUS_COLOR = "lightgray";
static const char* CANDIDATE_COLOR = "gray";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const char* GRID_SQUARE_COLOR = "silver";
static char *OUT_PATH = "./dist/fpd_process.svg";


typedef struct Pt {
    float x;
    float y;
} Pt;


/**
 * row of the grid. Each cell can be NULL to indicate no point is yet there,
 * or can point to the Pt in that grid space
 */
typedef struct GridRow {
    Pt *cells[MAX_COLS];
} GridRow;


/**
 * Breaks the space into grid cells, each just small enough that no two 
 * points would be allowed in the same one.
 */
typedef struct Grid {
    GridRow rows[MAX_ROWS];
} Grid;


/** for each cell in a row, which frame it was filled on, or N_ANIM_FRAMES+1 if never */
typedef struct RowFillFrames {
    size_t cells[MAX_COLS];
} RowFillFrames;


/** pair of start and end times */
typedef struct StartEnd {
    size_t start;
    size_t end;
} StartEnd;


typedef struct Fpd {
    /** all generated points */
    Pt pts[MAX_POINTS];
    size_t n_pts;
    /** points from `pts` separated into grid spaces */
    Grid grid;
    /** list of active points. buffer usually can be much smaller if I want to optimize for that */
    Pt *active[MAX_POINTS];
    size_t n_active;
} Fpd;


/** data from which to generate the SVG */
typedef struct Script {
    RowFillFrames grid[MAX_ROWS];
    StartEnd active[MAX_POINTS];
    StartEnd annuli[MAX_POINTS];
    size_t n_annuli;
    StartEnd candidates[MAX_CANDIDATES];
    size_t n_candidates;
    size_t point_times[MAX_POINTS];
    size_t n_points;
    size_t n_frames;
} Script;


/** verify I picked sane numbers before continuing */
static int config_check() {
    int max_cols = (int)ceilf((float)WIDTH / CELL_SIZE);
    // cppcheck-suppress knownConditionTrueFalse
    if (max_cols > MAX_COLS) {
        fprintf(stderr, "You need more columns for this config. (%d > %d)\n", max_cols, MAX_COLS);
        return 0;
    }
    int max_rows = (int)ceilf((float)HEIGHT / CELL_SIZE);
    // cppcheck-suppress knownConditionTrueFalse
    if (max_rows > MAX_ROWS) {
        fprintf(stderr, "You need more rows for this config. (%d > %d)\n", max_rows, MAX_ROWS);
        return 0;
    }
    return 1;
}


/** random float from the given range */
static float randrange(const float min, const float max) {
    return min + (max - min) * ((float)rand())/((float)RAND_MAX);
}


/** choose a starting point */
static Pt rand_pt() {
    return (Pt) { randrange(0, WIDTH), randrange(0, HEIGHT) };
}


/* Given a point `p`, select a random point in an annulus
   centered on `p`, with an inner radius equal to the 
   minimum radius and an outer radius twice the minimum radius. */
static Pt random_from_annulus(const Pt p) {
    float TAU = 2.f * 3.14159265358979323846;
    float a = randrange(0, TAU);
    float r_sq = randrange((float)MIN_DIST_SQ, 4.f * (float)MAX_DIST_SQ);
    float r = sqrtf(r_sq);
    return (Pt) {
        p.x + r * cosf(a),
        p.y + r * sinf(a)
    };
}


/** round `n` over `d` to an integer percentage */
static int calc_pct(int n, int d) {
    return (int)(0.5f + 100.f * (float)n / (float)d);
}


/** write out the SVG for a permanent point */
static void write_permanent_point(
    FILE *f,
    Pt p,
    int first_frame,
    int num_frames,
    float total_anim_duration
) {
    static size_t id = 0;
    if (NULL == f) { return; }
    fprintf(f, "\n<style type=\"text/css\">");
    fprintf(f, "\n#p%02zu {animation:p%02zu %.2fs ease-out infinite}", id, id, total_anim_duration);
    fprintf(f, "\n@keyframes p%02zu {", id);
    fprintf(f, "\n  from{r:0px}");
    int pct1 = calc_pct(first_frame, num_frames);
    int pct2 = calc_pct(first_frame+1, num_frames);
    fprintf(f, "\n  %d%%{r:0}", pct1);
    fprintf(f, "\n  %d%%{r:%dpx}", pct2, POINT_RADIUS);
    fprintf(f, "\n  to{r:%dpx}", POINT_RADIUS);
    fprintf(f, "\n}");
    fprintf(f, "\n</style>");
    fprintf(f, "\n<circle id=\"p%02zu\"", id);
    fprintf(f,          " cx=\"%.2f\"", p.x);
    fprintf(f,          " cy=\"%.2f\"", p.y);
    fprintf(f,          " r=\"%d\"", POINT_RADIUS);
    fprintf(f,          " fill=\"%s\" />", POINT_COLOR);
    ++id;
}


/** write out the grid squares, to be animated in the CSS generated by write_grid_square_anim */
static void write_grid_squares(FILE *f) {
    if (NULL == f) { return; }
    size_t max_rows = (size_t)ceilf((float)HEIGHT/ CELL_SIZE);
    size_t max_cols = (size_t)ceilf((float)WIDTH / CELL_SIZE);
    fprintf(f, "<g fill=\"%s\" >", GRID_SQUARE_COLOR);
    for (size_t row=0;row<max_rows;++row) {
        for (size_t col=0;col<max_cols;++col) {
            fprintf(f, "\n<rect id=\"r%02zuc%02zu\"", row, col);
            fprintf(f,        " x=\"%.2f\"", col * CELL_SIZE);
            fprintf(f,        " y=\"%.2f\"", row * CELL_SIZE);
            fprintf(f,        " width=\"%.2f\"", CELL_SIZE);
            fprintf(f,        " height=\"%.2f\" />", CELL_SIZE);
        }
    }
    fprintf(f, "</g>");
}

/** write the css to animate a grid square written by write_grid_squares */
static void write_grid_square_anim(
    FILE *f,
    Pt p,
    int frame,
    int num_frames,
    float total_anim_duration
) {
    if (NULL == f) { return; }
    size_t row = (size_t)floor(p.y / CELL_SIZE);
    size_t col = (size_t)floor(p.x / CELL_SIZE);
    enum {NAME_BUF_LEN = 16};
    char name[NAME_BUF_LEN] = {'\0'};
    snprintf(name, NAME_BUF_LEN, "r%02zuc%02zu", row, col);
    fprintf(f, "\n<style type=\"text/css\">");
    fprintf(f, "\n#%s {animation:%s %.2fs ease-out infinite}", name, name, total_anim_duration);
    fprintf(f, "\n@keyframes %s {", name);
    fprintf(f, "\n  from{opacity:1}");
    int pct1 = calc_pct(frame, num_frames);
    int pct2 = calc_pct(frame+1, num_frames);
    fprintf(f, "\n  %d%%{opacity:1}", pct1);
    fprintf(f, "\n  %d%%{opacity:0}", pct2);
    fprintf(f, "\n  to{opacity:0}");
    fprintf(f, "\n}");
    fprintf(f, "\n</style>");
}


/** write out the SVG for a permanent point */
static void write_annulus(
    FILE *f,
    Pt p,
    int first_frame,
    int num_frames,
    float total_anim_duration
) {
    static size_t id = 0;
    if (NULL == f) { return; }
    fprintf(f, "\n<style type=\"text/css\">");
    fprintf(f, "\n#annulus%02zu {animation:annulus%02zu %.2fs ease-out infinite}", id, id, total_anim_duration);
    fprintf(f, "\n@keyframes annulus%02zu {", id);
    fprintf(f, "\n    0%%{stroke-width:0px}");
    int pct1 = calc_pct(first_frame, num_frames);
    int pct2 = calc_pct(first_frame+1, num_frames);
    fprintf(f, "\n  %3d%%{stroke-width:0px}", pct1);
    fprintf(f, "\n  %3d%%{stroke-width:%dpx}", pct2, MIN_DIST);
    fprintf(f, "\n  100%%{stroke-width:%dpx}", MIN_DIST);
    fprintf(f, "\n}");
    fprintf(f, "\n</style>");
    fprintf(f, "\n<circle id=\"annulus%02zu\"", id);
    fprintf(f,          " cx=\"%.2f\"", p.x);
    fprintf(f,          " cy=\"%.2f\"", p.y);
    fprintf(f,          " r=\"%.1f\"", 1.5f * (float)MIN_DIST);
    fprintf(f,          " fill=\"none\"");
    fprintf(f,          " stroke=\"%s\"", ANNULUS_COLOR);
    fprintf(f,          " stroke-width=\"%d\" />", MIN_DIST);
    ++id;
}


/** record a permanent point appearing */
static int script_add_permanent_point(Script *script) {
    if (script->n_points >= MAX_POINTS) { return 0; }
    script->point_times[script->n_points] = script->n_points;
    /* it immediately becomes active */
    script->active[script->n_points].start = script->n_points;
    script->active[script->n_points].end = N_ANIM_FRAMES+1;
    ++(script->n_points);
    return 1;
}


/** record when a grid space fills */
static int script_fill_grid_space(Script *script, size_t row, size_t col) {
    if (row >= MAX_ROWS) { return 0; }
    if (col >= MAX_COLS) { return 0; }
    if (script->n_points >= MAX_POINTS) { return 0; }
    script->grid[row].cells[col] = script->n_points;
    ++(script->n_points);
    return 1;
}


/** add a new point with all its bookkeeping. returns nonzero if successful */
static int insert_point(Fpd *fpd, Script *script, Pt p) {
    if (fpd->n_pts >= MAX_POINTS) { return 0; }

    /* write coord data */
    Pt *dst = &(fpd->pts[fpd->n_pts++]);
    *dst = p;


    /* insert into grid*/
    size_t row = (size_t)floor(p.y / CELL_SIZE);
    size_t col = (size_t)floor(p.x / CELL_SIZE);
    fpd->grid.rows[row].cells[col] = dst;

    /* insert into active list */
    fpd->active[fpd->n_active++] = dst;

    /* update script */
    if (!script_add_permanent_point(script)) { return 0; }
    if (!script_fill_grid_space(script, row, col)) { return 0; }

    return 1;
}


/** write out an SVG  */
static int render_script(const char *path, Script *script) {
    FILE *f = fopen(path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", path);
        goto error;
    }

    fprintf(f, "<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">", WIDTH, HEIGHT);
    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />", WIDTH, HEIGHT, BACKGROUND_COLOR );
    if (!write_css(f, script)) { return 0; }
    if (!write_grid_squares(f)) { return 0; }

    write_permanent_point(f, p, 1, script->n_frames, ANIMATION_LENGTH);
    write_grid_square_anim(f, p, 2, script->n_frames, ANIMATION_LENGTH);
    write_annulus(f, p, 1, script->n_frames, ANIMATION_LENGTH);

    fprintf(f, "\n</svg>");
    fclose(f);
    return 1;

error:
    if (NULL != f) {
        fprintf(f, "\n</svg>");
        fclose(f);
    }
    return 0;
}

/** build up a script that can be used to render out what all it did */
static int clear_script(Script *script) {
    for (size_t row=0;row<MAX_ROWS;++row) {
        for (size_t col=0;col<MAX_COLS;++col) {
            script->grid[row].cells[col] = N_ANIM_FRAMES+1;
        }
    }
    script->n_annuli = 0;
    script->n_candidates = 0;
    script->n_points = 0;
    script->n_frames = 0;
}

/** build up a script that can be used to render out what all it did */
static int build_script(Script *script) {
    clear_script(script);
    Fpd fpd = {
        .pts = {{0}},
        .n_pts = 0,
        .grid={.rows={{.cells={0}}}},
        .active = {NULL},
        .n_active = 0,
    };
    Pt p = rand_pt();
    if (!insert_point(&fpd, &script, p)) {
        return 0;
    }

    while ((fpd.n_pts < MAX_POINTS) && (fpd.n_active > 0)) {
        script_add_annulus(left_off_here);
    }

    return 1;
}

/** entry point */
int main() {
    srand(SEED);
    if (!config_check()) { return 1; }
    Script script;
    if (!build_script(&script)) { return 1; }
    if (!render_script(OUT_PATH, &script)) { return 1; }
    return 0;
}
