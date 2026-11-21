// isatty is POSIX, not C11. Needs to be defined before any include.
#define _POSIX_C_SOURCE 200809L

#include "cli/args/args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli/parse/parse.h"
#include "core/calendar/calendar.h"

enum { MAX_POSITIONAL = 2 };

// Help text lives here and not in its own module because args is the only
// caller. If a second caller ever shows up, that is the time to split it out.
static void usage_print(const char *prog, FILE *to) {
    fprintf(to,
        "Usage: %s [options] [[month] year]\n"
        "\n"
        "  %s              current month\n"
        "  %s 10 2026      October 2026\n"
        "  %s oct 2026     month names work too\n"
        "  %s 10           October of this year\n"
        "  %s 2026         the whole year\n"
        "\n"
        "Options:\n"
        "  -m              Monday as first day of the week\n"
        "  -w              show ISO week numbers\n"
        "  -n, --no-color  disable colors (also honours NO_COLOR)\n"
        "  -h, --help      show this help\n",
        prog, prog, prog, prog, prog, prog);
}

// Colors only when stdout is a terminal. Piping into a file or grep with raw
// escape codes in it is a pain, and NO_COLOR is the community standard for
// opting out.
static int default_use_color(void) {
    return isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
}

// Short flags can be combined, so "-mw" works like "-m -w".
static ArgsStatus parse_short_flags(const char *flags, RenderOptions *render) {
    for (const char *f = flags; *f; f++) {
        switch (*f) {
        case 'm': render->monday_first = 1; break;
        case 'w': render->show_weeks = 1; break;
        case 'n': render->use_color = 0; break;
        case 'h': return ARGS_HELP;
        default:
            fprintf(stderr, "Unknown option: -%c\n", *f);
            return ARGS_ERROR;
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
        fprintf(stderr, "Could not read the system clock, pass a month and year.\n");
        return ARGS_ERROR;
    }

    if (npos == 1) {
        // One argument is ambiguous by nature. Anything that parses as a month
        // (1 to 12 or a name) is a month of the current year, everything else
        // is treated as a year. The cost is that years 1 to 12 need the
        // two argument form, which nobody asks for in practice.
        if (parse_month(positional[0], &cfg->month)) {
            if (!today->valid) {
                fprintf(stderr, "Could not read the system clock, pass a year too.\n");
                return ARGS_ERROR;
            }
        } else if (parse_year(positional[0], &cfg->year)) {
            cfg->view = VIEW_YEAR;
        } else {
            fprintf(stderr, "Invalid month or year: %s\n", positional[0]);
            return ARGS_ERROR;
        }
    } else if (npos == 2) {
        if (!parse_month(positional[0], &cfg->month)) {
            fprintf(stderr, "Invalid month: %s\n", positional[0]);
            return ARGS_ERROR;
        }
        if (!parse_year(positional[1], &cfg->year)) {
            fprintf(stderr, "Invalid year: %s (use 1 to %d)\n", positional[1], MAX_YEAR);
            return ARGS_ERROR;
        }
    }
    return ARGS_OK;
}

ArgsStatus args_parse(int argc, char *argv[], const Today *today, Config *cfg) {
    cfg->render.monday_first = 0;
    cfg->render.show_weeks = 0;
    cfg->render.use_color = default_use_color();

    const char *positional[MAX_POSITIONAL];
    int npos = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            usage_print(argv[0], stdout);
            return ARGS_HELP;
        }
        if (strcmp(arg, "--no-color") == 0) {
            cfg->render.use_color = 0;
            continue;
        }

        if (arg[0] == '-' && arg[1] != '\0') {
            ArgsStatus st = parse_short_flags(arg + 1, &cfg->render);
            if (st == ARGS_HELP) usage_print(argv[0], stdout);
            if (st == ARGS_ERROR) usage_print(argv[0], stderr);
            if (st != ARGS_OK) return st;
            continue;
        }

        if (npos >= MAX_POSITIONAL) {
            usage_print(argv[0], stderr);
            return ARGS_ERROR;
        }
        positional[npos++] = arg;
    }

    return resolve_target(positional, npos, today, cfg);
}