#ifndef CAL_TESTS_SUPPORT_FIXTURES_H
#define CAL_TESTS_SUPPORT_FIXTURES_H

#include <stdio.h>
#include <string.h>

#include "events/model/event.h"

// year == EVERY_YEAR makes it a yearly event.
static inline void fixture_add_event(EventList *list, int year, int month, int day,
                                     const char *label) {
    Event event;
    memset(&event, 0, sizeof event);
    event.year = year;
    event.month = month;
    event.day = day;
    snprintf(event.label, sizeof event.label, "%s", label);
    eventlist_add(list, &event);
}

#endif