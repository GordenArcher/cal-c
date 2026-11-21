#include <stdlib.h>
#include <string.h>

#include "cli/args/args.h"
#include "core/clock/clock.h"
#include "render/view/view.h"

// Wiring only. If logic starts creeping in here it belongs in one of the
// domain folders instead.
int main(int argc, char *argv[]) {
    Today today = clock_today();

    Config cfg;
    memset(&cfg, 0, sizeof cfg);

    switch (args_parse(argc, argv, &today, &cfg)) {
    case ARGS_HELP:
        return EXIT_SUCCESS;
    case ARGS_ERROR:
        return EXIT_FAILURE;
    case ARGS_OK:
        break;
    }

    if (cfg.view == VIEW_YEAR) view_print_year(cfg.year, &cfg.render, &today);
    else                       view_print_month(cfg.year, cfg.month, &cfg.render, &today);
    return EXIT_SUCCESS;
}