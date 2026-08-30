/* fast_poisson_disk.h - Mike Meyer 2026

   Provides a function to distribute points in a 2D rect using the Fast Poisson Disk algorithm from Bridson

   (Robert Bridson. 2007. Fast Poisson disk sampling in arbitrary dimensions. In ACM SIGGRAPH 2007 sketches (SIGGRAPH '07). Association for Computing Machinery, New York, NY, USA, 22–es. <a href="https://doi.org/10.1145/1278780.1278807">https://doi.org/10.1145/1278780.1278807</a>)

   Single-file library in the style of "stb" libraries.
   To create the implementation,
       #define FAST_POISSON_DISK_IMPLEMENTATION
   in *one* C/CPP file that includes this file.
 */



#ifdef __cplusplus
extern "C" {
#endif

/*
   Generate points using the Fast Poisson Disk algorithm described by Bridson.
   (https://doi.org/10.1145/1278780.1278807)

   Allocates temporary memory using `calloc`, `realloc`, and `free`. Final 
   results are stored in a buffer you provide. The number of points (not 
   floats) written is stored in `out_num_written`.

   buf       - Buffer to which to write the positions. Each point uses two 
               consecutive slots in the array, one for x and one for y
   buf_size  - How many floats there is room for in buf, limiting the maximum 
               number of points this function can produce
   width     - Maximum of x value of produced points.
   height    - Maximum of y value of produced points.
   min_dist  - Minimum allowed distance between generated points.
   max_tries - Limit the number of tries from any previously generated point.
               Higher values can produce denser distribution at the cost of 
               speed. The recommended value is 30.
   out_num_points - Will contain the number of *points* written to buf

   return value - nonzero if successful
 */
int fast_poisson_disk(
    float *buf,
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

/* the algorithm could easily be expanded to handle higher dimensions */
const size_t FPD_DIMENSIONS = 2;

typedef struct FpdState {
    float *write_head;
    float *buf_end;
    float width;
    float height;
    float min_dist;

    /* Grid: index by `(row * num_cols) + col` to get a pointer to the first 
       of the pair of coords for the point in that grid square or NULL */
    float **grid;

    /* cell size chosen to make sure no more than one point can fit in a cell.
       NOTE: if adapting this for dimensions other than 2, change that sqrt(2) 
       to sqrt(num_dimensions) */
    float cell_size;

    size_t num_rows;
    size_t num_cols;

    /* each item in the list points to the first of the coords for a point */
    float **active_list;
    size_t list_len;
    size_t max_tries;

    size_t count;
} FpdState;

size_t fpd_min(size_t a, size_t b) { return (a<b) ? a : b; }
size_t fpd_max(size_t a, size_t b) { return (a>b) ? a : b; }

/* create the initial state */
int fpd_init(
    FpdState *fpd, 
    float *buf,
    size_t buf_size,
    float width,
    float height,
    float min_dist,
    size_t max_tries,
    size_t *out_num_points
) {
    if (NULL == fpd) {
        return 0;
    }    
    if (NULL == buf) {
        return 0;
    }
    if (NULL == out_num_points) {
        return 0;
    }
    fpd->write_head = &(buf[0]);
    fpd->buf_end = fpd->write_head + buf_size;
    fpd->width = width;
    fpd->height = height;
    fpd->min_dist = min_dist;
    fpd->grid = NULL;
    fpd->cell_size = min_dist / sqrt((float)FPD_DIMENSIONS);
    fpd->num_rows = (size_t)fpd_max(1, ceil(height / fpd->cell_size));
    fpd->num_cols = (size_t)fpd_max(1, ceil(width / fpd->cell_size));
    fpd->active_list = NULL;
    fpd->list_len = 0;
    fpd->max_tries = max_tries;
    fpd->count = 0;

    fpd->grid = calloc(fpd->num_rows * fpd->num_cols, sizeof(fpd->grid[0]));
    if (NULL == fpd->grid) {
        return 0;
    }

    // TODO: this is usually way more than we need. look into reserving less 
    // and resize when it fills
    size_t max_points = fpd_min(buf_size / FPD_DIMENSIONS, fpd->num_rows * fpd->num_cols);
    fpd->active_list = calloc(max_points, sizeof(fpd->active_list[0]));
    if (NULL == fpd->active_list) {
        return 0;
    }
    return 1;
}

/* return nonzero if candidate point seems ok */
int fpd_distance_check(FpdState *fpd, float x, float y) {
    float min_dist_sq = fpd->min_dist * fpd->min_dist;
    size_t row = (size_t)floor(y / fpd->cell_size);
    size_t col = (size_t)floor(x / fpd->cell_size);
    size_t min_row = (size_t) fpd_max(1, row) - 1;
    size_t max_row = (size_t) fpd_min(row + 2, fpd->num_rows - 1);
    size_t min_col = (size_t) fpd_max(1, col) - 1;
    size_t max_col = (size_t) fpd_min(col + 1, fpd->num_cols - 1);
    for (size_t row=min_row;row<=max_row;++row) {
        for (size_t col=min_col;col<=max_col;++col) {
            size_t index = row * fpd->num_cols + col;
            float * ptr = fpd->grid[index];
            if (NULL == ptr) {
                continue;
            }
            float dx = ptr[0] - x;
            float dy = ptr[1] - y;
            float d_sq = dx*dx + dy*dy;
            if (d_sq < min_dist_sq) {
                return 0;
            }
        }
    }
    return 1;
}

/* free resources allocated by the FpdState. But not the FpdState itself. That's on the stack. */
int fpd_cleanup(FpdState *fpd) {
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
float fpd_rand(float max) {
    return ((float)rand())/((float)RAND_MAX) * max;
}

/* Helper for fast_poisson_disk - Uniformly select a random point in an annulus
   around another point */
void fpd_random_radius(float cx, float cy, float min_r, float max_r, float *out_x, float *out_y) {
    float TAU = 2.f * PI;
    float min_r_sq = min_r*min_r;
    float max_r_sq = max_r*max_r;
    float r_sq = min_r_sq + fpd_rand(max_r_sq - min_r_sq);
    float a = fpd_rand(TAU);
    float r = sqrt(r_sq);
    *out_x = cx + r * cosf(a);
    *out_y = cy + r * sinf(a);
}

int fpd_insert(FpdState *fpd, float x, float y) {
    if (NULL == fpd) {
        return 0;
    }
    if (fpd->write_head + FPD_DIMENSIONS > fpd->buf_end) {
        return 0;
    }

    /* write coord data */
    fpd->write_head[0] = x;
    fpd->write_head[1] = y;
    fpd->count++;

    /* insert into grid*/
    size_t row = (size_t)floor(y / fpd->cell_size);
    size_t col = (size_t)floor(x / fpd->cell_size);
    size_t index = row * fpd->num_cols + col;
    if ((index > fpd->num_cols * fpd->num_rows) ||
        (NULL != fpd->grid[index])
    ) {
        return 0;
    }
    fpd->grid[index] = fpd->write_head;

    /* insert into active list */
    fpd->active_list[fpd->list_len++] = fpd->write_head;

    /* advance write_head */
    fpd->write_head += FPD_DIMENSIONS;
    return 1;
}

/* helper for fast_poisson_disk - nonzero if algorithm is over or buffer is full */
int fpd_done(FpdState *fpd) {
    return (fpd->write_head >= fpd->buf_end) || (fpd->list_len == 0);
}

/* helper for fast_poisson_disk - check the current active point. returns nonzero if successful */
int fpd_peek(FpdState *fpd, float *x, float *y) {
    if (0 == fpd->list_len) {
        return 0;
    }
    float *ptr = fpd->active_list[fpd->list_len - 1];
    *x = ptr[0];
    *y = ptr[1];
    return 1;
}

/* helper for fast_poisson_disk - do one iteration of the algorithm. returns nonzero if successful */
int fpd_step(FpdState *fpd) {
    float cur_x;
    float cur_y;
    if (!fpd_peek(fpd, &cur_x, &cur_y)) {
        return 0;
    }

    size_t tries = 0;
    while (tries++ < fpd->max_tries) {
        float candidate_x;
        float candidate_y;
        fpd_random_radius(cur_x, cur_y, fpd->min_dist, 2 * fpd->min_dist, &candidate_x, &candidate_y);
        if ((0.f <= candidate_x) &&
            (candidate_x <= fpd->width) && 
            (0.f <= candidate_y) &&
            (candidate_y <= fpd->height) &&
            fpd_distance_check(fpd, candidate_x, candidate_y)
        ) {
            return fpd_insert(fpd, candidate_x, candidate_y);
        }
    }

    /* retry limit exceeded, remove from active list */
    fpd->list_len--;

    return 1;
}

/* see comments on the forward declaration above */
int fast_poisson_disk(
    float *buf,
    size_t buf_size,
    float width,
    float height,
    float min_dist,
    size_t max_tries,
    size_t *out_num_points
) { 
    *out_num_points = 0;

    /* early out: if asked for nothing, do nothing */
    if (buf_size < 2) {
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
        max_tries,
        out_num_points)
    ) {
        fpd_cleanup(&fpd);
        return 0;
    }
    
    /* insert initial point */
    if (!fpd_insert(&fpd, fpd_rand(width), fpd_rand(height))) {
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

#endif // FAST_POISSON_DISK_IMPLEMENTATION