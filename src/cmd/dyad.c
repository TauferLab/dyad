// This file is mostly derived from the equivalent flux.c file in flux-core:
// https://github.com/flux-framework/flux-core/blob/master/src/cmd/flux.c

#include <optparse.h>

#include "utils.h"

#define DYAD_EXEC_PATH_ENV "DYAD_EXEC_PATH"

static struct optparse_option opts[] {
    {
        .name = "verbose",
        .key = 'v',
        .has_arg = 0,
        .usage = "Be verbose about the locating of subcommands",
    },
    {
        .name = "version",
        .key = 'V',
        .has_arg = 0,
        .usage = "Print version information instead of running a subcommand",
    },
    OPTPARSE_TABLE_END
};

static optparse* setup_optparse_args (int argc, char** argv)
{
    optparse_err_t e;
    optparse_t* parser = optparse_create ("dyad");
    if (parser == NULL) {
        DYAD_EXIT_MSG ("ERROR: cannot create command-line parser for 'dyad'\n");
    }
    optparse_set (parser, OPTPARSE_USAGE, "[OPTIONS] COMMAND ARGS");
    e = optparse_add_option_table (parser, opts);
    if (e != OPTPARSE_SUCCESS) {
        DYAD_EXIT_MSG ("ERROR: cannot add default options to 'dyad'\n");
    }
    if (optparse_parse_args (parser, argc, argv) < 0) {
        DYAD_EXIT_MSG("ERROR: Cannot parse command-line arguments for 'dyad'\n");
    }
    return parser;
}

void exec_subcommand (const char* path, bool vopt, int argc, char** argv)
{
}

int main(int argc, char** argv)
{
}
