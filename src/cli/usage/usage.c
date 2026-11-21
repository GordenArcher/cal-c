#include "cli/usage/usage.h"

void usage_print(const char *prog, FILE *to) {
    fprintf(to,
        "Usage: %s [options] [[month] year]\n"
        "       %s add [-e FILE] DATE LABEL...\n"
        "\n",
        prog, prog);

    fprintf(to,
        "  %s              current month\n"
        "  %s 10 2026      October 2026\n"
        "  %s oct 2026     month names work too\n"
        "  %s 10           October of this year\n"
        "  %s 2026         the whole year\n"
        "\n",
        prog, prog, prog, prog, prog);

    fputs(
        "Options:\n"
        "  -m                  Monday as first day of the week\n"
        "  -w                  show ISO week numbers\n"
        "  -n, --no-color      disable colors (also honours NO_COLOR)\n"
        "  -e, --events FILE   read events from FILE\n"
        "      --no-events     do not load any events\n"
        "  -h, --help          show this help\n"
        "\n"
        "Events:\n"
        "  Days with events are underlined, and listed under the month view.\n"
        "  The file is FILE from -e, else $CAL_EVENTS, else ~/.cal_events.\n"
        "  One event per line: YYYY-MM-DD|label for a single day, or\n"
        "  MM-DD|label to repeat every year. Lines starting with # are ignored.\n",
        to);

    fprintf(to, "  Add one from the shell:  %s add 12-25 Christmas\n", prog);
}