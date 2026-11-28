// strncasecmp is POSIX, not C11. Needs to be defined before any include.
#define _POSIX_C_SOURCE 200809L

#include "cli/parse/parse.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "core/calendar/calendar.h"

enum { MIN_MONTH_PREFIX = 3 };

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
int parse_month(const char *s, int *out) {
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

int parse_year(const char *s, int *out) {
    int n;
    if (!parse_number(s, &n) || n < 1 || n > MAX_YEAR) return 0;
    *out = n;
    return 1;
}

int parse_line_number(const char *s, int *out) {
    int n;
    if (!parse_number(s, &n) || n < 1) return 0;
    *out = n;
    return 1;
}
