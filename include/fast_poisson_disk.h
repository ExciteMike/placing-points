/* fast_poisson_disk.h - Mike Meyer 2026

   Provides a function to distribute points in a 2D rect using the Fast Poisson Disk algorithm from Bridson

   (Robert Bridson. 2007. Fast Poisson disk sampling in arbitrary dimensions. In ACM SIGGRAPH 2007 sketches (SIGGRAPH '07). Association for Computing Machinery, New York, NY, USA, 22–es. <a href="https://doi.org/10.1145/1278780.1278807">https://doi.org/10.1145/1278780.1278807</a>)

   Single-file library in the style of "stb" libraries.
   To create the implementation,
       #define FAST_POISSON_DISK_IMPLEMENTATION
   in *one* C/CPP file that includes this file.
 */
#include <stddef.h>
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Generate points using the Fast Poisson Disk algorithm described by Bridson.
 *  (https://doi.org/10.1145/1278780.1278807)
 *
 *  Allocates temporary memory using `calloc`, `realloc`, and `free`. Final 
 *  results are stored in a buffer you provide. The number of points written is
 *  stored in `out_num_written`.
 *
 *  buf       - Buffer to which to write the positions.
 *  buf_size  - How many points there is room for in buf, limiting the maximum 
 *              number of points this function can produce
 *  width     - Maximum of x value of produced points.
 *  height    - Maximum of y value of produced points.
 *  min_dist  - Minimum allowed distance between generated points.
 *  max_tries - Limit the number of tries from any previously generated point.
 *              Higher values can produce denser distribution at the cost of 
 *              speed. The recommended value is 30.
 *  out_num_points - Will contain the number of *points* written to buf
 *
 *  return value - nonzero if successful
 */
int fast_poisson_disk(
    Pt *buf,
    size_t buf_size,
    float width,
    float height,
    float min_dist,
    size_t max_tries,
    size_t *out_num_points_written
);

#ifdef __cplusplus
}
#endif


#ifdef FAST_POISSON_DISK_IMPLEMENTATION

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* the algorithm could easily be expanded to handle higher dimensions */
static const size_t FPD_DIMENSIONS = 2;


/** everything we need to track as we step the algorithm */
typedef struct FpdState {
    Pt *buf;
    size_t buf_size;
    float width;
    float height;
    float min_dist_sq;
    size_t max_tries;
    size_t max_points;
    size_t count;

    /**
     *  Grid: index by `(row * num_cols) + col` to get a pointer to the first 
     *  of the pair of coords for the point in that grid square or NULL
     */
    Pt **grid;

    /**
     *  cell size chosen to make sure no more than one point can fit in a cell.
     *  Should equal the minimum distance between points divided by the square 
     *  root of the number of dimensions.
     */
    float cell_size;

    size_t num_rows;
    size_t num_cols;

    /* each item in the list points to the first of the coords for a point */
    Pt **active_list;
    size_t list_len;
} FpdState;


static size_t fpd_min(size_t a, size_t b) { return (a<b) ? a : b; }
static size_t fpd_max(size_t a, size_t b) { return (a>b) ? a : b; }
static float dist_sq(Pt p1, Pt p2) {
    float x = p2.x-p1.x;
    float y = p2.y-p1.y;
    return x*x + y*y;
}


/* create the initial state */
static int fpd_init(
    FpdState *fpd, 
    Pt *buf,
    size_t buf_size,
    float width,
    float height,
    float min_dist,
    size_t max_tries
) {
    if (NULL == fpd) {
        return 0;
    }    
    if (NULL == buf) {
        return 0;
    }
    const float cell_size = min_dist / sqrt((float)FPD_DIMENSIONS);
    const size_t num_rows = (size_t)fpd_max(1, ceil(height / cell_size));
    const size_t num_cols = (size_t)fpd_max(1, ceil(width / cell_size));
    const size_t max_points = fpd_min(buf_size, num_rows * num_cols);

    fpd->buf = buf;
    fpd->buf_size = buf_size;
    fpd->width = width;
    fpd->height = height;
    fpd->min_dist_sq = min_dist*min_dist;
    fpd->max_tries = max_tries;
    fpd->count = 0;
    fpd->max_points = max_points;

    fpd->grid = NULL;
    fpd->cell_size = cell_size;
    fpd->num_rows = num_rows;
    fpd->num_cols = num_cols;
    fpd->grid = calloc(fpd->num_rows * fpd->num_cols, sizeof(fpd->grid[0]));
    if (NULL == fpd->grid) {
        return 0;
    }

    /* allocating here for worst case. you may prefer to start smaller and resize as needed */
    fpd->active_list = calloc(max_points, sizeof(fpd->active_list[0]));
    if (NULL == fpd->active_list) {
        return 0;
    }
    fpd->list_len = 0;

    return 1;
}


