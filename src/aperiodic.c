/*
   Generate svg showing how a a blue-noise-like distribution can be constructed from an aperiodic tiling.
   
   It uses the spectre tiling from
      Smith, D., Myers, J. S, Kaplan, C. S, & Goodman-Strauss, C. (2024). A chiral aperiodic monotile. Combinatorial Theory, 4(2). http://dx.doi.org/10.5070/C64264241
   
   Based on the Javascript implementation by those authors found at https://cs.uwaterloo.ca/~csk/spectre/spectre.js
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

#define WRITE_SVG_IMPLEMENTATION
#include "write_svg.h"
#undef WRITE_SVG_IMPLEMENTATION


static const float WIDTH = 240.f;
static const float HEIGHT = 180.f;
static const float POINT_RADIUS = 3.f;
static const char* POINT_COLOR = "blue";
static const char* SECONDARY_COLOR = "gray";
static const char *OUT_PATH = "./dist/aperiodic.svg";
static const char* BACKGROUND_COLOR = "#f9f9f9";
static const float PI_3    = 1.04719755119659774615f;
static const float PI_6    = 0.52359877559829887307f;
static const size_t N_ITERATIONS = 0;


/* types of tiles */
typedef enum TileType {
    TT_FIRST = 0,
    TT_GAMMA = 0, /* aka the mystic */
    TT_DELTA,
    TT_THETA,
    TT_LAMBDA,
    TT_XI,
    TT_PI,
    TT_SIGMA,
    TT_PHI,
    TT_PSI,
    TT_COUNT,
    TT_END = TT_COUNT,
    TT_INVALID = TT_END
} TileType;


/* point in 2d */
typedef union Float2 {
    float raw[2];
    struct {
        float x;
        float y;
    };
} Float2;


enum {N_SPECTRE_POINTS=14};

/* spectre-tile polygon */
Float2 SPECTRE_POINTS[N_SPECTRE_POINTS] = {
    {{0.f, 0.f}},
    {{1.0f, 0.0f}},
    {{1.5f, -0.8660254037844386f}},
    {{2.366025403784439f, -0.36602540378443865f}},
    {{2.366025403784439f, 0.6339745962155614f}},
    {{3.366025403784439f, 0.6339745962155614f}},
    {{3.866025403784439f, 1.5f}},
    {{3.0f, 2.0f}},
    {{2.133974596215561f, 1.5f}},
    {{1.6339745962155614f, 2.3660254037844393f}},
    {{0.6339745962155614f, 2.3660254037844393f}},
    {{-0.3660254037844386f, 2.3660254037844393f}},
    {{-0.866025403784439f, 1.5f}},
    {{0.0f, 1.0f}},
};


/* indices into SPECTRE_POINTS */
typedef union Quad {
    Float2 raw[4];
    struct {
        Float2 a;
        Float2 b;
        Float2 c;
        Float2 d;
    };
} Quad;


/* affine transformations in a matrix representation */
typedef union XForm {
    float raw[6];
    struct {
        float r00;
        float r01;
        float x;
        float r10;
        float r11;
        float y;
    };
    struct {
        float m00;
        float m01;
        float m02;
        float m10;
        float m11;
        float m12;
    };
} XForm;
XForm IDENTITY = {{ 1.f, 0.f, 0.f, 0.f, 1.f, 0.f}};
XForm FLIP     = {{-1.f, 0.f, 0.f, 0.f, 1.f, 0.f}};


/* information about a particular tile */
typedef struct ChildTile {
    struct Meta *meta;
    XForm xform;
} ChildTile;


/* a tile that may be made up of other tiles */
typedef struct Meta {
    Quad quad;
    ChildTile children[TT_COUNT];
    size_t num_children;
} Meta;


/* rotation angle, starting quad point, target quad point */
typedef struct XFormRule {
    float angle;
    size_t from_idx;
    size_t to_idx;
} XFormRule;


/*
   the tiles that can be produced from a tile, indexed by their type
 
 
   TT_INVALID marks that that slot is unused
 */
typedef TileType SubsRule[TT_COUNT];

