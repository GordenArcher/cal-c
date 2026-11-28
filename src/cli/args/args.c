// isatty is POSIX, not C11. Needs to be defined before any include.
#define _POSIX_C_SOURCE 200809L

#include "cli/args/args.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli/parse/parse.h"
#include "core/calendar/calendar.h"

enum { MAX_POSITIONAL = 2 };

static ArgsStatus fail(Config *cfg, int show_usage, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cfg->error, sizeof cfg->error, fmt, ap);
    va_end(ap);

    cfg->show_usage = show_usage;
    return ARGS_ERROR;
}

// Colors only when stdout is a terminal. Piping into a file or grep with raw
// escape codes in it is a pain, and NO_COLOR is the community standard for
// opting out.
static int default_use_color(void) {
    return isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
}

// Short flags can be combined, so "-mw" works like "-m -w".
static ArgsStatus parse_short_flags(const char *flags, Config *cfg) {
    for (const char *f = flags; *f; f++) {
        switch (*f) {
        case 'm': cfg->render.monday_first = 1; break;
        case 'w': cfg->render.show_weeks = 1; break;
        case 'n': cfg->render.use_color = 0; break;
        case 'h': return ARGS_HELP;
        default:
            return fail(cfg, 1, "Unknown option: -%c", *f);
        }
    }
    return ARGS_OK;
}

// Turns the leftover non-flag arguments into a month or year target.
static ArgsStatus resolve_target(const char *positional[], int npos,
                                 const Today *today, Config *cfg) {
    cfg->view = VIEW_MONTH;
    cfg->month = today->month;
    cfg->year = today->year;

    if (npos == 0 && !today->valid) {
        return fail(cfg, 0, "Could not read the system clock, pass a month and year.");
    }

    if (npos == 1) {
        // One argument is ambiguous by nature. Anything that parses as a month
        // (1 to 12 or a name) is a month of the current year, everything else
        // is treated as a year. The cost is that years 1 to 12 need the
        // two argument form, which nobody asks for in practice.
        if (parse_month(positional[0], &cfg->month)) {
            if (!today->valid) {
                return fail(cfg, 0, "Could not read the system clock, pass a year too.");
            }
        } else if (parse_year(positional[0], &cfg->year)) {
            cfg->view = VIEW_YEAR;
        } else {
            return fail(cfg, 0, "Invalid month or year: %s", positional[0]);
        }
    } else if (npos == 2) {
        if (!parse_month(positional[0], &cfg->month)) {
            return fail(cfg, 0, "Invalid month: %s", positional[0]);
        }
        if (!parse_year(positional[1], &cfg->year)) {
            return fail(cfg, 0, "Invalid year: %s (use 1 to %d)", positional[1], MAX_YEAR);
        }
    }
    return ARGS_OK;
}

ArgsStatus args_parse(int argc, char *argv[], const Today *today, Config *cfg) {
    memset(cfg, 0, sizeof *cfg);
    cfg->use_events = 1;
    cfg->use_holidays = 1;
    cfg->render.use_color = default_use_color();

    const char *positional[MAX_POSITIONAL];
    int npos = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) return ARGS_HELP;

        if (strcmp(arg, "--no-color") == 0) {
            cfg->render.use_color = 0;
            continue;
        }
        if (strcmp(arg, "--no-events") == 0) {
            cfg->use_events = 0;
            continue;
        }
        if (strcmp(arg, "--no-holidays") == 0) {
            cfg->use_holidays = 0;
            continue;
        }

        // Takes a value, so it cannot live in the combinable short flags.
        if (strcmp(arg, "-e") == 0 || strcmp(arg, "--events") == 0) {
            if (i + 1 >= argc) return fail(cfg, 1, "Option %s needs a file name", arg);
            cfg->events_path = argv[++i];
            continue;
        }
        if (strcmp(arg, "-H") == 0 || strcmp(arg, "--holidays") == 0) {
            if (i + 1 >= argc) return fail(cfg, 1, "Option %s needs a file name", arg);
            cfg->holidays_path = argv[++i];
            continue;
        }

        if (arg[0] == '-' && arg[1] != '\0') {
            ArgsStatus status = parse_short_flags(arg + 1, cfg);
            if (status != ARGS_OK) return status;
            continue;
        }

        if (npos >= MAX_POSITIONAL) {
            // No message on purpose, the usage text says it all.
            cfg->show_usage = 1;
            return ARGS_ERROR;
        }
        positional[npos++] = arg;
    }

    return resolve_target(positional, npos, today, cfg);
}