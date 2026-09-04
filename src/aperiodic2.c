/*
   Generate svg showing how a a blue-noise-like distribution can be constructed from an aperiodic tiling.
   
   It uses the spectre tiling from
      Smith, D., Myers, J. S, Kaplan, C. S, & Goodman-Strauss, C. (2024). A chiral aperiodic monotile. Combinatorial Theory, 4(2). http://dx.doi.org/10.5070/C64264241
   
   Code based on the Javascript implementation by those authors found at https://cs.uwaterloo.ca/~csk/spectre/spectre.js
 */
#include <math.h>
#include <stdio.h>

#define PI   (3.1415926535897932384626433832795f)
#define PI_3 (1.0471975511965977461542144610932f)
#define DEG_TO_RAD (0.01745329251994329576923690768489f)

/* Begin enums */

enum {
/** number of vertices in the spectre geometry */
    N_SPECTRE_VERTICES = 14
};

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


/** 2d point */
typedef struct Pt {
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


/** leaf type in our metatile tree */
typedef struct Shape {
    Quad quad;
    TileColor color;
} Shape;


/** current tiles system */
typedef struct Sys {

} Sys;


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


/* End types */


/* begin forward declarations */

XForm ttrans(float tx, float ty);

/* end forward declarations */


/* begin globals */


/** do-nothing transfrom */
const XForm ident     = {{1, 0, 0, 0, 1, 0}};


/** transform to fit things in the view window */
const XForm to_screen = {{5, 0, 0, 0, -5, 0}};


/** map the tile color enum to coor strings */
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


/* end globals */


/** Affine matrix multiply */
XForm mul(XForm A, XForm B) {
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
float radians(float deg) { return deg * DEG_TO_RAD; }


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
Pt pt(float x, float y) {
    return (Pt) {.x=x,.y=y};
}


/** make a shape */
Shape shape(Quad quad, TileColor color) {
    return (Shape) {.quad=quad,.color=color};
}


/** transform a point */
Pt transPt(XForm M, Pt P) {
    return pt(M.e0*P.x + M.e1*P.y + M.e2, M.e3*P.x + M.e4*P.y + M.e5);
}


/** Translation matrix from p to q */
XForm transTo(Pt p, Pt q) {
    return ttrans( q.x - p.x, q.y - p.y );
}


/** Rotation matrix */
XForm trot(float ang) {
    const float c = cosf( ang );
    const float s = sinf( ang );
    return (XForm) {{ c, -s, 0.f, s, c, 0.f }};
}


/** Translation matrix */
XForm ttrans(float tx, float ty) {
    return (XForm) {{1.f, 0.f, tx, 0.f, 1.f, ty}};
}


/** write out SVG for the shape */
void write_shape(Shape *this, FILE* f, XForm S) {
    fprintf(f, "\n<polygon points=\"");
    for (size_t i=0;i<N_SPECTRE_VERTICES;++i) {
        const Pt sp = transPt(S, SPECTRE[i]);
        if (i!=0) {
            fprintf(f, " ");
        }
        fprintf(f, "%f,%f", sp.x, sp.y);
    }
    const char* col = colmap[this->color];
    fprintf(f, "\" stroke=\"black\" stroke-weight=\"0.1\" fill=\"%s\" />", col);
}


class Meta
{
    constructor()
    {
        this.geoms = [];
        this.quad = [];
    }

    addChild( g, T )
    {
        this.geoms.push( { geom : g, xform: T } );
    }

    printSvg( S )
    {
        for( let g of this.geoms ) {
            g.geom.printSvg( mul( S, g.xform ) );
        }
    }
}

function buildSpectreBase( curved )
{
    const spectre = [
        pt(0, 0),
        pt(1.0, 0.0),
        pt(1.5, -0.8660254037844386),
        pt(2.366025403784439, -0.36602540378443865),
        pt(2.366025403784439, 0.6339745962155614),
        pt(3.366025403784439, 0.6339745962155614),
        pt(3.866025403784439, 1.5),
        pt(3.0, 2.0),
        pt(2.133974596215561, 1.5),
        pt(1.6339745962155614, 2.3660254037844393),
        pt(0.6339745962155614, 2.3660254037844393),
        pt(-0.3660254037844386, 2.3660254037844393),
        pt(-0.866025403784439, 1.5),
        pt(0.0, 1.0) 
    ];

    const spectre_keys = [
        spectre[3], spectre[5], spectre[7], spectre[11]
    ];

    const ret = {};

    for( lab of ['Delta', 'Theta', 'Lambda', 'Xi', 
                 'Pi', 'Sigma', 'Phi', 'Psi'] ) {
        if( curved ) {
            ret[lab] = new CurvyShape( spectre, spectre_keys, lab );
        } else {
            ret[lab] = new Shape( spectre, spectre_keys, lab );
        }
    }

    const mystic = new Meta();
    if( curved ) {
        mystic.addChild( 
            new CurvyShape( spectre, spectre_keys, 'Gamma1' ), ident );
        mystic.addChild( 
            new CurvyShape( spectre, spectre_keys, 'Gamma2' ),
                mul( ttrans( spectre[8].x, spectre[8].y ), trot( PI / 6 ) ) );
    } else {
        mystic.addChild( new Shape( spectre, spectre_keys, 'Gamma1' ), ident );
        mystic.addChild( new Shape( spectre, spectre_keys, 'Gamma2' ),
            mul( ttrans( spectre[8].x, spectre[8].y ), trot( PI / 6 ) ) );
    }
    mystic.quad = spectre_keys;
    ret['Gamma'] = mystic;

    return ret;
}


function buildSupertiles( sys )
{
    // First, use any of the nine-unit tiles in sys to obtain
    // a list of transformation matrices for placing tiles within
    // supertiles.

    const quad = sys['Delta'].quad;
    const R = [-1,0,0,0,1,0];
    
    const t_rules = [
        [60, 3, 1], [0, 2, 0], [60, 3, 1], [60, 3, 1],
        [0, 2, 0], [60, 3, 1], [-120, 3, 3] ];  

    const Ts = [ident];
    let total_ang = 0;
    let rot = ident;
    const tquad = [...quad];
    for( const [ang,from,to] of t_rules ) {
        total_ang += ang;
        if( ang != 0 ) {
            rot = trot( radians( total_ang ) );
            for( i = 0; i < 4; ++i ) {
                tquad[i] = transPt( rot, quad[i] );
            }
        }

        const ttt = transTo( tquad[to], 
            transPt( Ts[Ts.length-1], quad[from] ) );
        Ts.push( mul( ttt, rot ) );
    }

    for( let idx = 0; idx < Ts.length; ++idx ) {
        Ts[idx] = mul( R, Ts[idx] );
    }

    // Now build the actual supertiles, labelling appropriately.
    const super_rules = {
        'Gamma' :  ['Pi','Delta','null','Theta','Sigma','Xi','Phi','Gamma'],
        'Delta' :  ['Xi','Delta','Xi','Phi','Sigma','Pi','Phi','Gamma'],
        'Theta' :  ['Psi','Delta','Pi','Phi','Sigma','Pi','Phi','Gamma'],
        'Lambda' : ['Psi','Delta','Xi','Phi','Sigma','Pi','Phi','Gamma'],
        'Xi' :     ['Psi','Delta','Pi','Phi','Sigma','Psi','Phi','Gamma'],
        'Pi' :     ['Psi','Delta','Xi','Phi','Sigma','Psi','Phi','Gamma'],
        'Sigma' :  ['Xi','Delta','Xi','Phi','Sigma','Pi','Lambda','Gamma'],
        'Phi' :    ['Psi','Delta','Psi','Phi','Sigma','Pi','Phi','Gamma'],
        'Psi' :    ['Psi','Delta','Psi','Phi','Sigma','Psi','Phi','Gamma'] };
    const super_quad = [
        transPt( Ts[6], quad[2] ),
        transPt( Ts[5], quad[1] ),
        transPt( Ts[3], quad[2] ),
        transPt( Ts[0], quad[1] ) ]; 

    const ret = {};

    for( const [lab, subs] of Object.entries( super_rules ) ) {
        const sup = new Meta();
        for( let idx = 0; idx < 8; ++idx ) {
            if( subs[idx] == 'null' ) {
                continue;
            }
            sup.addChild( sys[subs[idx]], Ts[idx] );
        }
        sup.quad = super_quad;

        ret[lab] = sup;
    }

    return ret;
}

const ITERATIONS = 1;
sys = buildSpectreBase();
for (let i=0;i<ITERATIONS;++i){
    sys = buildSupertiles(sys);
}
console.log('<svg viewBox="0 0 240 180" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">');
console.log('<g transform="translate(120,90)">');
sys['Gamma'].printSvg(to_screen);
console.log('</g>');
console.log('</svg>');