/* substitution rules to use as we produce the tiling */
static SubsRule SUBS_RULES[TT_COUNT] = {
    {      TT_PI,   TT_DELTA, TT_INVALID,   TT_THETA,   TT_SIGMA,      TT_XI,     TT_PHI,   TT_GAMMA},
    {      TT_XI,   TT_DELTA,      TT_XI,     TT_PHI,   TT_SIGMA,      TT_XI,     TT_PHI,   TT_GAMMA},
    {     TT_PSI,   TT_DELTA,      TT_PI,     TT_PHI,   TT_SIGMA,      TT_PI,     TT_PHI,   TT_GAMMA},
    {     TT_PSI,   TT_DELTA,      TT_XI,     TT_PHI,   TT_SIGMA,      TT_PI,     TT_PHI,   TT_GAMMA},
    {     TT_PSI,   TT_DELTA,      TT_PI,     TT_PHI,   TT_SIGMA,      TT_PI,     TT_PHI,   TT_GAMMA},
    {     TT_PSI,   TT_DELTA,      TT_XI,     TT_PHI,   TT_SIGMA,     TT_PSI,     TT_PHI,   TT_GAMMA},
    {      TT_XI,   TT_DELTA,      TT_XI,     TT_PHI,   TT_SIGMA,     TT_PSI,  TT_LAMBDA,   TT_GAMMA},
    {     TT_PSI,   TT_DELTA,     TT_PSI,     TT_PHI,   TT_SIGMA,      TT_PI,     TT_PHI,   TT_GAMMA},
    {     TT_PSI,   TT_DELTA,     TT_PSI,     TT_PHI,   TT_SIGMA,     TT_PSI,     TT_PHI,   TT_GAMMA}
};


/*
   Index 0 is not transformed, the others follow the transforms we will
   generate from XFORMRULES
 */
enum {N_XFORMRULES = TT_COUNT - 1};


/* transforms for the substituted tiles */
static const XFormRule XFORMRULES[N_XFORMRULES] = {
    {  PI_6, 3, 1},
    {   0.f, 2, 0},
    {  PI_6, 3, 1},
    {  PI_6, 3, 1},
    {   0.f, 2, 0},
    {  PI_6, 3, 1},
    { -PI_3, 3, 3},
};


/* build a translation transform */
void mk_translation(const Float2 p, XForm *dst) {
    *dst = (XForm){{1.f, 0.f, p.x,
                    0.f, 1.f, p.y}};
}

/* build a translation transform that turns p1 into p2 */
void mk_translation_to(const Float2 p1, const Float2 p2, XForm *dst) {
    Float2 p = {{p2.x-p1.x, p2.y-p1.y}};
    mk_translation(p, dst);
}

/* build a rotate transform */
void mk_rotation(float angle, XForm *dst) {
    float c = cosf(angle);
    float s = sinf(angle);
    *dst = (XForm){{  c, -s, 0.f,
                    0.f,  s, 0.f}};
}

/* compose transformations */
void compose(XForm *a, XForm *b, XForm *out) {
    *out = (XForm){{
        a->m00 * b->m00 + a->m01 * b->m10,
        a->m00 * b->m01 + a->m01 * b->m11,
        a->m00 * b->m02 + a->m01 * b->m12 + a->m02,
        a->m10 * b->m00 + a->m11 * b->m10,
        a->m10 * b->m01 + a->m11 * b->m11,
        a->m10 * b->m02 + a->m11 * b->m12 + a->m12
    }};
}

/* apply a transformation to a point */
Float2 xform_point(XForm *xform, const Float2 p) {
    return (Float2) {{
        xform->m00 * p.x + xform->m01 * p.y + xform->m02,
        xform->m10 * p.x + xform->m11 * p.y + xform->m12
    }};
}

/* apply a transformation to a point, modifying it in place */
void xform_point_in_place(XForm *xform, Float2 *p) {
    float x = p->x;
    float y = p->y;
    p->x = xform->m00 * x + xform->m01 * y + xform->m02;
    p->y = xform->m10 * x + xform->m11 * y + xform->m12;
}

/* add a new tile to the tile type */
void add_child(Meta *tc, Meta *meta, XForm *xform) {
    if (tc->num_children >= TT_COUNT) {
        fprintf(stderr, "added too many children");
        exit(1);
    }
    tc->children[tc->num_children].meta = meta;
    tc->children[tc->num_children].xform = *xform;
    tc->num_children++;
}

enum {TILES_CAPACITY=256};
static Meta ALL_TILES[TILES_CAPACITY];
static size_t num_tiles = 0;
Meta* alloc_tiles(size_t request_size) {
    size_t new_size = num_tiles + request_size;
    if (num_tiles + request_size > TILES_CAPACITY) {
        fprintf(stderr, "increase buffer size >=%lld", new_size);
        exit(1);
    }
    Meta* ptr = &(ALL_TILES[num_tiles]);
    num_tiles = new_size;
    return ptr;
}

/*
 Initialize a tile system that we can use build_super_tiles on.
 */
