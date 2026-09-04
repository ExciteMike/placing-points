/*
   Generate svg showing how a a blue-noise-like distribution can be constructed from an aperiodic tiling.
   
   It uses the spectre tiling from
      Smith, D., Myers, J. S, Kaplan, C. S, & Goodman-Strauss, C. (2024). A chiral aperiodic monotile. Combinatorial Theory, 4(2). http://dx.doi.org/10.5070/C64264241
   
   Code based on the Javascript implementation by those authors found at https://cs.uwaterloo.ca/~csk/spectre/spectre.js

   Note: In the interest of making port form JS straightforward, this code
   intentionally leaks memory, allocates with unreasonable frequency, makes a
   lot of unnecessary copies, and is bad about C best practices and coding
   style.
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define PI   (3.1415926535897932384626433832795f)
#define PI_OVER_3 (1.0471975511965977461542144610932f)
#define PI_OVER_6 (0.52359877559829887307710723054658f)
#define DEG_TO_RAD (0.01745329251994329576923690768489f)


/* Begin enums */


enum {
/** number of vertices in the spectre geometry */
    N_SPECTRE_VERTICES = 14
};


/** child tiles can be either a drawable shape or a meta tile */
typedef enum ChildKind {
    CK_NONE,
    CK_META,
    CK_SHAPE
} ChildKind;


/** tile type for color purposes */
typedef enum TileColor {
    TC_FIRST = 0,
    TC_GAMMA1 = 0,
    TC_GAMMA2,
    TC_DELTA,
    TC_THETA,
    TC_LAMBDA,
    TC_XI,
    TC_PI,
    TC_SIGMA,
    TC_PHI,
    TC_PSI,
    TC_COUNT,
    TC_END = TC_COUNT
} TileColor;


/** types of tiles */
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


/* End enums */


/* Begin types */


