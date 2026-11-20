/*
 * cal: month and year calendar for the terminal.
 *
 * Usage:
 *   cal                  current month
 *   cal 10 2026          October 2026
 *   cal oct 2026         same thing, month names work (3+ letters)
 *   cal 10               October of the current year
 *   cal 2026             whole year, three months per row
 *   cal -m -w 2026       Monday first, with ISO week numbers
 *
 * All date math is done by hand (Sakamoto + ISO week formula) instead of
 * mktime/strftime. mktime is bound by time_t limits and the local timezone,
 * so years far from 1970 behave badly. The pure arithmetic works for any
 * Gregorian year from 1 to 9999 and has no locale surprises.
 */

#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

enum {
    DAYS_PER_WEEK = 7,
    MONTHS_PER_YEAR = 12,
    MAX_WEEK_ROWS = 6,
    HEADER_LINES = 2,
    LINES_PER_BLOCK = HEADER_LINES + MAX_WEEK_ROWS,
    CELL_WIDTH = 3,
    DAYS_WIDTH = DAYS_PER_WEEK * CELL_WIDTH - 1,
    WEEKNUM_WIDTH = 3,
    YEAR_COLUMNS = 3,
    COLUMN_GAP = 3,
    MIN_MONTH_PREFIX = 3,
    MAX_YEAR = 9999,
    FEBRUARY = 2,
    SUNDAY = 0,
    SATURDAY = 6,
    ISO_SUNDAY = 7,
    LEAP_FEBRUARY_DAYS = 29,
    TITLE_CAP = 32,
    // Worst case per line is 7 cells with reverse + color + reset codes plus
    // the week column, roughly 170 bytes. 384 leaves room so vsnprintf never
    // truncates an escape sequence in half and corrupts the terminal state.
    LINE_CAP = 384
};

#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_RED     "\033[31m"
#define ANSI_REVERSE "\033[7m"

static const char *MONTH_NAMES[MONTHS_PER_YEAR] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

// Indexed by weekday where 0 = Sunday, matching day_of_week() below.
static const char *DAY_NAMES[DAYS_PER_WEEK] = {
    "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"
};

typedef struct {
    int monday_first;
    int show_weeks;
    int use_color;
} Options;

typedef struct {
    int valid;
    int year;
    int month;
    int day;
} Today;

// Fixed-capacity line buffer. Every rendered line is padded to the exact
// same visible width, so the year view can print blocks side by side without
// having to measure strings that contain invisible ANSI codes.
typedef struct {
    char text[LINE_CAP];
    size_t len;
} Line;

static void line_add(Line *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(l->text + l->len, LINE_CAP - l->len, fmt, ap);
    va_end(ap);

    if (n < 0) return;
    l->len += (size_t)n;
    // vsnprintf reports the length it wanted, not what fit. Clamp so a later
    // add never computes a negative remaining size.
    if (l->len >= LINE_CAP) l->len = LINE_CAP - 1;
}

static void line_pad(Line *l, int count) {
    for (int i = 0; i < count; i++) line_add(l, " ");
}

// Colors are gated here once, so render code never branches on use_color.
static const char *style(const Options *o, const char *code) {
    return o->use_color ? code : "";
}

static int is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(int y, int m) {
    static const int days[MONTHS_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == FEBRUARY && is_leap(y)) return LEAP_FEBRUARY_DAYS;
    return days[m - 1];
}

// Sakamoto's algorithm, 0 = Sunday. Picked over Zeller because it has no
// negative-modulo edge cases to trip over. The y-- for Jan/Feb works on a
// local copy, callers' years are untouched. The t[] table encodes month
// offsets, do not "tidy" it.
static int day_of_week(int y, int m, int d) {
    static const int t[MONTHS_PER_YEAR] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y--;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % DAYS_PER_WEEK;
}

static int day_of_year(int y, int m, int d) {
    int total = d;
    for (int i = 1; i < m; i++) total += days_in_month(y, i);
    return total;
}

// A year has 53 ISO weeks when Jan 1 is a Thursday, or a Wednesday in a leap
// year. This p() form of the rule avoids needing the weekday of Dec 31.
static int iso_p(int y) {
    return (y + y / 4 - y / 100 + y / 400) % DAYS_PER_WEEK;
}

static int weeks_in_iso_year(int y) {
    return (iso_p(y) == 4 || iso_p(y - 1) == 3) ? 53 : 52;
}

// ISO 8601 week. Early January days can belong to last year's final week and
// late December days can already be week 1 of next year, so the two edge
// branches below are load-bearing for every year-boundary row.
static int iso_week(int y, int m, int d) {
    int wd = day_of_week(y, m, d);
    int iso_wd = (wd == SUNDAY) ? ISO_SUNDAY : wd;
    int week = (day_of_year(y, m, d) - iso_wd + 10) / DAYS_PER_WEEK;

    if (week < 1) return weeks_in_iso_year(y - 1);
    if (week > weeks_in_iso_year(y)) return 1;
    return week;
}

