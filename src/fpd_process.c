/* poissondisk.c - Mike Meyer 2026

   Generate SVGs to demonstrate Fast Poisson Disk 
 */

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define SQRT_ONE_HALF (0.707106781187f)
#define CELL_SIZE ((float)MIN_DIST * SQRT_ONE_HALF)

enum {
    WIDTH = 240,
    HEIGHT = 180,
    MIN_DIST = 30,
    MAX_DIST = 2*MIN_DIST,
    MIN_DIST_SQ = MIN_DIST*MIN_DIST,
    MAX_DIST_SQ = MAX_DIST*MAX_DIST,
    POINT_RADIUS = 6,
    CANDIDATE_RADIUS = 12,
    MAX_POINTS = 40,
    MAX_TRIES = 4,
    MAX_COLS = 14,
    MAX_ROWS = 12,
    PATH_BUF_LEN = 32,
    SEED = 99000317,
    ANIMATION_LENGTH = 60,
    MAX_CANDIDATES = 1024,
    PAUSE_FRAMES = 30,
    JUMP_TO_FRAME = 0,
    STOP_AT_FRAME = INT_MAX,
    ANNULUS_OUTLINE_WIDTH = 1,
    DO_CURTAIN = 1
};
static const char* POINT_COLOR = "blue";
static const char* ANNULUS_FILL_COLOR = "aliceblue";
static const char* ANNULUS_BORDER_COLOR = "#999";
static const char* CANDIDATE_FILL_COLOR = "#f9f9f9";
static const char* CANDIDATE_FILL_OPACITY = "1";
static const char* CANDIDATE_SUCCESS_COLOR = "palegreen";
static const char* CANDIDATE_FAIL_COLOR = "lightpink";
static const char* CANDIDATE_OUTLINE_COLOR = "#999";
static const char* CANDIDATE_LINE_COLOR = "black";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const char* GRID_COLOR = "#ccc";
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


/** information needed to render a candidate point */
typedef struct CandidateData {
    size_t parent_idx;
    StartEnd times;
    Pt p;
    float distance_to_nearest;
    int successful;
} CandidateData;


/** data from which to generate the SVG */
typedef struct Script {
    StartEnd annuli[MAX_POINTS];
    CandidateData candidates[MAX_CANDIDATES];
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
static float dist_sq(const Pt p1, const Pt p2);
static void fpd_step(Fpd *fpd, Script *script);
static void init_fpd(Fpd *fpd);
static void insert_point(Fpd *fpd, Script *script, Pt p);
static size_t min(size_t a, size_t b);
static size_t max(size_t a, size_t b);
static Pt rand_pt();
static float rand_range(const float min, const float max);
static Pt random_from_annulus(const Pt p);
static void render_curtain_css(FILE *f, const Script *script);
static void render_points_css(FILE *f, const Script *script);
static void script_activate_last_point(Script *script);
static size_t script_add_candidate(Script *script, Pt p, size_t parent_pt_idx, float distance_to_nearest, int successful);
static void script_add_permanent_point(Script *script, Pt p);
static void script_clear_all_candidates(Script*);
static void script_clear_candidates(Script*,size_t,size_t);
static float time_warp(float);


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
    script_clear_all_candidates(script);

    /* pause at the end */
    script->n_frames += PAUSE_FRAMES;
}


/** round `n` over `d` to an integer percentage */
static float calc_pct(size_t n, size_t d) {
    n = max(JUMP_TO_FRAME, min(n, d)) - JUMP_TO_FRAME;
    d = max(JUMP_TO_FRAME+1, d) - JUMP_TO_FRAME;
    size_t stop_at = max(JUMP_TO_FRAME, STOP_AT_FRAME) - JUMP_TO_FRAME;
    n = min(n, stop_at);
    d = min(d, stop_at);
    float linear = (float)n / (float)d;
    return 100.f * time_warp(linear);
}


