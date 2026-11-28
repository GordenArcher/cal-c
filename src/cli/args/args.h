#ifndef CAL_CLI_ARGS_ARGS_H
#define CAL_CLI_ARGS_ARGS_H

#include "core/clock/clock.h"
#include "render/style/options.h"

enum { ARGS_ERROR_CAP = 256 };

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

    // NULL means "pick the default file", see store_resolve_path.
    const char *events_path;
    int use_events;

    // Same rule for the read-only holidays layer, see holidays_resolve_path.
    const char *holidays_path;
    int use_holidays;

    // Filled when the status is ARGS_ERROR. error may be empty when the usage
    // text alone says enough, show_usage asks the caller to print it too.
    char error[ARGS_ERROR_CAP];
    int show_usage;
} Config;

// Pure parsing: nothing is printed here. That keeps it testable, and main
// stays the only place that decides what goes to stdout and stderr.
ArgsStatus args_parse(int argc, char *argv[], const Today *today, Config *cfg);

#endif