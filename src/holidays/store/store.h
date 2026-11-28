#ifndef CAL_HOLIDAYS_STORE_STORE_H
#define CAL_HOLIDAYS_STORE_STORE_H

#include <stddef.h>
#include <stdio.h>

#include "events/model/event.h"

enum { HOLIDAYS_PATH_CAP = 1024 };

typedef enum {
    HOLIDAYS_OK,
    HOLIDAYS_NOT_FOUND,
    HOLIDAYS_ERROR
} HolidaysStatus;

// Picks the holidays file: explicit_path first, then $CAL_HOLIDAYS, then
// ~/.cal_holidays. Returns 0 when nothing can be determined or it does not fit.
int holidays_resolve_path(const char *explicit_path, char *buf, size_t cap);

// Loads every holiday that lands in year and appends it to *out. Rules are
// expanded here, so out holds concrete dates and the renderer needs no rule
// knowledge. Bad lines are skipped and reported to warn (NULL stays quiet),
// so one typo never hides the rest of the file. A holiday is a dated label,
// exactly what Event already models; only the loader differs.
HolidaysStatus holidays_load(const char *path, int year, EventList *out, FILE *warn);

#endif