/** build up a script that can be used to render out what all it did */
static void clear_script(Script *script) {
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


/* distance to nearest point */
static float distance_to_nearest_point(const Fpd *fpd, const Pt p) {
    const size_t candidate_row = (size_t)floor(p.y / (float)CELL_SIZE);
    const size_t candidate_col = (size_t)floor(p.x / (float)CELL_SIZE);
    const size_t min_row = max(1, candidate_row) - 1;
    const size_t max_row = min(candidate_row + 2, MAX_ROWS - 1);
    const size_t min_col = max(1, candidate_col) - 1;
    const size_t max_col = min(candidate_col + 1, MAX_COLS - 1);
    float nearest = FLT_MAX;
    for (size_t row=min_row;row<=max_row;++row) {
        for (size_t col=min_col;col<=max_col;++col) {
            const size_t pt_idx = fpd->grid.rows[row].cells[col];
            if (SIZE_MAX == pt_idx) {
                continue;
            }
            float d = sqrtf(dist_sq(fpd->pts[pt_idx], p));
            if (d < nearest) {
                nearest = d;
            }
        }
    }
    /* no points!? */
    return nearest;
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
    size_t cur_pt_idx = fpd->active[fpd->n_active-1];
    Pt cur = fpd->pts[cur_pt_idx];
    size_t new_candidates_begin = script->n_candidates;

    size_t tries = 0;
    while (tries++ < MAX_TRIES) {
        Pt candidate = random_from_annulus(cur);
        float d = fmin(MAX_DIST, distance_to_nearest_point(fpd, candidate));
        int successful = (0.f <= candidate.x) &&
            (candidate.x <= (float)WIDTH) && 
            (0.f <= candidate.y) &&
            (candidate.y <= (float)HEIGHT) &&
            (d >= MIN_DIST);
        script_add_candidate(script, candidate, cur_pt_idx, d, successful);
        if (successful) {
            insert_point(fpd, script, candidate);
            script_clear_candidates(script, new_candidates_begin, script->n_candidates);
            return;
        }
    }

    /* retry limit exceeded, remove from active list */
    script_clear_candidates(script, new_candidates_begin, script->n_candidates);
    script->annuli[cur_pt_idx].end = script->n_frames++;
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
    script_activate_last_point(script);
}


static size_t min(size_t a, size_t b) { return (a<b)?a:b; }
static size_t max(size_t a, size_t b) { return (a>b)?a:b; }


/** choose a starting point */
static Pt rand_pt() {
    return (Pt) { rand_range(0, WIDTH), rand_range(0, HEIGHT) };
}


/** random float from the given range */
static float rand_range(const float low, const float high) {
    return low + (high - low) * ((float)rand())/((float)RAND_MAX);
}


/* Given a point `p`, select a random point in an annulus
   centered on `p`, with an inner radius equal to the 
   minimum radius and an outer radius twice the minimum radius. */
static Pt random_from_annulus(const Pt p) {
    float TAU = 2.f * 3.14159265358979323846;
    float a = rand_range(0, TAU);
    float r_sq = rand_range((float)MIN_DIST_SQ, (float)MAX_DIST_SQ);
    float r = sqrtf(r_sq);
    return (Pt) {
        p.x + r * cosf(a),
        p.y + r * sinf(a)
    };
}


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
        if (script->annuli[idx].end < JUMP_TO_FRAME) { continue; }
        if (script->annuli[idx].start > STOP_AT_FRAME) { continue; }
        Pt p = script->pts[idx];
        fprintf(f, "\n<path d=\"");
        fprintf(f, "M %.1f %.1f ", p.x, p.y - outer_radius);
        fprintf(f, "A %.1f %.1f 0 1 0 %.1f %.1f ", outer_radius, outer_radius, p.x, p.y + outer_radius);
        fprintf(f, "A %.1f %.1f 0 1 0 %.1f %.1f ", outer_radius, outer_radius, p.x, p.y - outer_radius);
        fprintf(f, "Z ");
        fprintf(f, "M %.1f %.1f ", p.x, p.y - inner_radius);
        fprintf(f, "A %.1f %.1f 0 1 1 %.1f %.1f ", inner_radius, inner_radius, p.x, p.y + inner_radius);
        fprintf(f, "A %.1f %.1f 0 1 1 %.1f %.1f ", inner_radius, inner_radius, p.x, p.y - inner_radius);
        fprintf(f, "Z ");
        fprintf(f, "\"");
        fprintf(f, " id=\"a%02zu\"", idx);
        fprintf(f, " />");
    }
    fprintf(f, "\n</g>");
}


