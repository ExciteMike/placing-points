/*
 * Expects to be run in the main project directory.
 * 
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "build_windows.h"
#include "load_config.h"

/* config file name */
char *CONFIG_PATH = "build_config";

/* config data */
struct Config CONFIG = {0};

int main(int argc, char ** argv) {
    const char* err_str = NULL;
    if (!load_config(CONFIG_PATH, &err_str, &CONFIG)) {
        fprintf(stderr, "Failed to load config: %s", err_str);
    }
    
    char buf[512];
    memset(buf, 0, sizeof(buf));
    
    FilePathList list = LoadDirectoryFiles(".");
    int errors = 0;
    for (int i=0; i<list.count; ++i) {
        const char* path = list.paths[i];
        if (IsFileExtension(path, ".c") && IsPathFile(path)) {
            int num_written = snprintf(buf, 512, "%s %s %s", CONFIG.gcc_path, path, CONFIG.gcc_opts);
            if (num_written == 512) {
                buf[511] = '\0';
            }
            int exit_code = run(buf);
            if (exit_code) {
                printf("build error for \"%s\"", path);
                errors += 1;
            }
        }
    }
    return errors;
}


