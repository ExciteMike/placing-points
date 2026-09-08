/* poissondisk.c - Mike Meyer 2026

   Generate SVGs to demonstrate Fast Poisson Disk 
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define SQRT_ONE_HALF (0.707106781187)
#define CELL_SIZE ((float)MIN_DIST * SQRT_ONE_HALF)

enum {
    WIDTH = 240,
    HEIGHT = 180,
    MIN_DIST = 25,
    MAX_DIST = 2*MIN_DIST,
    MIN_DIST_SQ = MIN_DIST*MIN_DIST,
    MAX_DIST_SQ = MAX_DIST*MAX_DIST,
    POINT_RADIUS = 5,
    MAX_POINTS = 3,
    MAX_TRIES = 30,
    MAX_COLS = 14,
    MAX_ROWS = 11,
    PATH_BUF_LEN = 32,
    SEED = 20260907,
    ANIMATION_LENGTH = 30,
    MAX_CANDIDATES = MAX_POINTS * MAX_TRIES, /* a lot smaller in practice if I want to optimize */
    PAUSE_FRAMES = 25,
    ANNULUS_OUTLINE_WIDTH = 1
};
static const char* POINT_COLOR = "blue";
static const char* ANNULUS_FILL_COLOR = "ghostwhite";
static const char* ANNULUS_BORDER_COLOR = "gray";
static const char* CANDIDATE_COLOR = "gray";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const char* GRID_SQUARE_COLOR = "gray";
static char *OUT_PATH = "./dist/fpd_process.svg";


typedef struct Pt {
    float x;
    float y;
} Pt;


/**
 * row of the grid. Each cell can be SIZE_MAX to indicate no point is yet there,
 * or can point to the Pt in that grid space
 */
typedef struct GridRow {
    size_t cells[MAX_COLS];
} GridRow;


/**
 * Breaks the space into grid cells, each just small enough that no two 
 * points would be allowed in the same one.
 */
typedef struct Grid {
    GridRow rows[MAX_ROWS];
} Grid;


/** for each cell in a row, which frame it was filled on, or SIZE_MAX if never */
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
    /** list of active points by indx. if I want to optimize for memory usage some, this buffer can usually can be much smaller */
    size_t active[MAX_POINTS];
    size_t n_active;
} Fpd;


/** data from which to generate the SVG */
typedef struct Script {
    RowFillFrames grid[MAX_ROWS];
    StartEnd annuli[MAX_POINTS];
    StartEnd candidates[MAX_CANDIDATES];
    size_t n_candidates;
    Pt pts[MAX_POINTS];
    size_t point_times[MAX_POINTS];
    size_t n_points;
    size_t n_frames;
} Script;


/* FORWARD DECLARATIONS */
static float calc_pct(size_t n, size_t d);
static void clear_script(Script *script);
static int config_check();
static int distance_check(const Fpd *fpd, const Pt p);
static float dist_sq(const Pt p1, const Pt p2);
static void fpd_step(Fpd *fpd, Script *script);
static void init_fpd(Fpd *fpd);
static void insert_point(Fpd *fpd, Script *script, Pt p);
static size_t min(size_t a, size_t b);
static size_t max(size_t a, size_t b);
static Pt rand_pt();
static float rand_range(const float min, const float max);
static Pt random_from_annulus(const Pt p);
static void render_grid_squares_css(FILE *f, const Script *script);
static void render_points_css(FILE *f, const Script *script);
static void script_activate_last_point(Script *script);
static void script_add_permanent_point(Script *script, Pt p);
static void script_fill_grid_space(Script *script, size_t row, size_t col);

/** build up a script that can be used to render out what all it did */
static void build_script(Script *script) {
    clear_script(script);
    Fpd fpd;
    init_fpd(&fpd);
    Pt p = rand_pt();
    insert_point(&fpd, script, p);

    while ((fpd.n_pts < MAX_POINTS) && (fpd.n_active > 0)) {
        fpd_step(&fpd, script);
    }

    /* pause at the end */
    script->n_frames += PAUSE_FRAMES;
}


/** round `n` over `d` to an integer percentage */
static float calc_pct(size_t n, size_t d) {
    n = min(n, d);
    d = max(1, d);
    return 100.f * (float)n / (float)d;
}


/** build up a script that can be used to render out what all it did */
static void clear_script(Script *script) {
    for (size_t row=0;row<MAX_ROWS;++row) {
        for (size_t col=0;col<MAX_COLS;++col) {
            script->grid[row].cells[col] = SIZE_MAX;
        }
    }
    script->n_candidates = 0;
    script->n_points = 0;
    script->n_frames = 0;
}


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