/** write out the CSS to animate the candidate points */
static void render_candidates_css(FILE *f, const Script *script) {
    for (size_t idx=0;idx<script->n_candidates;++idx) {
        const CandidateData *data = &(script->candidates[idx]);
        size_t frame1 = min(script->n_frames-1, data->times.start);
        size_t frame2 = min(script->n_frames-1, frame1+1);
        size_t frame3 = min(script->n_frames-1, data->times.end);
        size_t frame4 = min(script->n_frames-1, frame3+1);
        float pct1 = calc_pct(frame1, script->n_frames);
        float pct2 = calc_pct(frame2, script->n_frames);
        float pct3 = calc_pct(frame3, script->n_frames);
        float pct4 = calc_pct(frame4, script->n_frames);

        fprintf(f, "\n#c%03zu{animation:c%03zu %ds linear infinite}", idx, idx, ANIMATION_LENGTH);
        fprintf(f, "\n@keyframes c%03zu {", idx);
        fprintf(f, "\n0%%,%.1f%%,%.1f%%,100%%{r:0}", pct1, pct4);
        fprintf(f, "\n%.1f%%,%.1f%%{r:%dpx}", pct2, pct3, CANDIDATE_RADIUS/*data->distance_to_nearest*/);

        const char *color = (data->successful) ? CANDIDATE_SUCCESS_COLOR : CANDIDATE_FAIL_COLOR;
        fprintf(f, "\n%.1f%%{fill:%s}", pct2, CANDIDATE_FILL_COLOR);
        fprintf(f, "\n%.1f%%{fill:%s}", pct3, color);
        fprintf(f, "\n%.1f%%{fill:none}", pct4);

        fprintf(f, "\n}");
        fprintf(f, "\n#l%03zu{animation:l%03zu %ds linear infinite}", idx, idx, ANIMATION_LENGTH);
        fprintf(f, "\n@keyframes l%03zu {", idx);
        fprintf(f, "\n0%%,%.1f%%,%.1f%%,100%%{stroke-width:0}", pct1, pct4);
        fprintf(f, "\n%.1f%%,%.1f%%{stroke-width:1px}", pct2, pct3);
        fprintf(f, "\n}");
    }
}


/** write out the svg for the candidate points */
static void render_candidates(FILE *f, const Script *script) {
    fprintf(f, "\n<g fill=\"%s\" fill-opacity=\"%s\" stroke=\"%s\">", CANDIDATE_FILL_COLOR, CANDIDATE_FILL_OPACITY, CANDIDATE_OUTLINE_COLOR);
    for (size_t idx=0;idx<script->n_candidates;++idx) {
        if (script->candidates[idx].times.end < JUMP_TO_FRAME) { continue; }
        if (script->candidates[idx].times.start > STOP_AT_FRAME) { continue; }
        Pt p = script->candidates[idx].p;
        float d = script->candidates[idx].distance_to_nearest;
        fprintf(f, "\n<circle id=\"c%03zu\"", idx);
        fprintf(f,          " cx=\"%.2f\"", p.x);
        fprintf(f,          " cy=\"%.2f\"", p.y);
        fprintf(f,          " r=\"%.1f\"", d);
        fprintf(f,          " />");
    }
    fprintf(f, "\n</g>");
    fprintf(f, "\n<g stroke=\"%s\">", CANDIDATE_LINE_COLOR);
    for (size_t idx=0;idx<script->n_candidates;++idx) {
        if (script->candidates[idx].times.end < JUMP_TO_FRAME) { continue; }
        if (script->candidates[idx].times.start > STOP_AT_FRAME) { continue; }
        Pt p = script->candidates[idx].p;
        size_t parent_idx = script->candidates[idx].parent_idx;
        Pt parent = script->pts[parent_idx];
        fprintf(f, "\n<line id=\"l%03zu\"", idx);
        fprintf(f,          " x1=\"%.2f\"", parent.x);
        fprintf(f,          " y1=\"%.2f\"", parent.y);
        fprintf(f,          " x2=\"%.2f\"", p.x);
        fprintf(f,          " y2=\"%.2f\"", p.y);
        fprintf(f,          " stroke-width=\"2\" />");
    }
    fprintf(f, "\n</g>");
}


/** write out the CSS needed to animate the SVG. returns nonzero if succesful */
static void render_css(FILE *f, const Script *script) {
    fprintf(f, "\n<style type=\"text/css\">");
    render_annuli_css(f, script);
    render_candidates_css(f, script);
    render_points_css(f, script);
    render_curtain_css(f, script);
    fprintf(f, "\n</style>");
}


