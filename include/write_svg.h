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
extern void write_point(FILE *file, Vector2 point, float radius, const char *fill_color); // draw circle for a point
extern void write_points(FILE *file, Vector2 *points, size_t count, float radius, const char *fill_color); // draw circles for each point
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
void write_point(FILE *file, Vector2 point, float radius, const char *fill_color) {
    fprintf(file, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\"/>", point.x, point.y, radius, fill_color);
}

// draw circles for each point
void write_points(FILE *file, Vector2 *points, size_t count, float radius, const char *fill_color) {
    for (int i=0; i<count; ++i) {
        write_point(file, points[i], radius, fill_color);
    }
}

#endif // WRITE_SVG_IMPLEMENTATION