/* return nonzero if candidate point seems ok */
static int distance_check(const Fpd *fpd, const Pt p) {
    const size_t candidate_row = (size_t)floor(p.y / (float)CELL_SIZE);
    const size_t candidate_col = (size_t)floor(p.x / (float)CELL_SIZE);
    const size_t min_row = max(1, candidate_row) - 1;
    const size_t max_row = min(candidate_row + 2, MAX_ROWS - 1);
    const size_t min_col = max(1, candidate_col) - 1;
    const size_t max_col = min(candidate_col + 1, MAX_COLS - 1);
    for (size_t row=min_row;row<=max_row;++row) {
        for (size_t col=min_col;col<=max_col;++col) {
            const size_t pt_idx = fpd->grid.rows[row].cells[col];
            if (SIZE_MAX == pt_idx) {
                continue;
            }
            float d_sq = dist_sq(fpd->pts[pt_idx], p);
            if (d_sq < (float)MIN_DIST_SQ) {
                /* found a point that it is too close to */
                return 0;
            }
        }
    }
    /* no point was too close */
    return 1;
}


/* calculate distance squared between two points */
static float dist_sq(const Pt p1, const Pt p2) {
    float x = p2.x-p1.x;
    float y = p2.y-p1.y;
    return x*x + y*y;
}


/**
 * one step of the fpd algorithm. it will either insert a new point
 * or pop an active point
 */
static void fpd_step(Fpd *fpd, Script *script) {
    if (fpd->n_active < 1) { return; }
    size_t pt_idx = fpd->active[fpd->n_active-1];
    Pt cur = fpd->pts[pt_idx];

    size_t tries = 0;
    while (tries++ < MAX_TRIES) {
        Pt candidate = random_from_annulus(cur);
        if ((0.f <= candidate.x) &&
            (candidate.x <= (float)WIDTH) && 
            (0.f <= candidate.y) &&
            (candidate.y <= (float)HEIGHT) &&
            distance_check(fpd, candidate)
        ) {
            insert_point(fpd, script, candidate);
            return;
        }
    }

    /* retry limit exceeded, remove from active list */
    script->annuli[pt_idx].end = script->n_frames++;
    fpd->n_active--;
}


/** get the Fast Poisson Disk struct ready for use */
static void init_fpd(Fpd *fpd) {
    fpd->n_pts = 0;
    fpd->n_active = 0;
    for (size_t row=0;row<MAX_ROWS;++row) {
        for (size_t col=0;col<MAX_COLS;++col) {
            fpd->grid.rows[row].cells[col] = SIZE_MAX;
        }
    }
}


/** choose a starting point */
static Pt rand_pt() {
    return (Pt) { rand_range(0, WIDTH), rand_range(0, HEIGHT) };
}


/** random float from the given range */
static float rand_range(const float min, const float max) {
    return min + (max - min) * ((float)rand())/((float)RAND_MAX);
}


/* Given a point `p`, select a random point in an annulus
   centered on `p`, with an inner radius equal to the 
   minimum radius and an outer radius twice the minimum radius. */
static Pt random_from_annulus(const Pt p) {
    float TAU = 2.f * 3.14159265358979323846;
    float a = rand_range(0, TAU);
    float r_sq = rand_range((float)MIN_DIST_SQ, 4.f * (float)MAX_DIST_SQ);
    float r = sqrtf(r_sq);
    return (Pt) {
        p.x + r * cosf(a),
        p.y + r * sinf(a)
    };
}


/** add a new point with all its bookkeeping. returns nonzero if successful */
static void insert_point(Fpd *fpd, Script *script, Pt p) {
    if (fpd->n_pts >= MAX_POINTS) { return; }

    /* write coord data */
    size_t new_index = fpd->n_pts;
    fpd->pts[fpd->n_pts] = p;
    ++(fpd->n_pts);

    /* insert into grid*/
    size_t row = (size_t)floor(p.y / CELL_SIZE);
    size_t col = (size_t)floor(p.x / CELL_SIZE);
    fpd->grid.rows[row].cells[col] = new_index;

    /* insert into active list */
    fpd->active[fpd->n_active++] = new_index;

    /* update script */
    script_add_permanent_point(script, p);
    script_fill_grid_space(script, row, col);
    script_activate_last_point(script);
}


static size_t min(size_t a, size_t b) { return (a<b)?a:b; }
static size_t max(size_t a, size_t b) { return (a>b)?a:b; }