/** write out CSS to animate the curtain */
static void render_curtain_css(FILE *f, const Script *script) {
    if (!DO_CURTAIN) { return; }
    fprintf(f, "\n#curtain{animation:curtain %ds linear infinite}", ANIMATION_LENGTH);
    fprintf(f, "\n@keyframes curtain{");
    size_t frame1 = script->n_frames - PAUSE_FRAMES;
    size_t frame2 = script->n_frames - PAUSE_FRAMES/2;
    float pct1 = calc_pct(frame1, script->n_frames);
    float pct2 = calc_pct(frame2, script->n_frames);
    fprintf(f, "\n0%%,%.1f%%{transform:translateX(0)}", pct1);
    fprintf(f, "\n%.1f%%,100%%{transform:translateX(%dpx)}", pct2, WIDTH);
    fprintf(f, "\n}");
}


static void render_grid(FILE *f) {
    if (NULL == f) { return; }
    fprintf(f, "\n<g id=\"grid\" stroke=\"%s\">", GRID_COLOR);
    for (size_t row=1;row<MAX_ROWS;++row) {
        fprintf(f, "\n<line x1=\"0\" y1=\"%.1f\" x2=\"%d\" y2=\"%.1f\" />", row * CELL_SIZE, WIDTH, row * CELL_SIZE);
    }
    for (size_t col=1;col<MAX_COLS;++col) {
        fprintf(f, "\n<line x1=\"%.1f\" y1=\"0\" x2=\"%.1f\" y2=\"%d\" />", col * CELL_SIZE, col * CELL_SIZE, HEIGHT);
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
static void render_points(FILE *f, const Script *script) {
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
static int render_script(const char *path, const Script *script) {
    FILE *f = fopen(path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", path);
        goto error;
    }

    fprintf(f, "<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">", WIDTH, HEIGHT);
    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />", WIDTH, HEIGHT, BACKGROUND_COLOR );
    render_css(f, script);
    render_grid(f);
    render_annuli(f, script);
    render_candidates(f, script);
    if (DO_CURTAIN) {
        fprintf(f, "\n<rect id=\"curtain\" x=\"%d\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />", -WIDTH, WIDTH, HEIGHT, BACKGROUND_COLOR );
    }
    render_points(f, script);

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


/** record a candidate point appearing. returns an index you can use to mark its end, or SIZE_MAX if something goes wrong */
static size_t script_add_candidate(Script *script, Pt p, size_t parent_pt_idx, float distance_to_nearest, int successful) {
    size_t n = script->n_candidates;
    if (n >= MAX_CANDIDATES) {
        fprintf(stderr, "increase candidate buffer size\n");
        return n-1;
    }
    script->candidates[n].parent_idx = parent_pt_idx;
    script->candidates[n].times.start = script->n_frames;
    script->candidates[n].times.end = SIZE_MAX;
    script->candidates[n].p = p;
    script->candidates[n].distance_to_nearest = successful ? (float)MIN_DIST : distance_to_nearest;
    script->candidates[n].successful = successful;
    ++(script->n_candidates);
    ++(script->n_frames);
    return n;
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

/** if any candidates remain, end them */
static void script_clear_all_candidates(Script *script) {
    int changed_any = 0;
    for (size_t i=0;i<script->n_candidates;++i) {
        if (SIZE_MAX == script->candidates[i].times.end) {
            script->candidates[i].times.end = script->n_frames;
            changed_any = 1;
        }
    }
    if (changed_any) {
        ++(script->n_frames);
    }
}


/** Record the removal of candidate points */
static void script_clear_candidates(Script *script, size_t candidates_begin, size_t candidates_end) {
    candidates_begin = min(script->n_candidates, candidates_begin);
    candidates_end = min(script->n_candidates, candidates_end);
    for (size_t candidate_idx=candidates_begin;candidate_idx<candidates_end;++candidate_idx) {
        script->candidates[candidate_idx].times.end = script->n_frames;
    }
    ++(script->n_frames);
}


/** custom ease function for overall timing */
static float time_warp(float t) {
    if ((JUMP_TO_FRAME != 0)||(STOP_AT_FRAME!=INT_MAX)) {
        return t;
    }
    float t_sq = t*t;
    float t_cu = t*t*t;
    return 0.5f*t_cu - 1.5f*t_sq + 2.f*t;
}


/** entry point */
int main() {
    srand(SEED);
    Script script;
    if (!config_check()) { return 1; }
    build_script(&script);
    render_script(OUT_PATH, &script);
    return 0;
}
