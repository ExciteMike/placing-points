/* generate svgs to demonstrate the Mitchell's Best Candidate */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

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
static const char* SECONDARY_COLOR = "gray";
static const char *OUT_PATH = "./dist/aperiodic.svg";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const float PI      = 3.14159265358979323846f;
static const float PI_3    = 1.04719755119659774615f;
static const float PI_6    = 0.52359877559829887307f;
static const float SQRT3   = 1.73205080756887729352f;
static const float SQRT3_2 = 0.86602540378443864676f;
static const size_t N_ITERATIONS = 3;

#define TILEBUFSIZE (64)

typedef float Float2[2];
typedef size_t Edge[2];

/* spectre-tile polygon */
Float2 SPECTRE_POINTS[14] = {
    {0.f,                0.f},
    {1.f,              0.f},
    {1.5f,              -SQRT3_2},
    {1.5+SQRT3_2, 0.5-SQRT3_2},
    {1.5+SQRT3_2, 1.5-SQRT3_2},
    {2.5+SQRT3_2, 1.5-SQRT3_2},
    {3+SQRT3_2,   1.5},
    {3.0,              2.0},
    {3-SQRT3_2,   1.5},
    {2.5-SQRT3_2, 1.5+SQRT3_2},
    {1.5-SQRT3_2, 1.5+SQRT3_2},
    {0.5-SQRT3_2, 1.5+SQRT3_2},
    {-SQRT3_2,    1.5},
    {0.0,              1.0}
};

/* indices into SPECTRE_POINTS */
typedef Float2 Quad[4];

typedef float XForm[6];
XForm IDENTITY = {1.f, 0.f, 0.f, 0.f, 1.f, 0.f};

/* mapping between metatiles and transformations on them */
typedef struct Geometry {
    Quad *quad;
    XForm *xform;
} Geometry;

/* Metatiles */
typedef struct MetaTile {
    Quad quad;
    size_t n_subtiles;
    XForm subtiles[2];
} MetaTile;

/* indices into METATILES */
typedef enum MetaTileIndex {
    MTI_FIRST = 0,
    MTI_GAMMA = 0,
    MTI_GAMMA1,
    MTI_GAMMA2,
    MTI_DELTA,
    MTI_THETA,
    MTI_LAMBDA,
    MTI_XI,
    MTI_SIGMA,
    MTI_PHI,
    MTI_PSI,
    MTI_COUNT = MTI_PSI,
    MTI_END
} MetaTileIndex;

MetaTile METATILES[MTI_COUNT] = {0};

/* build a translation transform */
void mk_translation(Float2 v, XForm *dst) {
    (*dst)[0] = 1;
    (*dst)[1] = 0;
    (*dst)[2] = v[0];
    (*dst)[3] = 0;
    (*dst)[4] = 1;
    (*dst)[5] = v[1];
}

/* build a rotate transform */
void mk_rotation(float angle, XForm *dst) {
    float c = cosf(angle);
    float s = sinf(angle);
    (*dst)[0] = c;
    (*dst)[1] = -s;
    (*dst)[2] = 0;
    (*dst)[3] = s;
    (*dst)[4] = c;
    (*dst)[5] = 0;
}

/* compose transformations */
void mul(XForm *a, XForm *b, XForm *out) {
    (*out)[0] = (*a)[0] * (*b)[0] + (*a)[1] * (*b)[3];
    (*out)[1] = (*a)[0] * (*b)[1] + (*a)[1] * (*b)[4];
    (*out)[2] = (*a)[0] * (*b)[2] + (*a)[1] * (*b)[5] + (*a)[2];
    (*out)[3] = (*a)[3] * (*b)[0] + (*a)[4] * (*b)[3];
    (*out)[4] = (*a)[3] * (*b)[1] + (*a)[4] * (*b)[4];
    (*out)[5] = (*a)[3] * (*b)[2] + (*a)[4] * (*b)[5] + (*a)[5];
}

/* build initial shape library */
void init_meta_tiles(MetaTile *buf, size_t bufsize) {
    if (bufsize < MTI_COUNT) {
        fprintf(stderr, "insufficient space");
        exit(1);
    }

    Quad quad_init;
    memcpy(&(quad_init[ 0 ]), &(SPECTRE_POINTS[ 3 ]), sizeof(Float2));
    memcpy(&(quad_init[ 1 ]), &(SPECTRE_POINTS[ 5 ]), sizeof(Float2));
    memcpy(&(quad_init[ 2 ]), &(SPECTRE_POINTS[ 7 ]), sizeof(Float2));
    memcpy(&(quad_init[ 3 ]), &(SPECTRE_POINTS[ 11 ]), sizeof(Float2));

    for (size_t i=MTI_FIRST;i<MTI_END;++i) {
        memcpy(&(METATILES[i]), &quad_init, sizeof(Quad));
        METATILES[i].n_subtiles = 0;
    }
    METATILES[MTI_GAMMA].n_subtiles = 2;
    memcpy(&(METATILES[MTI_GAMMA].subtiles[0]),
           &IDENTITY,
           sizeof(XForm));
    XForm trans;
    mk_translation(SPECTRE_POINTS[8],  &trans);
    XForm rot;
    mk_rotation(PI_6, &rot);
    XForm xform;
    mul(&trans, &rot, &xform);
    memcpy(&(METATILES[MTI_GAMMA].subtiles[1]),
           &xform,
           sizeof(XForm));
}

/* rotation angle, starting quad point, target quad point */
typedef struct XFormRule {
    float angle;
    float start;
    float target;
} XFormRule;