/** write out the css for the rings around active points */
static void render_annuli_css(FILE *f, const Script *script) {
    enum {NAME_BUF_LEN = 16};
    for (size_t idx=0;idx<script->n_points;++idx) {
        Pt p = script->pts[idx];
        StartEnd times = script->annuli[idx];
        char name[NAME_BUF_LEN] = {'\0'};
        snprintf(name, NAME_BUF_LEN, "a%02zu", idx);
        float pct1 = calc_pct(times.start, script->n_frames);
        float pct2 = calc_pct(times.start+1, script->n_frames);
        float pct3 = calc_pct(min(script->n_frames-1, times.end), script->n_frames);
        float pct4 = calc_pct(min(script->n_frames-1, times.end)+1, script->n_frames);
        fprintf(f, "\n#%s {animation:%s %ds linear infinite}", name, name, ANIMATION_LENGTH);
        fprintf(f, "\n@keyframes %s {", name);
        fprintf(f, "\n0%%,%.1f%%,%.1f%%,100%%{transform:translate(%.1fpx,%.1fpx) scale(0)}", pct1, pct4, p.x, p.y);
        fprintf(f, "\n%.1f%%,%.1f%%{transform:translate(0,0) scale(1)}", pct2, pct3);
        fprintf(f, "\n}");
    }
}


/** write out the SVG for the rings around active points */
static void render_annuli(FILE *f, const Script *script) {
    float outer_radius = 2.f * (float)MIN_DIST;
    float inner_radius = (float)MIN_DIST;
    fprintf(f, "\n<g fill=\"%s\" stroke=\"%s\" stroke-width=\"%d\">", ANNULUS_FILL_COLOR, ANNULUS_BORDER_COLOR, ANNULUS_OUTLINE_WIDTH);
    for (size_t idx=0;idx<script->n_points;++idx) {
        Pt p = script->pts[idx];
        fprintf(f, "\n<path d=\"");
        fprintf(f, "M %.1f %.1f", p.x, p.y - outer_radius);
        fprintf(f, "A %.1f %.1f 0 1 0 %.1f %.1f", outer_radius, outer_radius, p.x, p.y + outer_radius);
        fprintf(f, "A %.1f %.1f 0 1 0 %.1f %.1f", outer_radius, outer_radius, p.x, p.y - outer_radius);
        fprintf(f, "Z");
        fprintf(f, "M %.1f %.1f", p.x, p.y - inner_radius);
        fprintf(f, "A %.1f %.1f 0 1 0 %.1f %.1f", inner_radius, inner_radius, p.x, p.y + inner_radius);
        fprintf(f, "A %.1f %.1f 0 1 0 %.1f %.1f", inner_radius, inner_radius, p.x, p.y - inner_radius);
        fprintf(f, "Z");
        fprintf(f, "\"");
        fprintf(f, " id=\"a%02zu\"", idx);
        fprintf(f, " />");
    }
    fprintf(f, "\n</g>");
}


/** write out the CSS needed to animate the SVG. returns nonzero if succesful */
static void render_css(FILE *f, const Script *script) {
    fprintf(f, "\n<style type=\"text/css\">");
    render_grid_squares_css(f, script);
    /* parentage lines */
    render_annuli_css(f, script);
    /* candidates */
    /* points */
    render_points_css(f, script);
    /* active list */
    fprintf(f, "\n</style>");
}


/** write out the CSS needed to animate the SVG. returns nonzero if succesful */
static void render_grid_squares_css(FILE *f, const Script *script) {
    enum {NAME_BUF_LEN = 16};
    if (NULL == f) { return; }
    for (size_t row=0;row<MAX_ROWS;++row) {
        for (size_t col=0;col<MAX_COLS;++col) {
            size_t frame = script->grid[row].cells[col];
            if (frame == SIZE_MAX) { continue; }
            char name[NAME_BUF_LEN] = {'\0'};
            snprintf(name, NAME_BUF_LEN, "r%02zuc%02zu", row, col);
            fprintf(f, "\n#%s {animation:%s %ds linear infinite}", name, name, ANIMATION_LENGTH);
            fprintf(f, "\n@keyframes %s {", name);
            float pct1 = calc_pct(frame, script->n_frames);
            float x = (float)CELL_SIZE * (0.5f + (float)col);
            float y = (float)CELL_SIZE * (0.5f + (float)row);
            fprintf(f, "\n0%%,%.1f%%,100%%{transform:translate(%.1fpx,%.1fpx) scale(0)}", pct1, x, y);
            float pct2 = calc_pct(frame+1, script->n_frames);
            float pct3 = calc_pct(script->n_frames-1, script->n_frames);
            fprintf(f, "\n%.1f%%,%.1f%%{transform:translate(0,0) scale(1)}", pct2, pct3);
            fprintf(f, "\n}");
        }
    }
}