/* return nonzero if candidate point seems ok */
static int fpd_distance_check(const FpdState *fpd, const Pt p) {
    const float min_dist_sq = fpd->min_dist_sq;
    const size_t candidate_row = (size_t)fmax(0.f, fmin((float)(fpd->num_rows), floor(p.y / fpd->cell_size)));
    const size_t candidate_col = (size_t)fmax(0.f, fmin((float)(fpd->num_cols), floor(p.x / fpd->cell_size)));
    const size_t min_row = (size_t) fpd_max(2, candidate_row) - 2;
    const size_t max_row = (size_t) fpd_min(candidate_row + 2, fpd->num_rows - 1);
    const size_t min_col = (size_t) fpd_max(2, candidate_col) - 2;
    const size_t max_col = (size_t) fpd_min(candidate_col + 2, fpd->num_cols - 1);
    for (size_t row=min_row;row<=max_row;++row) {
        for (size_t col=min_col;col<=max_col;++col) {
            const size_t index = row * fpd->num_cols + col;
            const Pt * ptr = fpd->grid[index];
            if (NULL == ptr) {
                continue;
            }
            float d_sq = dist_sq(*ptr, p);
            if (d_sq < min_dist_sq) {
                /* found a point that it is too close to */
                return 0;
            }
        }
    }
    /* no point was too close */
    return 1;
}


/* free resources allocated for the FpdState. But not the FpdState itself. That's on the stack. */
static int fpd_cleanup(FpdState *fpd) {
    if (NULL == fpd) {
        return 0;
    }
    if (NULL != fpd->grid) {
        free(fpd->grid);
        fpd->grid = NULL;
    }
    if (NULL != fpd->active_list) {
        free(fpd->active_list);
        fpd->active_list = NULL;
    }
    return 1;
}


/* helper for fast_poisson_disk */
static float fpd_randrange(const float min, const float max) {
    return min + (max - min) * ((float)rand())/((float)RAND_MAX);
}


/* Given a point `p` and the square of the minimum radius, 
   uniformly select a random point in an annulus
   centered on `p`, with an inner radius equal to the 
   minimum radius and an outer radius twice the minimum radius. */
static Pt fpd_random_from_annulus(const Pt p, const float min_r_sq) {
    const float TAU = 2.f * 3.14159265358979323846;
    float a = fpd_randrange(0, TAU);
    float r_sq = fpd_randrange(min_r_sq, 4.f * min_r_sq);
    float r = sqrtf(r_sq);
    return (Pt) {
        p.x + r * cosf(a),
        p.y + r * sinf(a)
    };
}


/** add a new point with all its bookkeeping. returns nonzero if successful */
static int fpd_insert(FpdState *fpd, Pt p) {
    if (NULL == fpd) {
        return 0;
    }
    if (fpd->count >= fpd->buf_size) {
        return 0;
    }

    /* write coord data */
    Pt *dst = &(fpd->buf[fpd->count]);
    *dst = p;

    /* insert into grid*/
    size_t row = (size_t)floor(p.y / fpd->cell_size);
    size_t col = (size_t)floor(p.x / fpd->cell_size);
    size_t index = row * fpd->num_cols + col;
    if ((index >= fpd->num_cols * fpd->num_rows) ||
        (NULL != fpd->grid[index])
    ) {
        return 0;
    }
    fpd->grid[index] = dst;

    /* insert into active list */
    fpd->active_list[fpd->list_len++] = dst;

    /* advance */
    fpd->count++;
    return 1;
}


/* nonzero if algorithm is over or buffer is full */
static int fpd_done(const FpdState *fpd) {
    return (fpd->count >= fpd->max_points) || (fpd->list_len == 0);
}


/* do one iteration of the algorithm. returns nonzero if successful */
static int fpd_step(FpdState *fpd) {
    if (NULL == fpd) {
        return 0;
    }
    if (fpd->list_len < 1) {
        return 0;
    }
    Pt cur = *(fpd->active_list[fpd->list_len - 1]);

    size_t tries = 0;
    while (tries++ < fpd->max_tries) {
        Pt candidate = fpd_random_from_annulus(cur, fpd->min_dist_sq);
        if ((0.f <= candidate.x) &&
            (candidate.x <= fpd->width) && 
            (0.f <= candidate.y) &&
            (candidate.y <= fpd->height) &&
            fpd_distance_check(fpd, candidate)
        ) {
            return fpd_insert(fpd, candidate);
        }
    }

    /* retry limit exceeded, remove from active list */
    fpd->list_len--;

    return 1;
}


/* see comments on the forward declaration above */
int fast_poisson_disk(
    Pt *buf,
    size_t buf_size,
    float width,
    float height,
    float min_dist,
    size_t max_tries,
    size_t *out_num_points
) {
    *out_num_points = 0;

    /* early out: if asked for nothing, do nothing */
    if (buf_size < 1) {
        return 1;
    }
    
    FpdState fpd = {0};
    if (!fpd_init(
        &fpd,
        buf,
        buf_size,
        width,
        height,
        min_dist,
        max_tries)
    ) {
        fpd_cleanup(&fpd);
        return 0;
    }
    
    /* insert initial point */
    if (!fpd_insert(&fpd, (Pt) { fpd_randrange(0, width), fpd_randrange(0, height) })) {
        fpd_cleanup(&fpd);
        return 0;
    }
    *out_num_points = fpd.count;
    
    /* keep placing more points until we can't */
    while (!fpd_done(&fpd)) {
        if (!fpd_step(&fpd)) {
            fpd_cleanup(&fpd);
            return 0;
        }
        *out_num_points = fpd.count;
    }

    *out_num_points = fpd.count;
    fpd_cleanup(&fpd);
    return 1;
}

#endif /* FAST_POISSON_DISK_IMPLEMENTATION */
