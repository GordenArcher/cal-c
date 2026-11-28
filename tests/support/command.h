#ifndef CAL_TESTS_SUPPORT_COMMAND_H
#define CAL_TESTS_SUPPORT_COMMAND_H

#include <stdio.h>

#include "support/test.h"

enum { COMMAND_OUTPUT_CAP = 4096 };

// Everything a subcommand did: its exit code and what it printed.
typedef struct {
    int code;
    char out[COMMAND_OUTPUT_CAP];
    char err[COMMAND_OUTPUT_CAP];
} CommandResult;

typedef int (*CommandFn)(const char *prog, int argc, char *argv[], FILE *out, FILE *err);

// Runs a subcommand with its output captured in temp files, so a test can
// check the exit code, stdout and stderr separately.
static inline void command_run(CommandFn fn, CommandResult *result, int argc, char *argv[]) {
    FILE *out = tmpfile();
    FILE *err = tmpfile();

    result->code = fn("mycal", argc, argv, out, err);
    test_slurp(out, result->out, sizeof result->out);
    test_slurp(err, result->err, sizeof result->err);

    fclose(out);
    fclose(err);
}

#endif
