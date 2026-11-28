#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli/commands/add/add.h"
#include "cli/args/args.h"
#include "cli/usage/usage.h"
#include "core/clock/clock.h"
#include "events/model/event.h"
#include "events/store/store.h"
#include "render/view/view.h"

// Returns 0 only when the run should stop with an error.
static int load_events(const Config *cfg, EventList *events) {
    if (!cfg->use_events) return 1;

    char path[STORE_PATH_CAP];
    if (!store_resolve_path(cfg->events_path, path, sizeof path)) return 1;

    switch (store_load(path, events, stderr)) {
    case STORE_OK:
        return 1;
    case STORE_NOT_FOUND:
        // A missing default file just means "no events yet". A missing file
        // the user named on the command line is a mistake worth reporting.
        if (cfg->events_path) {
            fprintf(stderr, "Cannot open events file: %s\n", path);
            return 0;
        }
        return 1;
    case STORE_ERROR:
        // Keep showing the calendar, but never hide that events were lost.
        fprintf(stderr, "Could not read events file: %s\n", path);
        return 1;
    }
    return 1;
}

// Wiring only: decide which command runs, hand it its inputs, own all the
// printing to stdout and stderr. Logic belongs in the domain folders.
int main(int argc, char *argv[]) {
    if (argc >= 2 && strcmp(argv[1], "add") == 0) {
        return add_run(argv[0], argc - 2, argv + 2, stdout, stderr);
    }

    Today today = clock_today();
    Config cfg;

    switch (args_parse(argc, argv, &today, &cfg)) {
    case ARGS_HELP:
        usage_print(argv[0], stdout);
        return EXIT_SUCCESS;
    case ARGS_ERROR:
        if (cfg.error[0] != '\0') fprintf(stderr, "%s\n", cfg.error);
        if (cfg.show_usage) usage_print(argv[0], stderr);
        return EXIT_FAILURE;
    case ARGS_OK:
        break;
    }

    EventList events;
    eventlist_init(&events);

    if (!load_events(&cfg, &events)) {
        eventlist_free(&events);
        return EXIT_FAILURE;
    }

    if (cfg.view == VIEW_YEAR) view_print_year(stdout, cfg.year, &cfg.render, &today, &events);
    else                       view_print_month(stdout, cfg.year, cfg.month, &cfg.render, &today, &events);

    eventlist_free(&events);
    return EXIT_SUCCESS;
}