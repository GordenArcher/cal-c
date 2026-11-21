#ifndef CAL_CLI_ARGS_ARGS_H
#define CAL_CLI_ARGS_ARGS_H

#include "core/clock/clock.h"
#include "render/style/options.h"

typedef enum {
    VIEW_MONTH,
    VIEW_YEAR
} ViewKind;

typedef enum {
    ARGS_OK,
    ARGS_HELP,
    ARGS_ERROR
} ArgsStatus;

// What main needs to know after the command line has been understood.
typedef struct {
    ViewKind view;
    int month;
    int year;
    RenderOptions render;
} Config;

// ARGS_HELP and ARGS_ERROR mean everything worth printing already went out
// (help to stdout, errors and usage to stderr), so the caller only has to
// pick the exit code and stop.
ArgsStatus args_parse(int argc, char *argv[], const Today *today, Config *cfg);

#endif