static int block_width(const Options *o) {
    return DAYS_WIDTH + (o->show_weeks ? WEEKNUM_WIDTH : 0);
}

// Renders one month into LINES_PER_BLOCK fixed-width lines and returns how
// many week rows actually have days. The year view needs all blocks the same
// height to interleave them, the single view uses the return value to skip
// the empty trailing rows.
static int render_month(int y, int m, int with_year, const Options *o,
                        const Today *today, Line out[LINES_PER_BLOCK]) {
    const int width = block_width(o);
    const char *dim = style(o, ANSI_DIM);
    const char *reset = style(o, ANSI_RESET);

    for (int i = 0; i < LINES_PER_BLOCK; i++) {
        out[i].len = 0;
        out[i].text[0] = '\0';
    }

    char title[TITLE_CAP];
    if (with_year) snprintf(title, sizeof title, "%s %d", MONTH_NAMES[m - 1], y);
    else           snprintf(title, sizeof title, "%s", MONTH_NAMES[m - 1]);

    int title_len = (int)strlen(title);
    int left = (width - title_len) / 2;
    line_pad(&out[0], left);
    line_add(&out[0], "%s%s%s", style(o, ANSI_BOLD), title, reset);
    line_pad(&out[0], width - left - title_len);

    if (o->show_weeks) line_add(&out[1], "%sWk%s ", dim, reset);
    for (int c = 0; c < DAYS_PER_WEEK; c++) {
        int dow = o->monday_first ? (c + 1) % DAYS_PER_WEEK : c;
        line_add(&out[1], "%s%s%s", dim, DAY_NAMES[dow], reset);
        if (c < DAYS_PER_WEEK - 1) line_add(&out[1], " ");
    }

    const int first_dow = day_of_week(y, m, 1);
    const int offset = o->monday_first ? (first_dow + DAYS_PER_WEEK - 1) % DAYS_PER_WEEK : first_dow;
    const int total = days_in_month(y, m);
    int used_rows = 0;

    for (int r = 0; r < MAX_WEEK_ROWS; r++) {
        Line *ln = &out[HEADER_LINES + r];
        const int row_first = r * DAYS_PER_WEEK - offset + 1;
        int row_last = row_first + DAYS_PER_WEEK - 1;
        if (row_last > total) row_last = total;

        if (row_first > total) {
            line_pad(ln, width);
            continue;
        }
        used_rows++;

        // Week number comes from the LAST in-month day of the row. With a
        // Sunday start, the Sunday belongs to the previous ISO week but
        // Mon to Sat belong to the next, so the last day gives the right
        // answer for both start modes. Using the first day would be wrong
        // for Sunday-first layouts.
        if (o->show_weeks) line_add(ln, "%s%2d%s ", dim, iso_week(y, m, row_last), reset);

        for (int c = 0; c < DAYS_PER_WEEK; c++) {
            int day = row_first + c;

            if (day < 1 || day > total) {
                line_pad(ln, CELL_WIDTH - 1);
            } else {
                int dow = (first_dow + day - 1) % DAYS_PER_WEEK;
                int weekend = (dow == SUNDAY || dow == SATURDAY);
                int is_today = today->valid && y == today->year &&
                               m == today->month && day == today->day;

                line_add(ln, "%s%s%2d%s",
                         is_today ? style(o, ANSI_REVERSE) : "",
                         weekend ? style(o, ANSI_RED) : "",
                         day, reset);
            }
            if (c < DAYS_PER_WEEK - 1) line_add(ln, " ");
        }
    }
    return used_rows;
}

static void print_month_view(int y, int m, const Options *o, const Today *today) {
    Line block[LINES_PER_BLOCK];
    int used = render_month(y, m, 1, o, today, block);

    for (int i = 0; i < HEADER_LINES + used; i++) puts(block[i].text);
}

// Year view builds three blocks per row group and prints them line by line.
// This only works because render_month pads every line to a constant visible
// width. If that padding ever changes, the columns will drift apart.
static void print_year_view(int y, const Options *o, const Today *today) {
    const int width = block_width(o);
    const int total_width = YEAR_COLUMNS * width + (YEAR_COLUMNS - 1) * COLUMN_GAP;

    char title[TITLE_CAP];
    snprintf(title, sizeof title, "%d", y);
    int left = (total_width - (int)strlen(title)) / 2;
    printf("%*s%s%s%s\n\n", left, "", style(o, ANSI_BOLD), title, style(o, ANSI_RESET));

    for (int m = 1; m <= MONTHS_PER_YEAR; m += YEAR_COLUMNS) {
        Line blocks[YEAR_COLUMNS][LINES_PER_BLOCK];
        int rows = HEADER_LINES;

        for (int k = 0; k < YEAR_COLUMNS; k++) {
            int used = render_month(y, m + k, 0, o, today, blocks[k]);
            if (HEADER_LINES + used > rows) rows = HEADER_LINES + used;
        }

        for (int i = 0; i < rows; i++) {
            for (int k = 0; k < YEAR_COLUMNS; k++) {
                fputs(blocks[k][i].text, stdout);
                if (k < YEAR_COLUMNS - 1) printf("%*s", COLUMN_GAP, "");
            }
            putchar('\n');
        }
        if (m + YEAR_COLUMNS <= MONTHS_PER_YEAR) putchar('\n');
    }
}

