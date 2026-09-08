/*
	Generate svg showing how a blue-noise-like distribution can be constructed by relaxing an arbitrary initial set of points.

    It roughly does LLoyd's algorithm, but samples a grid to approximate Voronoi regions instead of calculating them.
 */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>


enum {
	N_POINTS = 150,
	N_KEYFRAMES = 12,
	WIDTH = 240,
	HEIGHT = 180,
    CELL_SIZE = 2,
    N_COLS = (WIDTH + CELL_SIZE - 1) / CELL_SIZE,
    N_ROWS = (HEIGHT + CELL_SIZE - 1) / CELL_SIZE,
    MAX_TRIES = 100,
    /** 1 or 0. whether to loop */
    LOOP = 0
};
static const float RELAX_STRENGTH = 0.5f; /** scale the amount of correction */
static const char *OUT_PATH = "./dist/relaxation.svg";
static const char* PRIMARY_COLOR = "blue";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const char* POINT_RADIUS = "2";
static const char* ANIM_DURATION = "10s";


/** 2d point coordinates */
typedef struct Pt {
	float x;
	float y;
} Pt;


/** all point positions for a frame */
typedef struct Keyframe {
	Pt pts[N_POINTS];
} Keyframe;


/** all keyframe data */
typedef struct AllKfs {
	Keyframe kfs[N_KEYFRAMES];
} AllKfs;


typedef struct GridRow {
    /** SIZE_MAX if empty. index of the point if it has one */
    size_t cells[N_COLS];
} GridRow;


/**  */
typedef struct Grid {
    GridRow rows[N_ROWS];
} Grid;

/** clear a grid */
static void clear_grid(Grid *grid) {
    memset(grid, -1, sizeof(Grid));
}


/** distance between points */
static float dist(Pt p1, Pt p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return sqrtf(dx*dx + dy*dy);
}


/** return the index of the point in the keyframe nearest the reference pt. or SIZE_MAX if the keyframe is empty */
static size_t find_nearest(Pt ref_pt, Keyframe *kf) {
    size_t ret = SIZE_MAX;
    const Pt *pts = &(kf->pts[0]); 
    float best = FLT_MAX;
    for (size_t pt_idx=0;pt_idx<N_POINTS;++pt_idx) {
        Pt candidate = pts[pt_idx];
        float d = dist(ref_pt, candidate);
        if (d<best) {
            best = d;
            ret = pt_idx;
        }
    }  
    return ret;
}


/** set grid information based on point data in the keyframe */
static void fill_grid(Grid *grid, Keyframe *kf) {
    for (size_t row=0;row<N_ROWS;++row) {
        GridRow *row_data = &(grid->rows[row]);
        for (size_t col=0;col<N_COLS;++col) {
            Pt cell_ctr = {
                ((float)col + 0.5) * (float)CELL_SIZE,
                ((float)row + 0.5) * (float)CELL_SIZE,
            };
            row_data->cells[col] = find_nearest(cell_ctr, kf);
        }
    }
}

static float rand_x() {
    return (float)(rand() % WIDTH);
}

static float rand_y() {
    return (float)(rand() % HEIGHT);
}

static Pt rand_pt() {
    return (Pt) {rand_x(), rand_y()};
}

/** create initial point locations */
static void randomize_kf(Keyframe *kf) {
    Grid grid;
    clear_grid(&grid);

	Pt *pts = &(kf->pts[0]);
	for (size_t i=0;i<N_POINTS;++i) {
        Pt candidate = rand_pt();
        size_t row = (size_t)(candidate.y / (float)CELL_SIZE);
        size_t col = (size_t)(candidate.x / (float)CELL_SIZE);
        size_t try=0;
        for (;try<MAX_TRIES;++try) {
            if (SIZE_MAX == grid.rows[row].cells[col]) {
                pts[i] = candidate;
                grid.rows[row].cells[col] = i;
                break;
            }
        }
        if (try >= MAX_TRIES) {
            fprintf(stderr, "error generating random kf. use smaller cells, fewer points, or more tries");
            exit(1);
            return;
        }
	}
}


static float lerpf(float a, float b, float t) {
    return a + (b-a)*t;
}
static Pt lerp_pt(Pt a, Pt b, float t) {
    return (Pt) {lerpf(a.x, b.x, t), lerpf(a.y, b.y, t)};
}


