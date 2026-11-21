#ifndef CAL_CLI_ADD_ADD_H
#define CAL_CLI_ADD_ADD_H

#include <stdio.h>

// Runs "cal add [-e FILE] DATE LABEL...". argc/argv are the arguments AFTER
// the word "add". Returns the process exit code. Output goes to out and err
// instead of stdout and stderr so tests can capture it.
int add_run(const char *prog, int argc, char *argv[], FILE *out, FILE *err);

#endif