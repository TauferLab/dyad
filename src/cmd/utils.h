#ifndef DYAD_CMD_UTILS_H
#define DYAD_CMD_UTILS_H

#include <stdio.h>
#include <stdlib.h>

#define DYAD_EXIT_MSG(msg, ...) {\
    fprintf (stderr, msg, __VA_ARGS__);\
    exit(EXIT_FAILURE);\
}

#endif /* DYAD_CMD_UTILS_H */