/** write out the grid squares, to be animated in the CSS generated by write_grid_square_anim */
static void render_grid_squares(FILE *f, const Script *script) {
    if (NULL == f) { return; }
    fprintf(f, "\n<g stroke=\"%s\" fill=\"none\">", GRID_SQUARE_COLOR);
    for (size_t row=0;row<MAX_ROWS;++row) {
        for (size_t col=0;col<MAX_COLS;++col) {
            size_t frame = script->grid[row].cells[col];
            if (frame == SIZE_MAX) { continue; }
            fprintf(f, "\n<rect id=\"r%02zuc%02zu\"", row, col);
            fprintf(f,        " x=\"%.2f\"", col * CELL_SIZE);
            fprintf(f,        " y=\"%.2f\"", row * CELL_SIZE);
            fprintf(f,        " width=\"%.2f\"", CELL_SIZE);
            fprintf(f,        " height=\"%.2f\" />", CELL_SIZE);
        }
    }
    fprintf(f, "</g>");
}


/** write out the css for points animation */
static void render_points_css(FILE *f, const Script *script) {
    enum {NAME_BUF_LEN = 16};
    for (size_t idx=0;idx<script->n_points;++idx) {
        size_t frame = script->point_times[idx];
        char name[NAME_BUF_LEN] = {'\0'};
        snprintf(name, NAME_BUF_LEN, "p%02zu", idx);
        float pct1 = calc_pct(frame, script->n_frames);
        float pct2 = calc_pct(frame+1, script->n_frames);
        float pct3 = calc_pct(script->n_frames-1, script->n_frames);
        fprintf(f, "\n#%s {animation:%s %ds linear infinite}", name, name, ANIMATION_LENGTH);
        fprintf(f, "\n@keyframes %s {", name);
        fprintf(f, "\n0%%, %.1f%%,100%%{r:0}", pct1);
        fprintf(f, "\n%.1f%%,%.1f%%{r:%dpx}", pct2, pct3, POINT_RADIUS);
        fprintf(f, "\n}");
    }
}


/** write out the svg for the points */
static void render_points(FILE *f, Script *script) {
    fprintf(f, "\n<g fill=\"%s\">", POINT_COLOR);
    for (size_t idx=0;idx<script->n_points;++idx) {
        Pt p = script->pts[idx];
        fprintf(f, "\n<circle id=\"p%02zu\"", idx);
        fprintf(f,          " cx=\"%.2f\"", p.x);
        fprintf(f,          " cy=\"%.2f\"", p.y);
        fprintf(f,          " r=\"%d\" />", POINT_RADIUS);
    }
    fprintf(f, "\n</g>");
}


/** write out an SVG. returns nonzero if succesful */
static int render_script(const char *path, Script *script) {
    FILE *f = fopen(path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", path);
        goto error;
    }

    fprintf(f, "<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">", WIDTH, HEIGHT);
    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />", WIDTH, HEIGHT, BACKGROUND_COLOR );
    render_css(f, script);
    render_annuli(f, script);
    render_grid_squares(f, script);
    /*render lines*/
    render_points(f, script);
    /* render active */
    /* rander candidates */

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

/** mark the top active point as active */
static void script_activate_last_point(Script *script) {
    if (script->n_points < 1) { return; }
    script->annuli[script->n_points-1].start = script->n_frames++;
}

/** record a permanent point appearing */
static void script_add_permanent_point(Script *script, Pt p) {
    size_t n_pts = script->n_points;
    if (n_pts >= MAX_POINTS) { return; }
    script->pts[n_pts] = p;
    script->point_times[n_pts] = script->n_frames;
    script->annuli[n_pts].start = SIZE_MAX;
    script->annuli[n_pts].end = SIZE_MAX;
    ++(script->n_points);
    ++(script->n_frames);
}


/** record when a grid space fills */
static void script_fill_grid_space(Script *script, size_t row, size_t col) {
    if (row >= MAX_ROWS) { return; }
    if (col >= MAX_COLS) { return; }
    if (script->n_points >= MAX_POINTS) { return; }
    script->grid[row].cells[col] = script->n_frames;
    ++(script->n_frames);
}


/** entry point */
int main() {
    srand(SEED);
    Script script;
    if (!config_check()) { return 0; }
    build_script(&script);
    render_script(OUT_PATH, &script);
    return 1;
}
