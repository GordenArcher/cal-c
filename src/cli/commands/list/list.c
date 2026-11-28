#include "cli/commands/list/list.h"

#include <stdlib.h>

#include "cli/commands/target/target.h"
#include "events/model/event.h"
#include "events/store/store.h"

enum {
    NUMBER_WIDTH = 4,
    DATE_COLUMN_WIDTH = 11,     // "every 12-25" is the widest date text
    DATE_TEXT_CAP = 16
};

static void print_list_usage(const char *prog, FILE *to) {
    fprintf(to, "Usage: %s list [-e FILE]\n", prog);
}

// "2026-10-15" for a single day, "every 12-25" for one that repeats.
static void date_text(const Event *event, char *buf, size_t cap) {
    if (event->year == EVERY_YEAR) {
        snprintf(buf, cap, "every %02d-%02d", event->month, event->day);
    } else {
        snprintf(buf, cap, "%04d-%02d-%02d", event->year, event->month, event->day);
    }
}

int list_run(const char *prog, int argc, char *argv[], FILE *out, FILE *err) {
    const char *explicit_path;
    int next;

    if (!target_parse(argc, argv, &explicit_path, &next, err) || argc - next != 0) {
        print_list_usage(prog, err);
        return EXIT_FAILURE;
    }

    char path[STORE_PATH_CAP];
    if (!target_resolve(explicit_path, path, sizeof path, err)) return EXIT_FAILURE;

    EventList events;
    eventlist_init(&events);

    switch (store_load(path, &events, err)) {
    case STORE_OK:
        break;
    case STORE_NOT_FOUND:
        eventlist_free(&events);
        // Same rule as showing a calendar: a missing default file just means
        // "no events yet", a missing file the user named is a mistake.
        if (explicit_path) {
            fprintf(err, "Cannot open events file: %s\n", path);
            return EXIT_FAILURE;
        }
        fprintf(out, "No events yet. Add one with: %s add 12-25 Christmas\n", prog);
        return EXIT_SUCCESS;
    case STORE_ERROR:
        eventlist_free(&events);
        fprintf(err, "Could not read events file: %s\n", path);
        return EXIT_FAILURE;
    }

    if (events.count == 0) {
        fprintf(out, "No events in %s\n", path);
        eventlist_free(&events);
        return EXIT_SUCCESS;
    }

    // The number is the line in the file, not a position in this list. It is
    // the same number the loader prints in its warnings and the one remove
    // takes, so a typo reported at "file:5" is also line 5 here and in an editor.
    for (size_t i = 0; i < events.count; i++) {
        const Event *event = &events.items[i];
        char date[DATE_TEXT_CAP];

        date_text(event, date, sizeof date);
        fprintf(out, "%*d  %-*s  %s\n", NUMBER_WIDTH, event->source_line,
                DATE_COLUMN_WIDTH, date, event->label);
    }

    eventlist_free(&events);
    return EXIT_SUCCESS;
}