Meta* init_meta_tiles() {
    Meta *buf = alloc_tiles(TT_COUNT);
    
    Meta st_init = {
        .quad = {{SPECTRE_POINTS[ 3 ],
                  SPECTRE_POINTS[ 5 ],
                  SPECTRE_POINTS[ 7 ],
                  SPECTRE_POINTS[ 11 ]}},
        .children = {
            {NULL, IDENTITY},
            {NULL, IDENTITY},
            {NULL, IDENTITY},
            {NULL, IDENTITY},
            {NULL, IDENTITY},
            {NULL, IDENTITY},
            {NULL, IDENTITY},
            {NULL, IDENTITY},
        },
    };

    for (size_t i=TT_FIRST;i<TT_END;++i) {
        buf[i] = st_init;
    }
    Meta *gamma1 = alloc_tiles(2);
    gamma1->quad = st_init.quad;
    gamma1->num_children = 0;
    Meta *gamma2 = gamma1 + 1;
    gamma2->quad = st_init.quad;
    gamma2->num_children = 0;
    add_child(&(buf[TT_GAMMA]), gamma1, &IDENTITY);
    XForm trans;
    mk_translation(SPECTRE_POINTS[8],  &trans);
    XForm rot;
    mk_rotation(PI_6, &rot);
    XForm xform;
    compose(&trans, &rot, &xform);
    add_child(&(buf[TT_GAMMA]), gamma2, &xform);
    return buf;
}


/*
 use the production rules on the current tiles to make even more tiles
 */
Meta* build_super_tiles(Meta *current_tiles) {
    /*
       first init a list of transformations based on a reference tile
     */
     
    Quad *reference_quad = &(current_tiles[TT_DELTA].quad);
    
    XForm transforms[TT_COUNT];
    transforms[0] = IDENTITY;
    float angle = 0.f;
    XForm rotation = IDENTITY;
    for (size_t i=0;i<N_XFORMRULES;++i) {
        Quad transformed_quad;
        memcpy(&transformed_quad, reference_quad, sizeof(transformed_quad));
        float xform_angle = XFORMRULES[i].angle;
        size_t xform_from_idx = XFORMRULES[i].from_idx;
        size_t xform_to_idx = XFORMRULES[i].to_idx;
        if (0.f != xform_angle) {
            angle += xform_angle;
            if (xform_angle != 0.f) {
                mk_rotation(angle, &rotation);
                for (size_t j=0;j<4;++j) {
                    transformed_quad.raw[i] = xform_point(
                        &rotation,
                        reference_quad->raw[i]
                    );
                }
            }
            
            Float2 tmp = xform_point(&(transforms[i]), transformed_quad.raw[xform_from_idx]);
            XForm translation;
            mk_translation_to(
                transformed_quad.raw[xform_to_idx],
                tmp,
                &translation);
            XForm rotated;
            compose(&translation, &rotation, &rotated);
            XForm flipped;
            compose(&FLIP, &rotated, &flipped);
            memcpy(&(transforms[i+1]), &flipped, sizeof(transforms[0]));
        }
    }
    
    /*
       now build the new tiles and add to their category
     */
    Quad new_quad = {{
        xform_point(&(transforms[6]), reference_quad->c),
        xform_point(&(transforms[5]), reference_quad->b),
        xform_point(&(transforms[3]), reference_quad->c),
        xform_point(&(transforms[0]), reference_quad->b)
    }};
    
    Meta *new_tiles = alloc_tiles(TT_COUNT);
    
    for (size_t i=0;i<TT_COUNT;++i) {
        for (size_t j=0;j<TT_COUNT;++j) {
            TileType sub_type = SUBS_RULES[i][j];
            if (sub_type == TT_INVALID) { continue; }
            add_child(
                &(new_tiles[i]),
                &(current_tiles[sub_type]),
                &(transforms[j])
            );
        }
        new_tiles[i].quad = new_quad;
    }
    return new_tiles;
}


/* write a single (meta) tile as SVG */
void write_tile(FILE *f, Meta *tile) {
    /* TODO */
}

/* write a batch of tiles as SVG */
void write_tiles(FILE *f, Meta *tiles, size_t count) {
    Meta* end = tiles + count;
    for (Meta*p=tiles;p<end;++p) {
        write_tile(f, p);
    }
}


/* entry point */
int main(int argc, const char **argv) {
    Meta *tiles = init_meta_tiles();
    for (size_t i=0;i<N_ITERATIONS;++i) {
        Meta *backup = tiles;
        tiles = build_super_tiles(tiles);
        free(backup);
    }

    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"", OUT_PATH);
        return 1;
    }

    begin_svg(f, WIDTH, HEIGHT);
    fprintf(f, "\n<rect x=\"0\" y=\"0\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\"/>", WIDTH, HEIGHT, BACKGROUND_COLOR);
    write_tiles(f, tiles, TT_COUNT);
    end_svg(f);
    fclose(f);
    free(tiles);

    return 0;
}
