/* Windows-specific implementations for the build program */
#include <stdlib.h>

int run(const char* command) {
    /* `system` is not actually windows-specific, but I will probably want to switch over to 
       Windows's `CreateProcess` or similar at some point */
    return system(command);
}