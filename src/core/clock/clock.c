#include "core/clock/clock.h"

#include <time.h>

enum { TM_YEAR_BASE = 1900 };

Today clock_today(void) {
    Today t = {0, 0, 0, 0};
    time_t now = time(NULL);
    if (now == (time_t)-1) return t;

    // localtime can return NULL, and the pointer is shared static storage, so
    // copy the fields out immediately instead of holding on to it.
    struct tm *lt = localtime(&now);
    if (!lt) return t;

    t.valid = 1;
    t.year = lt->tm_year + TM_YEAR_BASE;
    t.month = lt->tm_mon + 1;
    t.day = lt->tm_mday;
    return t;
}