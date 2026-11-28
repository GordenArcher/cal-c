#ifndef CAL_CLI_COMMANDS_REMOVE_REMOVE_H
#define CAL_CLI_COMMANDS_REMOVE_REMOVE_H

#include <stdio.h>

// Runs "mycal remove [-e FILE] LINE". argc/argv are the arguments AFTER the
// word "remove". LINE is the number "list" shows. Returns the process exit
// code. Output goes to out and err instead of stdout and stderr so tests can
// capture it.
int remove_run(const char *prog, int argc, char *argv[], FILE *out, FILE *err);

#endif
