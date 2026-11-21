#ifndef CAL_EVENTS_STORE_STORE_H
#define CAL_EVENTS_STORE_STORE_H

#include <stddef.h>
#include <stdio.h>

#include "events/model/event.h"

enum { STORE_PATH_CAP = 1024 };

typedef enum {
    STORE_OK,
    STORE_NOT_FOUND,
    STORE_ERROR
} StoreStatus;

// Reads every valid event from path and appends it to *out. Bad lines are
// skipped and reported to warn (pass NULL to stay quiet), so one typo never
// hides the rest of the file.
StoreStatus store_load(const char *path, EventList *out, FILE *warn);

// Appends one event as a new line, creating the file when needed.
StoreStatus store_append(const char *path, const Event *event);

// Picks the events file: explicit_path first, then $CAL_EVENTS, then
// ~/.cal_events. Returns 0 when nothing can be determined or it does not fit.
int store_resolve_path(const char *explicit_path, char *buf, size_t cap);

#endif