/** calculate keyframe positions from previous keyframe */
static void relax_kf(Keyframe *in_kf, Keyframe *out_kf) {
    Grid grid;
    fill_grid(&grid, in_kf);

    Pt sums[N_POINTS] = {{0}};
    size_t counts[N_POINTS] = {0};
    for (size_t row=0;row<N_ROWS;++row) {
        for (size_t col=0;col<N_COLS;++col) {
            size_t idx = grid.rows[row].cells[col];
            if (idx != SIZE_MAX) {
                (sums[idx].x) += ((float)col + 0.5) * (float)CELL_SIZE;
                (sums[idx].y) += ((float)row + 0.5) * (float)CELL_SIZE;
                (counts[idx])++;
            }
        }
    }

    const Pt *in_pts = &(in_kf->pts[0]);
    Pt *out_pts = &(out_kf->pts[0]);

    for (size_t i=0;i<N_POINTS;++i) {
        size_t count = counts[i];
        if (count == 0) {
            out_pts[i] = in_pts[i];
        } else {
            Pt mean = {sums[i].x / (float)count, sums[i].y / (float)count};
            out_pts[i] = lerp_pt(in_pts[i], mean, RELAX_STRENGTH);
        }
    }
}


/** write css to animate the point */
static void write_animation(FILE *f, AllKfs *data, size_t pt_idx) {
    if (N_KEYFRAMES < 2) { return; }
    Pt ref_pt = data->kfs[N_KEYFRAMES-1].pts[pt_idx];
    fprintf(f, "\n#pt%04zu{animation:pt%04zu %s linear infinite}", pt_idx, pt_idx, ANIM_DURATION);
    fprintf(f, "\n@keyframes pt%04zu{", pt_idx);
    int denom = LOOP ? N_KEYFRAMES : (N_KEYFRAMES - 1);
    for (int kf_idx=0;kf_idx<N_KEYFRAMES;++kf_idx) {
        int pct = 100 * kf_idx / denom;
        Keyframe *kf = &(data->kfs[kf_idx]);
        Pt p = kf->pts[pt_idx];
        fprintf(f, "\n%3d%%{transform:translate(%fpx,%fpx)}", pct, p.x - ref_pt.x, p.y - ref_pt.y);
    }
    Keyframe *kf0 = &(data->kfs[0]);
    Pt p0 = kf0->pts[pt_idx];
    if (LOOP) {
        fprintf(f, "\n100%%{transform:translate(%fpx,%fpx)}", p0.x - ref_pt.x, p0.y - ref_pt.y);
    }
    fprintf(f, "\n}");
}


/** write svg for a point */
static void write_point(FILE *f, size_t id, Pt p) {
    fprintf(f, "\n<circle id=\"pt%04zu\" cx=\"%f\" cy=\"%f\" r=\"%s\" />", id, p.x, p.y, POINT_RADIUS);
}


/** write svg for the given points using the given color */
static void write_points(FILE *f, const Pt* pts, size_t n, const char* color) {
    fprintf(f, "\n<g fill=\"%s\" stroke=\"none\">", color);
    for (size_t i=0;i<n;++i) {
        write_point(f, i, pts[i]);
    }
    fprintf(f, "\n</g>");
}


/** entry point */
int main() {
	srand(9911);

	AllKfs data = {{{{{0}}}}};
	randomize_kf(&(data.kfs[0]));
	for (size_t kf=1;kf<N_KEYFRAMES;++kf) {
		Keyframe *in_kf = &(data.kfs[kf-1]);
		Keyframe *out_kf = &(data.kfs[kf]);
		relax_kf(in_kf, out_kf);
	}
	 
	FILE *f = fopen(OUT_PATH, "w");
	if (NULL == f) {
		fprintf(stderr, "could not open output file \"%s\"\n", OUT_PATH);
		return 1;
	}
    fprintf(f, "<svg width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">", WIDTH, HEIGHT, WIDTH, HEIGHT);

    fprintf(f, "\n<style type=\"text/css\">");
    for (size_t i=0;i<N_POINTS;++i) {
        write_animation(f, &data, i);
    }
    fprintf(f, "\n</style>");

    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />", WIDTH, HEIGHT, BACKGROUND_COLOR);

    const Keyframe *last_kf = &(data.kfs[N_KEYFRAMES-1]);
    const Pt *last_kf_pts = last_kf->pts;
    write_points(f, last_kf_pts, N_POINTS, PRIMARY_COLOR);
    fprintf(f, "\n</svg>");
	fclose(f);
	return 0;
}