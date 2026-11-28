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

typedef enum {
    REMOVE_OK,
    REMOVE_FILE_NOT_FOUND,
    REMOVE_NO_SUCH_EVENT,   // that line is blank, a comment, a typo, or past the end
    REMOVE_ERROR
} RemoveStatus;

// Deletes the event on the given line (1-based, as in the file and as shown by
// "list") and leaves every other byte of the file exactly as it was, comments
// and typos included. The event that was removed is copied to *removed when it
// is not NULL. The file is replaced atomically, see store.c.
RemoveStatus store_remove(const char *path, int line, Event *removed);

// Picks the events file: explicit_path first, then $CAL_EVENTS, then
// ~/.cal_events. Returns 0 when nothing can be determined or it does not fit.
int store_resolve_path(const char *explicit_path, char *buf, size_t cap);

#endif