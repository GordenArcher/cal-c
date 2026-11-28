#ifndef CAL_CLI_COMMANDS_LIST_LIST_H
#define CAL_CLI_COMMANDS_LIST_LIST_H

#include <stdio.h>

// Runs "mycal list [-e FILE]". argc/argv are the arguments AFTER the word
// "list". Returns the process exit code. Output goes to out and err instead
// of stdout and stderr so tests can capture it.
int list_run(const char *prog, int argc, char *argv[], FILE *out, FILE *err);

#endif
