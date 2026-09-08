/* coliop.h - Mike Meyer 2026

   Helper functions for command-line arguments and options.
   
   This is a single-header library in the style of "stb" libraries.
   
   To use this library, do this in *one* C or C++ file:
      #define COLIOP_IMPLEMENTATION
      #include "coliop.h"
*/

#include <stdlib.h>
#include <stdio.h>

// how to interpret the option's value
typedef enum ColiopOptType {
    COLIOP_OPTTYPE_NOVALUE,
    COLIOP_OPTTYPE_STRING,
    COLIOP_OPTTYPE_INT,
    COLIOP_OPTTYPE_FLOAT,
} ColiopOptType;

typedef struct ColiopOption {
    void *destination;
    const char* descrip;
    const char* name;
    ColiopOptType type;
    char letter;
} ColiopOption;

// configuration for coliop
typedef struct ColiopConfig {
    const char* descrip; // description for help message
    ColiopOption *options; // options to parse
    size_t num_options; // how many options in the `options` field
    size_t max_positional_args; // limit to how many positional args the program accepts
} ColiopConfig;

// results of coliop_execute. must be freed with coliop_free_results
typedef struct ColiopResult {
    int success;
    size_t num_positional_args;
    const char *positional_args[ ];
} ColiopResult;


#ifdef __cplusplus
extern "C" {
#endif
extern ColiopResult *coliop_execute(ColiopConfig *config, int argc, const char **argv, FILE* output_stream); // process the command line
extern void coliop_free_results(ColiopResult*); // free resources
#ifdef __cplusplus
}
#endif


#ifdef COLIOP_IMPLEMENTATION


void coliop_print_help(
    const ColiopConfig *config,
    const char* name,
    FILE* output_stream
) {
    fprintf(output_stream, "USAGE: %s [OPTIONS] ...args\n", name);
    const ColiopOption *end_option = config->options + config->num_options;
    int longest = 0;
    for (const ColiopOption *option= config->options; option!=end_option; option++) {
        int len = strlen(option->name);
        if (len > longest) { longest = len; }
    }
    int descrip_start = longest + 18;
    
    fprintf(output_stream, "Options:\n");
    fprintf(output_stream, "  -h/--help       Show this message.");
    for (const ColiopOption *option= config->options; option!=end_option; option++) {
        int column = fprintf(output_stream, "  -%c/--%s", option->letter, option->name);
        switch (option->type) {
            case COLIOP_OPTTYPE_NOVALUE:
                break;
            case COLIOP_OPTTYPE_STRING:
                column += fprintf(output_stream, " <STRING> ");
                break;
            case COLIOP_OPTTYPE_INT:
                column += fprintf(output_stream, " <INTEGER> ");
                break;
            case COLIOP_OPTTYPE_FLOAT:
                column += fprintf(output_stream, " <NUMBER> ");
                break;
            default:
                break;
        }
        while (column++ < descrip_start) {
            fprintf(output_stream, " ");
        }
        // TODO: wordwrap
        fprintf(output_stream, "%s\n", option->descrip);
    }
}

// process the command line
ColiopResult *coliop_execute(
    ColiopConfig *config,
    int argc,
    const char **argv,
    FILE* output_stream
) {
    const char* name = argv[0]; // program name for usage message
    int i = 1; // index of current arg
    size_t pos_args_cap = 0; // how much room we have allocated to store positional args
    int allow_options = 1; // `--` is a magic arg that means we stop processing more options
    
    ColiopResult *result = calloc(1, sizeof(ColiopResult));
    if (NULL == result) {
        return NULL;
    }
    
    while (i < argc) {
        const char *cur_arg = argv[i++];
        if (allow_options && (cur_arg[0] == '-')) {
            
            // option
            if (0==strcmp("--", cur_arg)) {
                // stop processing further options
                allow_options = 0;
            } else if ((0==strcmp("-h", cur_arg)) || (0==strcmp("--help", cur_arg))) {
                fprintf(output_stream, "%s\n", config->descrip);
                coliop_print_help(config, name, output_stream);
                return calloc(1, sizeof(ColiopResult));
            } else {
                const ColiopOption *end_option = config->options + config->num_options;
                int option_handled = 0;
                for (ColiopOption *option= config->options; option!=end_option; option++) {
                    int is_letter = (cur_arg[1] == option->letter) && (cur_arg[2] == '\0');
                    int is_name = (cur_arg[1] == '-') && (0 == strcmp(cur_arg+2, option->name));
                    if (is_letter || is_name) {
                        option_handled = 1;
                        switch (option->type) {
                            case COLIOP_OPTTYPE_NOVALUE:
                                break;
                            case COLIOP_OPTTYPE_STRING:
                                if (i < argc) {
                                    const char *next_arg = argv[i++];
                                    const char** destination = option->destination;
                                    *destination = next_arg;
                                } else {
                                    fprintf(output_stream, "expected a (string) value after %s\n", cur_arg);
                                    coliop_print_help(config, name, output_stream);
                                    return result;
                                }
                                break;
                            case COLIOP_OPTTYPE_INT:
                                if (i < argc) {
                                    const char *next_arg = argv[i++];
                                    int* destination = option->destination;
                                    *destination = atoi(next_arg);
                                } else {
                                    fprintf(output_stream, "expected a (integer) value after %s\n", cur_arg);
                                    coliop_print_help(config, name, output_stream);
                                    return result;
                                }
                                break;
                            case COLIOP_OPTTYPE_FLOAT:
                                if (i < argc) {
                                    const char *next_arg = argv[i++];
                                    float* destination = option->destination;
                                    *destination = strtof(next_arg, NULL);
                                } else {
                                    fprintf(output_stream, "expected a (number) value after %s\n", cur_arg);
                                    coliop_print_help(config, name, output_stream);
                                    return result;
                                }
                                break;
                            default:
                                fprintf(
                                    output_stream,
                                    "Unhandled case. type=%d for %s\n",
                                    option->type,
                                    cur_arg
                                );
                                coliop_print_help(config, name, output_stream);
                                return result;
                                break;
                        }
                        break;
                    }
                }
                
                // none found
                if (!option_handled) {
                    fprintf(output_stream, "Unrecognized option \"%s\"\n", cur_arg);
                    coliop_print_help(config, name, output_stream);
                    return result;
                }
            }
            
        } else {
            
            // positional argument
            if (result->num_positional_args >= config->max_positional_args) {
                fprintf(output_stream, "error parsing command-line args: too many positional arguments (max = %zu)\n", config->max_positional_args);
                coliop_print_help(config, name, output_stream);
                return result;
            }
            
            if (result->num_positional_args >= pos_args_cap) {
                pos_args_cap = (pos_args_cap==0) ? 1 : pos_args_cap * 2;
                ColiopResult *backup = result;
                result = realloc(
                    result,
                    sizeof(ColiopResult) + pos_args_cap * sizeof(result->positional_args[0])
                );
                if (NULL == result) {
                    fprintf(output_stream, "error parsing command-line args: allocation failure\n");
                    free(backup);
                    return result;
                }
            }
            result->positional_args[result->num_positional_args++] = cur_arg;
        }
    }
    
    result->success = 1;
    return result;
}

// free resources
void coliop_free_results(ColiopResult *ptr) {
    free(ptr);
}

#endif // COLIOP_IMPLEMENTATION