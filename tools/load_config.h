#define CONFIG_FIELD_SIZE (256)

typedef struct Config {
    char gcc_path[CONFIG_FIELD_SIZE]; // path to gcc
    char gcc_opts[CONFIG_FIELD_SIZE]; // options to send to gcc
} Config;

/* error messages for config parsing */
const char * ERR_EXPECTED_EQ = "Expected an equals sign after setting name";
const char * ERR_UNRECOGNIZED_SETTING = "Unrecognized setting";


/* read past any whitespace */
void skip_space(const char** read_head) {
    while ((**read_head != '\0') && isspace(**read_head)) {
        ++(*read_head);
    }
}

/* check for the identifier and skip past it, if present */
bool read_exact(const char** read_head, const char* exact) {
    size_t n = strlen(exact);
    if (0 == strncmp(exact, *read_head, n)) {
        (*read_head) += n;
        return true;
    }
    return false;
}

// skip up to and past the next newline
void read_to_end_of_line(const char** read_head, char *dst, size_t count) {
    while (**read_head != '\0') {
        if (**read_head != '\n') {
            ++(*read_head);
            break;
        } else {
            (*dst++) = **read_head;
            ++(*read_head);
        }
    }
}

// Load a string setting from the rest of a config line afterr the setting name.
// If something goes wrong, it will set err_str and return FALSE
bool read_config_string(const char** read_head, const char ** err_str, char *dst, size_t count) {
    skip_space(read_head);
    if (!read_exact(read_head, "=")) {
        *err_str = ERR_EXPECTED_EQ;
        return false;
    }
    skip_space(read_head);
    read_to_end_of_line(read_head, dst, count);
    skip_space(read_head);
    return true;
}

/* load the config file */
bool load_config(const char* file_path, const char ** err_str, Config* config) {
    char* text = LoadFileText(file_path);
    const char* read_head = text;
    bool success = true;
    skip_space(&read_head);
    while (*read_head != '\0') {
        if (read_exact(&read_head, "gcc_path")) {
            if (!read_config_string(&read_head, err_str, &(config->gcc_path[0]), CONFIG_FIELD_SIZE)) {
                return false;
            }
        } else if (read_exact(&read_head, "gcc_opts")) {
            if (!read_config_string(&read_head, err_str, &(config->gcc_opts[0]), CONFIG_FIELD_SIZE)) {
                return false;
            }
        } else {
            *err_str = ERR_UNRECOGNIZED_SETTING;
            return false;
        }
        skip_space(&read_head);
    }
    
    UnloadFileText(text);
    return success;
}

#undef CONFIG_FIELD_SIZE