#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "arg_parser.h"

typedef enum {
    ARG_ALSA,
    ARG_MINVEL,
    ARG_FILE,
    ARG_UNKNOWN
} ArgType;

typedef struct {
    const char* key;
    ArgType type;
    const char* desc;
} ArgKey;

static const ArgKey known_keys[] = {
    {"alsa",    ARG_ALSA,   "Set ALSA output client:port"},
    {"p",       ARG_ALSA,   "Short alias for --alsa"},

    {"minvel",  ARG_MINVEL, "Set minimum velocity (0-127)"},
    {"mv",      ARG_MINVEL, "Alias for --minvel"},
    {"m",       ARG_MINVEL, "Short alias for --minvel"},
    
    {"file",    ARG_FILE,   "MIDI file to play"},
    {"f",       ARG_FILE,   "Short alias for --file"}
};

static ArgType identify_arg(const char* key) {
    for (size_t i = 0; i < sizeof(known_keys) / sizeof(known_keys[0]); ++i) {
        if (strcmp(key, known_keys[i].key) == 0) {
            return known_keys[i].type;
        }
    }
    return ARG_UNKNOWN;
}

static void print_usage(const char* program_name) {
    printf("Usage: %s [options] -f <midi_file>\n\n", program_name);
    printf("Options:\n");

    int printed[sizeof(known_keys) / sizeof(known_keys[0])] = {0};

    for (size_t i = 0; i < sizeof(known_keys) / sizeof(known_keys[0]); ++i) {
        if (printed[i]) continue;

        ArgType current_type = known_keys[i].type;
        const char* desc = known_keys[i].desc;

        printf("  ");
        for (size_t j = 0; j < sizeof(known_keys) / sizeof(known_keys[0]); ++j) {
            if (known_keys[j].type == current_type && !printed[j]) {
                printf("%s%s%s",
                    strlen(known_keys[j].key) == 1 ? "-" : "--",
                    known_keys[j].key,
                    j < sizeof(known_keys) / sizeof(known_keys[0]) - 1 ? ", " : "");
                printed[j] = 1;
            }
        }
        printf("\n      %s\n", desc);
    }

    printf("\nExamples:\n");
    printf("  %s -f song.mid --alsa=14:0 --minvel=64\n", program_name);
    printf("  %s -p 14:0 -m 10 song.mid\n", program_name);
}

int parse_args(int argc, char* argv[], Options* opts) {
    opts->filename = NULL;
    opts->alsa_port = NULL;
    opts->min_velocity = 1;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            exit(0);
        }

        if (arg[0] == '-') {
            const char* key_start = (arg[1] == '-') ? arg + 2 : arg + 1;
            const char* key = key_start;
            const char* value = NULL;

            char* eq = strchr(key_start, '=');
            if (eq) {
                *eq = '\0';
                key = key_start;
                value = eq + 1;
            } else {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Missing value for option: %s\n", arg);
                    return 0;
                }
                key = key_start;
                value = argv[++i];
            }

            switch (identify_arg(key)) {
                case ARG_ALSA:
                    opts->alsa_port = value;
                    break;
                case ARG_MINVEL: {
                    int vel = atoi(value);
                    if (vel < 0 || vel > 127) {
                        fprintf(stderr, "minvel must be between 0 and 127\n");
                        return 0;
                    }
                    opts->min_velocity = vel;
                    break;
                }
                case ARG_FILE:
                    opts->filename = value;
                    break;
                default:
                    fprintf(stderr, "Unknown option: --%s\n", key);
                    return 0;
            }
        } else if (!opts->filename) {
            opts->filename = arg;  // fallback to first non-flag as file
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", arg);
            return 0;
        }
    }

    if (!opts->filename) {
        fprintf(stderr, "No MIDI file specified.\n");
        print_usage(argv[0]);
        return 0;
    }

    return 1;
}
