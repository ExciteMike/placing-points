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

void get_base_name(const char *path, char *buf, size_t bufsize); // get basename from a path

// entry point
int main(int argc, char ** argv) {
    const char* err_str = NULL;
    if (!load_config(CONFIG_PATH, &err_str, &CONFIG)) {
        fprintf(stderr, "Failed to load config: %s", err_str);
    }
    
    char buf[512];
    memset(buf, 0, sizeof(buf));
    
    FilePathList list = LoadDirectoryFiles("./src");
    int errors = 0;
    for (int i=0; i<list.count; ++i) {
        const char* path = list.paths[i];
        if (IsFileExtension(path, ".c") && IsPathFile(path)) {
            char basename[256] = {'\0'};
            get_base_name(path, basename, sizeof(basename));
            char outfile[256] = {'\0'};
            int num_written = snprintf(outfile, sizeof(outfile), "./bin/%s.exe", basename);
            if (num_written==256) {
                outfile[255] = '\0';
            }
            long src_m_time = GetFileModTime(path);
            long output_m_time = GetFileModTime(outfile);
            if (src_m_time <= output_m_time) {
                continue;
            }
            
            num_written = snprintf(buf, 512, "%s %s -o %s %s", CONFIG.gcc_path, path, outfile, CONFIG.gcc_opts);
            if (num_written == 512) {
                buf[511] = '\0';
            }
            printf("building %s\n%s", outfile, buf);
            int exit_code = run(buf);
            if (exit_code) {
                printf("build error for \"%s\"", path);
                errors += 1;
            }
        }
    }
    UnloadDirectoryFiles(list);
    return errors;
}

// get basename from a path
void get_base_name(const char *path, char *buf, size_t bufsize) {
    const char *start = path;
    char *last_fwd_slash = strrchr(start, '/');
    if (NULL != last_fwd_slash) {
        start = last_fwd_slash+1;
    }
    const char *last_back_slash = strrchr(start, '\\');
    if (NULL != last_back_slash) {
        start = last_back_slash+1;
    }
    const char *end = start + strlen(start);
    const char *last_dot = strrchr(start, '.');
    if (NULL != last_dot) {
        end = last_dot;
    }
    strncpy_s(buf, bufsize, start, end - start);
}