/** matrix representation of affine transformations */
typedef union XForm {
    float raw[6];
    struct {
        float e0;
        float e1;
        float e2;
        float e3;
        float e4;
        float e5;
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


/** 2d point */
typedef union Pt {
    float raw[2];
    struct {
        float x;
        float y;   
    };
} Pt;


/** used to build the transforms as we iterate */
typedef union Quad {
    Pt raw[4];
    struct {
        Pt e0;
        Pt e1;
        Pt e2;
        Pt e3;
    };
} Quad;


/** parts of a metatile */
typedef struct Child {
    ChildKind kind;
    XForm xform;
    Quad quad;
    union {
        struct {
            struct Child *geoms;
            size_t n_geoms;
        } meta;
        struct {
            TileColor color;
        } shape;
    };
} Child;


/** the type of tile to generate for each source tile type */
typedef struct SuperRulesRow {
    TileType raw[8];
} SuperRulesRow;
enum {SUPER_RULES_ROW_LEN = sizeof(SuperRulesRow) / sizeof(TileType)};


/** current tiles system */
typedef struct Sys {
    Child tiles[TT_COUNT];
} Sys;


/** transformation rules when producing subtiles */
typedef struct TRule {
    /** in degrees */
    float angle;
    size_t from;
    size_t to;
} TRule;


/* End types */


/* begin forward declarations */


static XForm ttrans(float tx, float ty);


/* end forward declarations */


/* begin globals */


/** map the tile color enum to color strings */
const char* colmap[TC_COUNT] = {
    /*Gamma1*/ "#613915", 
    /*Gamma2*/ "#000000",
    /*Delta*/ "#028121",
    /*Theta*/ "#004CFF",
    /*Lambda*/ "760088",
    /*Xi*/ "#E50000",
    /*Pi*/ "#FFAFC7",
    /*Sigma*/ "#73D7EE",
    /*Phi*/ "#FF8D00",
    /*Psi*/ "#FFEE00" };


/** x-axis flip transfrom */
const XForm flip = {{-1.f, 0.f, 0.f, 0.f, 1.f, 0.f}};


/** do-nothing transfrom */
const XForm ident = {{1, 0, 0, 0, 1, 0}};


/** where to save the file */
static const char *OUT_PATH = "./dist/aperiodic.svg";


/** geometry of the spectre tile */
const Pt SPECTRE[N_SPECTRE_VERTICES] = {
        {{0, 0}},
        {{1.0, 0.0}},
        {{1.5, -0.8660254037844386}},
        {{2.366025403784439, -0.36602540378443865}},
        {{2.366025403784439, 0.6339745962155614}},
        {{3.366025403784439, 0.6339745962155614}},
        {{3.866025403784439, 1.5}},
        {{3.0, 2.0}},
        {{2.133974596215561, 1.5}},
        {{1.6339745962155614, 2.3660254037844393}},
        {{0.6339745962155614, 2.3660254037844393}},
        {{-0.3660254037844386, 2.3660254037844393}},
        {{-0.866025403784439, 1.5}},
        {{0.0, 1.0}},
    };


const SuperRulesRow super_rules[] = {
    /*Gamma*/  {{ TT_PI, TT_DELTA, TT_INVALID, TT_THETA, TT_SIGMA,  TT_XI,    TT_PHI, TT_GAMMA}},
    /*Delta*/  {{ TT_XI, TT_DELTA,      TT_XI, TT_PHI,   TT_SIGMA,  TT_PI,    TT_PHI, TT_GAMMA}},
    /*Theta*/  {{TT_PSI, TT_DELTA,      TT_PI, TT_PHI,   TT_SIGMA,  TT_PI,    TT_PHI, TT_GAMMA}},
    /*Lambda*/ {{TT_PSI, TT_DELTA,      TT_XI, TT_PHI,   TT_SIGMA,  TT_PI,    TT_PHI, TT_GAMMA}},
    /*Xi*/     {{TT_PSI, TT_DELTA,      TT_PI, TT_PHI,   TT_SIGMA, TT_PSI,    TT_PHI, TT_GAMMA}},
    /*Pi*/     {{TT_PSI, TT_DELTA,      TT_XI, TT_PHI,   TT_SIGMA, TT_PSI,    TT_PHI, TT_GAMMA}},
    /*Sigma*/  {{ TT_XI, TT_DELTA,      TT_XI, TT_PHI,   TT_SIGMA,  TT_PI, TT_LAMBDA, TT_GAMMA}},
    /*Phi*/    {{TT_PSI, TT_DELTA,     TT_PSI, TT_PHI,   TT_SIGMA,  TT_PI,    TT_PHI, TT_GAMMA}},
    /*Psi*/    {{TT_PSI, TT_DELTA,     TT_PSI, TT_PHI,   TT_SIGMA, TT_PSI,    TT_PHI, TT_GAMMA}}
    };
enum {N_SUPER_RULES = 8};


/** transform to fit things in the view window */
const XForm to_screen = {{3, 0, 0, 0, -3, -70}};


/** map the tile type enum to tile color enum */
const TileColor TT_TO_TC[TT_COUNT] = {
    /*TT_GAMMA*/ TC_GAMMA1,
    /*TT_DELTA*/ TC_DELTA,
    /*TT_THETA*/ TC_THETA,
    /*TT_LAMBDA*/ TC_LAMBDA,
    /*TT_XI*/ TC_XI,
    /*TT_PI*/ TC_PI,
    /*TT_SIGMA*/ TC_SIGMA,
    /*TT_PHI*/ TC_PHI,
    /*TT_PSI*/ TC_PSI,
};


/** transformation rules */
const TRule t_rules[] = {
    {60.f, 3, 1},
    {0.f, 2, 0},
    {60.f, 3, 1},
    {60.f, 3, 1},
    {0.f, 2, 0},
    {60.f, 3, 1},
    {-120.f, 3, 3},
};
enum {N_T_RULES = sizeof(t_rules) / sizeof(t_rules[0])};


/* end globals */


/** add more geometry to a metatile */
static void add_child(Child *this, Child g) {
    if (this->kind != CK_META) {
        fprintf(stderr, "Adding child to non-meta tile\n");
        exit(1);
    }
    this->meta.geoms = realloc(this->meta.geoms, (this->meta.n_geoms+1) * sizeof(this->meta.geoms[0]));
    if (NULL == this->meta.geoms) {
        fprintf(stderr, "alloc failed\n");
        exit(1);
    }
    this->meta.geoms[this->meta.n_geoms++] = g;
}


/** build a Meta */
static Child meta(Quad quad) {
    return (Child) {
        .kind = CK_META,
        .xform = ident,
        .quad = quad,
        .meta = {.geoms=NULL,.n_geoms=0}
    };
}


/** Affine matrix multiply */
static XForm mul(XForm A, XForm B) {
    return (XForm) {{
        A.e0*B.e0 + A.e1*B.e3, 
        A.e0*B.e1 + A.e1*B.e4,
        A.e0*B.e2 + A.e1*B.e5 + A.e2,

        A.e3*B.e0 + A.e4*B.e3, 
        A.e3*B.e1 + A.e4*B.e4,
        A.e3*B.e2 + A.e4*B.e5 + A.e5
    }};
}


/** convert degrees to radians */
static float radians(float deg) { return deg * DEG_TO_RAD; }


/**
  Used in the curved-shape code to make bezier control points.
  I don't understand the name - Mike
 */
Pt pframe(Pt o, Pt p, Pt q, float a, float b) {
    return (Pt) { .x = o.x + a*p.x + b*q.x, .y = o.y + a*p.y + b*q.y };
}


/** vector math */
Pt psub(Pt p, Pt q) {
    return (Pt) { .x = p.x - q.x, .y = p.y - q.y };
}


/** terse way to construct point */
static Pt pt(float x, float y) {
    return (Pt) {.x=x,.y=y};
}


/** make a shape */
static Child shape(Quad quad, XForm xform, TileColor color) {
    return (Child) {
        .kind = CK_SHAPE,
        .xform = xform,
        .quad = quad,
        .shape = {.color=color}
    };
}


/** transform a point */
static Pt transPt(XForm M, Pt P) {
    return pt(M.e0*P.x + M.e1*P.y + M.e2, M.e3*P.x + M.e4*P.y + M.e5);
}


/** Translation matrix from p to q */
static XForm transTo(Pt p, Pt q) {
    return ttrans( q.x - p.x, q.y - p.y );
}


/** Rotation matrix */
static XForm trot(float ang) {
    const float c = cosf( ang );
    const float s = sinf( ang );
    return (XForm) {{ c, -s, 0.f, s, c, 0.f }};
}


/** Translation matrix */
static XForm ttrans(float tx, float ty) {
    return (XForm) {{1.f, 0.f, tx, 0.f, 1.f, ty}};
}


static void debug_print_xform(const char* label, XForm xform) {
    printf("%s:\n  %.4f %.4f %.4f\n  %.4f %.4f %.4f\n", label, xform.e0, xform.e1, xform.e2, xform.e3, xform.e4, xform.e5);
}


/** write out SVG for the thing */
static void write_child(Child *this, FILE *f, XForm S) {
    XForm combined = mul(S, this->xform);
    if (this->kind == CK_META) {
        for (size_t i=0; i<this->meta.n_geoms; ++i) {
            Child *child = &(this->meta.geoms[i]);
            write_child(child, f, combined);
        }
    } else if (this->kind == CK_SHAPE) {
        fprintf(f, "\n<polygon points=\"");
        for (size_t i=0;i<N_SPECTRE_VERTICES;++i) {
            const Pt sp = transPt(combined, SPECTRE[i]);
            if (i!=0) {
                fprintf(f, " ");
            }
            fprintf(f, "%f,%f", sp.x, sp.y);
        }
        const char* col = colmap[this->shape.color];
        fprintf(f, "\" stroke=\"black\" stroke-weight=\"0.1\" fill=\"%s\" />", col);
    } else {
        // nothing to do for CK_NONE
    }
}


/** prepare the initial metatiles */
static Sys buildSpectreBase() {
    const Quad spectre_keys = {{
        SPECTRE[3], SPECTRE[5], SPECTRE[7], SPECTRE[11]
    }};

    Sys ret = {0};

    for(TileType tile_type=TT_FIRST;tile_type<TT_END;++tile_type) {
        ret.tiles[tile_type] = (Child) {
            .kind = CK_SHAPE,
            .xform = ident,
            .quad = spectre_keys,
            .shape = {.color=TT_TO_TC[tile_type]}
        };
    }

    Child mystic = meta(spectre_keys);
    add_child(&mystic, shape(spectre_keys, ident, TC_GAMMA1));
    XForm xform = mul( ttrans(SPECTRE[8].x, SPECTRE[8].y), trot( PI_OVER_6 ) );
    add_child(&mystic, shape(spectre_keys, xform, TC_GAMMA2));
    ret.tiles[TT_GAMMA] = mystic;

    return ret;
}


static Sys buildSupertiles( Sys sys ) {
    /* arbitrarily choosing delta to create the list of xforms */
    const Quad quad = sys.tiles[TT_DELTA].quad;
    XForm Ts[N_T_RULES+1] = {
        ident,
        ident,
        ident,
        ident,
        ident,
        ident,
        ident,
        ident
    };
    size_t ts_length = 1;
    float total_ang = 0;
    XForm rot = ident;
    Quad tquad = quad;
    for (size_t rule_idx=0;rule_idx<N_T_RULES;++rule_idx) {
        float ang = t_rules[rule_idx].angle;
        size_t from = t_rules[rule_idx].from;
        size_t to = t_rules[rule_idx].to;
        total_ang += ang;
        if( ang != 0 ) {
            rot = trot( radians( total_ang ) );
            for(size_t i=0;i<4;++i) {
                tquad.raw[i] = transPt( rot, quad.raw[i] );
            }
        }

        XForm ttt = transTo( tquad.raw[to], 
            transPt( Ts[ts_length-1], quad.raw[from] ) );
        Ts[ts_length++] = mul( ttt, rot );
    }

    for(size_t idx = 0; idx < ts_length; ++idx ) {
        Ts[idx] = mul( flip, Ts[idx] );
    }

    /* Now build the actual supertiles, labelling appropriately. */
    Quad super_quad = {{
        transPt( Ts[6], quad.e2 ),
        transPt( Ts[5], quad.e1 ),
        transPt( Ts[3], quad.e2 ),
        transPt( Ts[0], quad.e1 ) }}; 

    Sys ret = {0};

    for (size_t tile_type=TT_FIRST;tile_type<TT_COUNT;++tile_type) {
        SuperRulesRow subs = super_rules[tile_type];
        Child sup = meta(super_quad);
        for (size_t idx=0;idx<SUPER_RULES_ROW_LEN;++idx) {
            TileType tt = subs.raw[idx];
            if (tt == TT_INVALID) {
                continue;
            }
            sys.tiles[tt].xform = Ts[idx];
            add_child(&sup, sys.tiles[tt]);
        }

        ret.tiles[tile_type] = sup;
    }

    return ret;
}


int main() {
    const size_t ITERATIONS = 4;
    const TileType START_TILE = TT_GAMMA;
    const float width = 240;
    const float height = 180;
    Sys sys = buildSpectreBase();
    for (size_t i=0;i<ITERATIONS;++i){
        sys = buildSupertiles(sys);
    }
    
    FILE *f = fopen(OUT_PATH, "w");
    if (NULL == f) {
        fprintf(stderr, "could not open output file \"%s\"\n", OUT_PATH);
        return 1;
    }

    fprintf(
        f,
        "<svg width=\"%f\" height=\"%f\" viewBox=\"0 0 %f %f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">",
        width,
        height,
        width,
        height
    );
    fprintf(f, "\n<g>");
    write_child(&sys.tiles[START_TILE], f, to_screen);
    fprintf(f, "\n</g>");
    fprintf(f, "\n</svg>");
}