static int parse_number(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);

    // Full-string check so "10abc" is rejected instead of silently read as 10.
    if (*s == '\0' || *end != '\0' || v < 0 || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

// Accepts 1..12 or a month name prefix of 3+ letters. The 3 letter floor
// matters: "ma" would be ambiguous between March and May, "mar" and "may"
// are not.
static int parse_month(const char *s, int *out) {
    int n;
    if (parse_number(s, &n)) {
        if (n < 1 || n > MONTHS_PER_YEAR) return 0;
        *out = n;
        return 1;
    }

    size_t len = strlen(s);
    if (len < MIN_MONTH_PREFIX) return 0;

    for (int i = 0; i < MONTHS_PER_YEAR; i++) {
        if (strncasecmp(s, MONTH_NAMES[i], len) == 0) {
            *out = i + 1;
            return 1;
        }
    }
    return 0;
}

static int parse_year(const char *s, int *out) {
    int n;
    if (!parse_number(s, &n) || n < 1 || n > MAX_YEAR) return 0;
    *out = n;
    return 1;
}

static Today get_today(void) {
    Today t = {0, 0, 0, 0};
    time_t now = time(NULL);
    if (now == (time_t)-1) return t;

    // localtime can return NULL, and the pointer is shared static storage, so
    // copy the fields out immediately instead of holding on to it.
    struct tm *lt = localtime(&now);
    if (!lt) return t;

    t.valid = 1;
    t.year = lt->tm_year + 1900;
    t.month = lt->tm_mon + 1;
    t.day = lt->tm_mday;
    return t;
}

static void print_usage(const char *prog, FILE *to) {
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

int main(int argc, char *argv[]) {
    Options opts = {0, 0, 0};
    // Colors only when stdout is a terminal. Piping into a file or grep with
    // raw escape codes in it is a pain, and NO_COLOR is the community standard
    // for opting out.
    opts.use_color = isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;

    const char *positional[2];
    int npos = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            print_usage(argv[0], stdout);
            return 0;
        }
        if (strcmp(arg, "--no-color") == 0) {
            opts.use_color = 0;
            continue;
        }

        if (arg[0] == '-' && arg[1] != '\0') {
            // Short flags can be combined, so "-mw" works like "-m -w".
            for (const char *f = arg + 1; *f; f++) {
                if (*f == 'm') opts.monday_first = 1;
                else if (*f == 'w') opts.show_weeks = 1;
                else if (*f == 'n') opts.use_color = 0;
                else if (*f == 'h') { print_usage(argv[0], stdout); return 0; }
                else {
                    fprintf(stderr, "Unknown option: -%c\n", *f);
                    print_usage(argv[0], stderr);
                    return 1;
                }
            }
            continue;
        }

        if (npos >= 2) {
            print_usage(argv[0], stderr);
            return 1;
        }
        positional[npos++] = arg;
    }

    Today today = get_today();
    int month = today.month;
    int year = today.year;
    int whole_year = 0;

    if (npos == 0 && !today.valid) {
        fprintf(stderr, "Could not read the system clock, pass a month and year.\n");
        return 1;
    }

    if (npos == 1) {
        // One argument is ambiguous by nature. Anything that parses as a month
        // (1 to 12 or a name) is a month of the current year, everything else
        // is treated as a year. The cost is that years 1 to 12 need the
        // two argument form, which nobody asks for in practice.
        if (parse_month(positional[0], &month)) {
            if (!today.valid) {
                fprintf(stderr, "Could not read the system clock, pass a year too.\n");
                return 1;
            }
        } else if (parse_year(positional[0], &year)) {
            whole_year = 1;
        } else {
            fprintf(stderr, "Invalid month or year: %s\n", positional[0]);
            return 1;
        }
    } else if (npos == 2) {
        if (!parse_month(positional[0], &month)) {
            fprintf(stderr, "Invalid month: %s\n", positional[0]);
            return 1;
        }
        if (!parse_year(positional[1], &year)) {
            fprintf(stderr, "Invalid year: %s (use 1 to %d)\n", positional[1], MAX_YEAR);
            return 1;
        }
    }

    if (whole_year) print_year_view(year, &opts, &today);
    else            print_month_view(year, month, &opts, &today);
    return 0;
}