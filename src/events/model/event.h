#ifndef CAL_EVENTS_MODEL_EVENT_H
#define CAL_EVENTS_MODEL_EVENT_H

#include <stddef.h>

enum {
    EVENT_LABEL_CAP = 128,
    // Stored in Event.year when the event repeats every year. 0 is safe as a
    // sentinel because year 0 does not exist in the range we accept (1 to 9999).
    EVERY_YEAR = 0
};

typedef struct {
    int year;
    int month;
    int day;
    char label[EVENT_LABEL_CAP];
} Event;

typedef struct {
    Event *items;
    size_t count;
    size_t capacity;
} EventList;

void eventlist_init(EventList *list);
void eventlist_free(EventList *list);

// Returns 0 on success, -1 when memory ran out. On failure the list is left
// exactly as it was, so the caller can still free it safely.
int eventlist_add(EventList *list, const Event *event);

int event_occurs_on(const Event *event, int year, int month, int day);

// A NULL list means "no events loaded", so renderers can pass it straight
// through without a separate flag.
int eventlist_has_on(const EventList *list, int year, int month, int day);

#endif