/* generate svgs to demonstrate the Mitchell's Best Candidate */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"

static const float WIDTH    = 320.f; /* width of the generated SVG. */
static const float HEIGHT   = 180.f; /* height of the generated SVG. */
static const float POINT_RADIUS = 4.f; /* radius of the circle we draw to indicate the points */
static const size_t MAX_POINTS = 120; /* maximum allowed number of points */
static const char* PERMANENT_POINT_COLOR = "blue";
static const char* BAD_POINT_COLOR = "crimson";
static const char* GOOD_POINT_COLOR = "limegreen";
static const char *OUT_PATH = "./dist/dart_throwing.svg";
static const float MIN_DISTANCE = 16.f; /* minimum distance between generated points */
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const size_t DIMENSIONS = 2;
static const size_t MAX_FRAMES = MAX_POINTS;
static const float ANIM_DURATION = 45.f;
static const float FRAME_TIME = ANIM_DURATION / (float)MAX_FRAMES;


float rand01() {
    return ((float)rand())/((float)RAND_MAX);
}
        
/* random x coordinate */
float rand_x() {
    return rand01() * WIDTH;
}

/* random x coordinate */
float rand_y() {
    return rand01() * HEIGHT;
}

/* calculate the distance between two points */
float distance(float x1, float y1, float x2, float y2) {
    float dx = x2-x1;
    float dy = y2-y1;
    return sqrt(dx*dx + dy*dy);
}

/* finds the distance from the candidate point to the nearest point in the provided array */
float candidate_distance(
    float candidate_x,
    float candidate_y,
    float *begin,
    float *end
) {
    float lowest = FLT_MAX;
    for (float *p=begin; p<end; p+=DIMENSIONS) {
        float d = distance(candidate_x, candidate_y, p[0], p[1]);
        if (d < lowest) {
            lowest = d;
        }
    }
    return lowest;
}

/* write out the animateMotion tag that effectively makes the thing appear and disappear at the specified times */
void write_motion(
    FILE* file,
    float x,
    float y,
    float total_time,
    float appear_at,
    float disappear_at
) {
    total_time = fmaxf(total_time, FLT_EPSILON);
    appear_at = fmaxf(0.f, fminf(appear_at, total_time));
    disappear_at = fmaxf(appear_at, fminf(disappear_at, total_time));
    float vis_time = disappear_at - appear_at;
    float before_weight = appear_at / total_time;
    float vis_weight = vis_time / total_time;
    float after_weight = (total_time - disappear_at) / total_time;
    fprintf(
        file,
        "<animateMotion dur=\"%.2f\" "
        "repeatCount=\"indefinite\" "
        "path=\"M-1000,0 L-1000,%.6f "
        "M%.6f,%.6f L%.6f,%.6f"
        "M-1000,0 L-1000,%.6f\"/>",
        total_time,
        before_weight * 0.01f,
        x,
        y,
        x,
        y + vis_weight * 0.01f,
        after_weight * 0.01f);
}


/* render a dot with a specified lifetime */
void write_permanent_dot(
    FILE* file,
    float x,
    float y,
    size_t appear_frame
) {
    float begin_time = (float)appear_frame * FRAME_TIME;
    fprintf(file, "<circle r=\"%.2f\" fill=\"%s\">", POINT_RADIUS, PERMANENT_POINT_COLOR);
    write_motion(file, x, y, ANIM_DURATION, begin_time, ANIM_DURATION);
    fprintf(file, "</circle>");
}


/* render a dot with a specified lifetime */
void write_candidate_dot(
    FILE* file,
    float x,
    float y,
    const char *color,
    size_t appear_frame
) {
    fprintf(file, "<g>");
    float begin_time = (float)appear_frame * FRAME_TIME;
    float end_time = begin_time + FRAME_TIME;
    write_motion(file, x, y, ANIM_DURATION, begin_time, end_time);
    fprintf(file, "<circle r=\"%.2f\" fill=\"%s\"/>", POINT_RADIUS, color);
    fprintf(file, "<circle r=\"%.2f\" fill=\"none\" stroke=\"%s\"/>", MIN_DISTANCE, color);
    fprintf(file, "</g>");
}


/* entry point */
int main(int argc, const char **argv) {
    srand(9906);

    /* where to store generated points*/
    float *buf = calloc(DIMENSIONS * MAX_POINTS, sizeof(float));
    if (NULL == buf) {
        fprintf(stderr, "allocation failed");
        return 1;
    }
    size_t write_pos = 0;
    size_t end_write_pos = DIMENSIONS * MAX_POINTS;
    size_t cur_frame = 0;

    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", OUT_PATH);
        return 1;
    }
    
    begin_svg(f, WIDTH, HEIGHT);

    /* background */
    fprintf(f, "<rect x=\"0\" y=\"0\" width=\"%.0f\" height=\"%.0f\" fill=\"%s\"/>", WIDTH, HEIGHT, BACKGROUND_COLOR);

    /* generate first point */
    if (write_pos < end_write_pos) {
        buf[write_pos++] = rand_x();
        buf[write_pos++] = rand_y();
        write_permanent_dot(f, buf[0], buf[1], cur_frame);
    }

    /* step algorithm */
    while ((write_pos < end_write_pos) && (cur_frame < MAX_FRAMES)) {
        ++cur_frame;
        float candidate_x = rand_x();
        float candidate_y = rand_y();
        float d = candidate_distance(candidate_x, candidate_y, buf, &(buf[write_pos]));
        if (d >= MIN_DISTANCE) {
            write_candidate_dot(f, candidate_x, candidate_y, GOOD_POINT_COLOR, cur_frame);
            write_permanent_dot(f, candidate_x, candidate_y, cur_frame);
            buf[write_pos++] = candidate_x;
            buf[write_pos++] = candidate_y;
        } else {
            write_candidate_dot(f, candidate_x, candidate_y, BAD_POINT_COLOR, cur_frame);
        }
    }

    end_svg(f);
    fclose(f);
    free(buf);

    return 0;
}