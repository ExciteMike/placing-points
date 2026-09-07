/* write_svg.h - Mike Meyer 2026

   Helper functions for writing out scalable vector graphics.

   To use this library, do this in *one* C or C++ file:
      #define WRITE_SVG_IMPLEMENTATION
      #include "write_svg.h"
*/

#include <stdio.h>
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif
extern void begin_svg(FILE *file, float width, float height); // write out the beginning of an svg file. use end_svg to finish.
extern void end_svg(FILE *file); // write out the last bit of an svg file.
extern void write_point(FILE *file, Pt p, float radius, const char *fill_color); // draw circle for a point
extern void write_points(FILE *file, Pt *points, size_t count, float radius, const char *fill_color); // draw circles for each point. TWO floats per point (x and y coords), so count must be even
#ifdef __cplusplus
}
#endif


#ifdef WRITE_SVG_IMPLEMENTATION


void begin_svg(FILE *file, float width, float height) {
    fprintf(file, "<svg width=\"%.2f\" height=\"%.2f\" xmlns=\"http://www.w3.org/2000/svg\">", width, height);
}

void end_svg(FILE *file) {
    fprintf(file, "</svg>");
}

// draw circle for a point
void write_point(FILE *file, Pt p, float radius, const char *fill_color) {
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\"/>", p.x, p.y, radius, fill_color);
}

// draw circles for each of `count` points. 
void write_points(
    FILE *file,
    Pt *points,
    size_t count,
    float radius,
    const char *fill_color
) {
    for (size_t i=0; i<count; i++) {
        write_point(file, points[i], radius, fill_color);
    }
}

#endif // WRITE_SVG_IMPLEMENTATION