#ifndef CAL_CLI_COMMANDS_TARGET_TARGET_H
#define CAL_CLI_COMMANDS_TARGET_TARGET_H

#include <stddef.h>
#include <stdio.h>

// Every subcommand works on one events file and picks it the same way: an
// optional leading "-e FILE" (or "--events FILE"), else $CAL_EVENTS, else
// ~/.cal_events. This module is that one shared rule, so add, list and
// remove can never drift apart.

// Reads the optional leading option. On success returns 1, sets *path to the
// file named (NULL when there was none) and *consumed to how many arguments
// it used (0 or 2). Returns 0 when the option has no value, after saying so
// on err.
int target_parse(int argc, char *argv[], const char **path, int *consumed, FILE *err);

// Turns the explicit path (may be NULL) into the file to work on. Returns 0
// and says why on err when there is none.
int target_resolve(const char *explicit_path, char *buf, size_t cap, FILE *err);

#endif
