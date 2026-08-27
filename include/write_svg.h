// Helper functions for writing out scalable vector graphics.
// 
// Single-file library in the style of "stb" libraries.
// To create the implementation,
//     #define WRITE_SVG_IMPLEMENTATION
// in *one* C/CPP file that includes this file.

#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void begin_svg(FILE *file, float width, float height); // write out the beginning of an svg file. use end_svg to finish.
extern void end_svg(FILE *file); // write out the last bit of an svg file.
extern void write_points(FILE *file, Vector2 *points, size_t count); // draw circles for each point
#ifdef __cplusplus
}
#endif


#ifdef WRITE_SVG_IMPLEMENTATION


void begin_svg(FILE *file, float width, float height) {
    fprintf(file, "<svg width=\"%.2f\" height=\"%.2f\">", width, height);
}

void end_svg(FILE *file) {
    fprintf(file, "</svg>");
}

void write_points(FILE *file, Vector2 *points, size_t count); // draw circles for each point

#endif // WRITE_SVG_IMPLEMENTATION