XFormRule XFORMRULES[] = {
    {  PI_6, 3.f, 1.f},
    {   0.f, 2.f, 0.f},
    {  PI_6, 3.f, 1.f},
    {  PI_6, 3.f, 1.f},
    {   0.f, 2.f, 0.f},
    {  PI_6, 3.f, 1.f},
    { -PI_3, 3.f, 3.f},
};

/* use the production rules on the current tiles in input to fill output with even more tiles */
void build_supertiles(MetaTile *input, size_t input_size, MetaTile *output, size_t output_size) {
    size_t n_xform_rules = sizeof(XFORMRULES) / sizeof(XFORMRULES[0]);
    float angle = 0.f;
    XForm rotation;
    memcpy(&rotation, &IDENTITY, sizeof(rotation));
    for (size_t i=0;i<n_xform_rules;++i) {
        float xform_angle = XFORMRULES[i].angle;
        if (0.f != xform_angle) {
            angle += xform_angle;
        }
    }

/* Reference code from https://github.com/shrx/spectre:
    # First, use any of the nine-unit tiles in tileSystem to obtain
    # a list of transformation matrices for placing tiles within
    # supertiles.
    quad = tileSystem["Delta"].quad
    R = [-1, 0, 0, 0, 1, 0]


    transformations = [IDENTITY]
    total_angle = 0
    rotation = IDENTITY
    transformed_quad = list(quad)

    for _angle, _from, _to in transformation_rules:
        if(_angle != 0):
            total_angle += _angle
            rotation = trot(np.deg2rad(total_angle))
            transformed_quad = [ transPt(rotation, quad_pt) for quad_pt in quad ]

        ttt = transTo(
            transformed_quad[_to],
            transPt(transformations[-1], quad[_from])
        )
        transformations.append(mul(ttt, rotation))

    transformations = [ mul(R, transformation) for transformation in transformations ]

    # Now build the actual supertiles, labelling appropriately.
    super_rules = {
        "Gamma":  ["Pi",  "Delta", None,  "Theta", "Sigma", "Xi",  "Phi",    "Gamma"],
        "Delta":  ["Xi",  "Delta", "Xi",  "Phi",   "Sigma", "Pi",  "Phi",    "Gamma"],
        "Theta":  ["Psi", "Delta", "Pi",  "Phi",   "Sigma", "Pi",  "Phi",    "Gamma"],
        "Lambda": ["Psi", "Delta", "Xi",  "Phi",   "Sigma", "Pi",  "Phi",    "Gamma"],
        "Xi":     ["Psi", "Delta", "Pi",  "Phi",   "Sigma", "Psi", "Phi",    "Gamma"],
        "Pi":     ["Psi", "Delta", "Xi",  "Phi",   "Sigma", "Psi", "Phi",    "Gamma"],
        "Sigma":  ["Xi",  "Delta", "Xi",  "Phi",   "Sigma", "Pi",  "Lambda", "Gamma"],
        "Phi":    ["Psi", "Delta", "Psi", "Phi",   "Sigma", "Pi",  "Phi",    "Gamma"],
        "Psi":    ["Psi", "Delta", "Psi", "Phi",   "Sigma", "Psi", "Phi",    "Gamma"]
    }
    super_quad = [
        transPt(transformations[6], quad[2]),
        transPt(transformations[5], quad[1]),
        transPt(transformations[3], quad[2]),
        transPt(transformations[0], quad[1])
    ]

    return {
        label: MetaTile(
            [ [tileSystem[substitution], transformation] for substitution, transformation in zip(substitutions, transformations) if substitution ],
            super_quad
        ) for label, substitutions in super_rules.items() }*/
}


/* entry point */
int main(int argc, const char **argv) {
    srand(9910);
    MetaTile *buf_a = calloc(2*TILEBUFSIZE, sizeof(MetaTile));
    if (NULL == buf_a) {
        fprintf(stderr, "alloc failed");
        exit(1);
    }
    MetaTile *buf_b = buf_a + TILEBUFSIZE;
    init_meta_tiles(buf_a, TILEBUFSIZE);
    MetaTile *input_buf = buf_a;
    MetaTile *output_buf = buf_a;
    for (size_t i=0;i<N_ITERATIONS;++i) {
        input_buf = (i%2==0) ? buf_a : buf_b;
        output_buf = (i%2==0) ? buf_b : buf_a;
        build_supertiles(input_buf, TILEBUFSIZE, output_buf, TILEBUFSIZE);
    }

    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", OUT_PATH);
        return 1;
    }

    begin_svg(f, WIDTH, HEIGHT);
    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\"/>", WIDTH, HEIGHT, BACKGROUND_COLOR);


    float col_width = 3 * KITE_SHORT_SIDE;
    float row_height = 2 * KITE_LONG_SIDE;
    int num_rows = (int)(ceilf(HEIGHT / row_height)) + 1;
    int num_cols = (int)(ceilf(WIDTH / col_width)) + 1;
    for (int row=0;row<num_rows;++row) {
        float y = row_height * (float)row;
        for (int col=0;col<num_cols;++col) {
            if ((row==0) && (col==0)) {continue;} /* the original handles 0,0 already */
            float x = col_width * (float)col;
            if (col%2 == 0) {
                fprintf(f, "\n<use x=\"%.2f\" y=\"%.2f\" href=\"#tileable-quad\" />", x, y);
            } else {
                fprintf(f, "\n<use x=\"%.2f\" y=\"%.2f\" href=\"#tileable-quad\" transform=\"scale(-1, 1)/>", -x, y);
            }
        }
    }
    end_svg(f);
    fclose(f);

    return 0;
}
