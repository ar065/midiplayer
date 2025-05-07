#ifndef ARG_PARSER_H
#define ARG_PARSER_H

typedef struct {
    const char* filename;
    const char* alsa_port;
    int min_velocity;
} Options;

int parse_args(int argc, char* argv[], Options* opts);

#endif // ARG_PARSER_H
