#include "cli/usage/usage.h"

void usage_print(const char *prog, FILE *to) {
    fprintf(to,
        "Usage: %s [options] [[month] year]\n"
        "       %s add [-e FILE] DATE LABEL...\n"
        "       %s list [-e FILE]\n"
        "       %s remove [-e FILE] LINE\n"
        "\n",
        prog, prog, prog, prog);

    fprintf(to,
        "  %s              current month\n"
        "  %s 10 2026      October 2026\n"
        "  %s oct 2026     month names work too\n"
        "  %s 10           October of this year\n"
        "  %s 2026         the whole year\n"
        "\n",
        prog, prog, prog, prog, prog);

    fprintf(to,
        "Options:\n"
        "  -m                  Monday as first day of the week\n"
        "  -w                  show ISO week numbers\n"
        "  -n, --no-color      disable colors (also honours NO_COLOR)\n"
        "  -e, --events FILE   read events from FILE\n"
        "      --no-events     do not load any events\n"
        "  -H, --holidays FILE read holidays from FILE\n"
        "      --no-holidays   do not load any holidays\n"
        "  -h, --help          show this help\n"
        "\n"
        "Events:\n"
        "  Days with events are underlined, and listed under the month view.\n"
        "  The file is FILE from -e, else $CAL_EVENTS, else ~/.cal_events.\n"
        "  One event per line: YYYY-MM-DD|label for a single day, or\n"
        "  MM-DD|label to repeat every year. Lines starting with # are ignored.\n"
        "\n"
        "  Add one from the shell:  %s add 12-25 Christmas\n"
        "  See them numbered:       %s list\n"
        "  Delete one by number:    %s remove 2\n"
        "\n",
        prog, prog, prog);

    fputs(
        "Holidays:\n"
        "  Days with holidays are bold, and listed with a * under the month view.\n"
        "  The file is FILE from -H, else $CAL_HOLIDAYS, else ~/.cal_holidays.\n"
        "  One holiday per line: DATE|label. DATE is YYYY-MM-DD, MM-DD, easter,\n"
        "  easter+N or easter-N, 4th-thu-11 for the nth weekday of a month, or\n"
        "  last-mon-5 for the last one. Lines starting with # are ignored.\n",
        to);
}