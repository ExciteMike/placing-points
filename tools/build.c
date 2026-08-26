/*
 * compile with `gcc ./tools/build.c -o build.exe`
 *
 * expects to be run in the main project directory
 */
#include <stdio.h>
#include "raylib.h"

int main(int argc, char ** argv) {
    FilePathList list = LoadDirectoryFiles(".");
    for (int i=0; i<list.count; ++i) {
        const char* path = list.paths[i];
        printf("found %s\n", path);
    }
    return 0;
}