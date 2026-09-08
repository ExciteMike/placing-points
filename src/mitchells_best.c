/* generate svgs to demonstrate the Mitchell's Best Candidate */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"

#define COLIOP_IMPLEMENTATION
#include "coliop.h"

static float _width    = 240.f; /* width of the generated SVG. may be overridden with a command-line arg */
static float _height   = 180.f; /* height of the generated SVG. may be overridden with a command-line arg */
static float _point_radius = 4.f; /* radius of the circle we draw to indicate the points */
static int _max_points = 45;   /* maximum allowed number of points */
static int _num_candidates = 3; /* How many candidate points to test at each step. */
static const char* _point_color = "blue"; /* color of the points */
static const char* _candidate_color = "gray"; /* color of the candidate points */
static char *_out_path = "./dist/mitchells_best.svg";
static const size_t DIMENSIONS = 2;
static const float ANIM_DURATION = 45.f;

ColiopOption options[] = {
    (ColiopOption) { .destination = &_width,
                     .descrip = "Width of the generated SVG.",
                     .name = "width",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'W' },
    (ColiopOption) { .destination = &_height,
                     .descrip = "Height of the generated SVG.",
                     .name = "height",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'H' },
    (ColiopOption) { .destination = &_out_path,
                     .descrip = "Path to write the SVG file at.",
                     .name = "out",
                     .type = COLIOP_OPTTYPE_STRING,
                     .letter = 'o' },
    (ColiopOption) { .destination = &_point_radius,
                     .descrip = "Radius of the dots used to indicate points.",
                     .name = "radius",
                     .type = COLIOP_OPTTYPE_FLOAT,
                     .letter = 'r' },
    (ColiopOption) { .destination = &_point_color,
                     .descrip = "Color to draw the dots with. Can be a color name like \"pink\" or a hex value like \"#e68b93\".",
                     .name = "color",
                     .type = COLIOP_OPTTYPE_STRING,
                     .letter = 'c' },
    (ColiopOption) { .destination = &_max_points,
                     .descrip = "The number of points to generate.",
                     .name = "num-points",
                     .type = COLIOP_OPTTYPE_INT,
                     .letter = 'n' },
    (ColiopOption) { .destination = &_num_candidates,
                     .descrip = "How many candidate points to test at each step.",
                     .name = "num-candidates",
                     .type = COLIOP_OPTTYPE_INT,
                     .letter = 'N' },
};

float rand01() {
    return ((float)rand())/((float)RAND_MAX);
}
        
/* random x coordinate */
float rand_x() {
    return rand01() * _width;
}

