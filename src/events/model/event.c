#include "events/model/event.h"

#include <stdlib.h>
#include <string.h>

enum {
    INITIAL_CAPACITY = 16,
    GROWTH_FACTOR = 2
};

void eventlist_init(EventList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void eventlist_free(EventList *list) {
    free(list->items);
    eventlist_init(list);
}

int eventlist_add(EventList *list, const Event *event) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * GROWTH_FACTOR : INITIAL_CAPACITY;

        // Assign to a temporary first. Writing realloc's result straight into
        // list->items would leak the old block and lose the data on failure.
        Event *grown = realloc(list->items, new_capacity * sizeof *grown);
        if (!grown) return -1;

        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count++] = *event;
    return 0;
}

int event_occurs_on(const Event *event, int year, int month, int day) {
    if (event->month != month || event->day != day) return 0;
    return event->year == EVERY_YEAR || event->year == year;
}

int eventlist_has_on(const EventList *list, int year, int month, int day) {
    if (!list) return 0;

    for (size_t i = 0; i < list->count; i++) {
        if (event_occurs_on(&list->items[i], year, month, day)) return 1;
    }
    return 0;
}