#include "events/store/store.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "events/format/format.h"

#define DEFAULT_FILE_NAME ".cal_events"
#define ENV_EVENTS_PATH   "CAL_EVENTS"

// snprintf returns the length it WANTED to write, so "fits" means it is
// positive and strictly below cap (room left for the terminating NUL).
static int fits(int written, size_t cap) {
    return written > 0 && (size_t)written < cap;
}

int store_resolve_path(const char *explicit_path, char *buf, size_t cap) {
    if (explicit_path) return fits(snprintf(buf, cap, "%s", explicit_path), cap);

    const char *from_env = getenv(ENV_EVENTS_PATH);
    if (from_env && from_env[0] != '\0') return fits(snprintf(buf, cap, "%s", from_env), cap);

    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') return 0;
    return fits(snprintf(buf, cap, "%s/%s", home, DEFAULT_FILE_NAME), cap);
}

static void warn_line(FILE *warn, const char *path, int lineno, const char *reason) {
    if (warn) fprintf(warn, "%s:%d: %s, line skipped\n", path, lineno, reason);
}

StoreStatus store_load(const char *path, EventList *out, FILE *warn) {
    FILE *f = fopen(path, "r");
    if (!f) return (errno == ENOENT) ? STORE_NOT_FOUND : STORE_ERROR;

    char buf[FORMAT_LINE_CAP];
    int lineno = 0;

    while (fgets(buf, sizeof buf, f)) {
        lineno++;

        // fgets stops at the buffer size, not at the end of the line. A line
        // that filled the buffer without a newline is longer than any valid
        // event, so throw away the rest of it. If we did not, the leftover
        // tail would be read as the next "line" and produce a bogus warning.
        size_t len = strlen(buf);
        if (len == sizeof buf - 1 && buf[len - 1] != '\n') {
            int c;
            while ((c = fgetc(f)) != EOF && c != '\n') {}
            warn_line(warn, path, lineno, "line too long");
            continue;
        }

        Event event;
        FormatStatus status = format_parse_line(buf, &event);

        if (status == FORMAT_SKIP) continue;
        if (status != FORMAT_OK) {
            warn_line(warn, path, lineno, format_status_message(status));
            continue;
        }

        if (eventlist_add(out, &event) != 0) {
            fclose(f);
            return STORE_ERROR;
        }
    }

    int failed = ferror(f);
    fclose(f);
    return failed ? STORE_ERROR : STORE_OK;
}

StoreStatus store_append(const char *path, const Event *event) {
    char line[FORMAT_LINE_CAP];
    if (format_line(event, line, sizeof line) < 0) return STORE_ERROR;

    // "a+" creates the file if missing and lets us peek at the last byte
    // while every write still goes to the end.
    FILE *f = fopen(path, "a+");
    if (!f) return STORE_ERROR;

    // A hand-edited file may not end with a newline. Without this check our
    // line would be glued onto the last existing event and both would break.
    if (fseek(f, -1L, SEEK_END) == 0) {
        int last = fgetc(f);
        // C requires a seek between reading and writing on the same stream.
        fseek(f, 0L, SEEK_END);
        if (last != '\n' && last != EOF) fputc('\n', f);
    }

    if (fprintf(f, "%s\n", line) < 0) {
        fclose(f);
        return STORE_ERROR;
    }
    return fclose(f) == 0 ? STORE_OK : STORE_ERROR;
}