/* random x coordinate */
float rand_y() {
    return rand01() * _height;
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
    const float *end,
    float *out_nearest_x,
    float *out_nearest_y
) {
    float lowest = FLT_MAX;
    for (float *p=begin; p<end; p+=DIMENSIONS) {
        float d = distance(candidate_x, candidate_y, p[0], p[1]);
        if (d < lowest) {
            lowest = d;
            if (NULL != out_nearest_x) {
                *out_nearest_x = p[0];
            }
            if (NULL != out_nearest_y) {
                *out_nearest_y = p[1];
            }
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

void begin_timed_group(
    FILE *file,
    const char* fill,
    const char* stroke,
    float appear_at,
    float disappear_at
) {
    fprintf(file, "<g");
    if (NULL != fill) {
        fprintf(file, " fill=\"%s\"", fill);
    }
    if (NULL != stroke) {
        fprintf(file, " stroke=\"%s\"", stroke);
    }
    fprintf(file, ">");
    write_motion(file, 0.f, 0.f, ANIM_DURATION, appear_at, disappear_at);
}
void end_timed_group(FILE *file) {
    fprintf(file, "</g>");
}

/* render a dot with a specified lifetime */
void write_temp_dot(
    FILE* file,
    float cx,
    float cy,
    float nearest_x,
    float nearest_y,
    float distance_to_nearest,
    size_t appear_frame
) {
    float frame_duration = 0.5f * ANIM_DURATION / _max_points;
    float appear_time = (float)appear_frame * frame_duration;
    float disappear_time = appear_time + frame_duration;
    begin_timed_group(file, NULL, NULL, appear_time, disappear_time);
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\"/>", cx, cy, _point_radius, _candidate_color);
    fprintf(file, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"%s\"/>", cx, cy, nearest_x, nearest_y, _candidate_color);
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" stroke=\"%s\" fill=\"none\" stroke-dasharray=\"4\"/>", cx, cy, distance_to_nearest, _candidate_color);
    end_timed_group(file);
}

/* render a dot with a specified lifetime */
void write_permanent_dot(
    FILE* file,
    float x,
    float y,
    size_t appear_frame
) {
    float frame_duration = 0.5f * ANIM_DURATION / _max_points;
    fprintf(file, "<circle r=\"%.2f\" fill=\"%s\">", _point_radius, _point_color);
    write_motion(file, x, y, ANIM_DURATION, (float)appear_frame * frame_duration, ANIM_DURATION);
    fprintf(file, "</circle>");
}


/* entry point */
int main(int argc, const char **argv) {
    srand(9905);
    ColiopConfig coliop = {
        .descrip =
            "Generate an SVG displaying sample points generated with the \"Mitchell's Best Candidate\" algorithm.",
        .options = &(options[0]),
        .num_options = sizeof(options) / sizeof(options[0]),
        .max_positional_args = 0
    };
    ColiopResult *args_result = coliop_execute(&coliop, argc, argv, stdout);
    if (NULL == args_result) {
        return 1;
    }
    if (!args_result->success) {
        coliop_free_results(args_result);
        return 1;
    }

    /* restrict to sane values */
    if (_num_candidates < 1)    { _num_candidates = 1; }
    if (_num_candidates > 1024) { _num_candidates = 1024; }

    srand(time(NULL));

    /* where to store generated points*/
    float *buf = calloc(DIMENSIONS * _max_points, sizeof(float));
    if (NULL == buf) {
        fprintf(stderr, "allocation failed");
        return 1;
    }
    size_t write_pos = 0;
    size_t end_write_pos = DIMENSIONS * _max_points;
    size_t cur_frame = 0;

    FILE *f = fopen(_out_path, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", _out_path);
        coliop_free_results(args_result);
        free(buf);
        return 1;
    }
    
    begin_svg(f, _width, _height);
    fprintf(f, "<rect x=\"0\" y=\"0\" width=\"%.0f\" height=\"%.0f\" fill=\"#f9f9f9\"/>", _width, _height);

    /* generate first point */
    if (write_pos < end_write_pos) {
        buf[write_pos++] = rand_x();
        buf[write_pos++] = rand_y();
        write_permanent_dot(f, buf[0], buf[1], cur_frame++);
    }

    /* step algorithm */
    while (write_pos < end_write_pos) {
        float best_x = -1.f;
        float best_y = -1.f;
        float highest = -1.f;
        for (int i=0;i<_num_candidates;++i) {
            float candidate_x = rand_x();
            float candidate_y = rand_y();
            float nearest_x = 0.f;
            float nearest_y = 0.f;
            float d = candidate_distance(candidate_x, candidate_y, buf, &(buf[write_pos]), &nearest_x, &nearest_y);
            write_temp_dot(f, candidate_x, candidate_y, nearest_x, nearest_y, d, cur_frame);
            if (d > highest) {
                highest = d;
                best_x = candidate_x;
                best_y = candidate_y;
            }
        }
        ++cur_frame;
        buf[write_pos++] = best_x;
        buf[write_pos++] = best_y;
        write_permanent_dot(f, best_x, best_y, cur_frame++);
    }

    end_svg(f);
    fclose(f);
    coliop_free_results(args_result);
    free(buf);

    return 0;
}