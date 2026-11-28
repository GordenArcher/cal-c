// realpath and fsync are POSIX, not C11. Needs to be defined before any include.
#define _POSIX_C_SOURCE 200809L

#include "events/store/store.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "events/format/format.h"

enum {
    PERMISSION_BITS = 07777,
    TEMP_SUFFIX_CAP = 8     // room for ".tmp" and the terminating NUL
};

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

        event.source_line = lineno;
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

// Copies the file to a temp file next to it, leaving one line out, then swaps
// the temp file in with rename(). rename is atomic within one filesystem, so a
// crash at any point leaves either the old file or the new one, never a
// half-written mix. This assumes one user at a time: two runs removing at the
// same moment would share the same temp name.
static RemoveStatus remove_line(const char *target, int line, Event *removed) {
    FILE *in = fopen(target, "r");
    if (!in) return REMOVE_ERROR;

    char temp_path[STORE_PATH_CAP + TEMP_SUFFIX_CAP];
    if (snprintf(temp_path, sizeof temp_path, "%s.tmp", target) >= (int)sizeof temp_path) {
        fclose(in);
        return REMOVE_ERROR;
    }

    FILE *out = fopen(temp_path, "w");
    if (!out) {
        fclose(in);
        return REMOVE_ERROR;
    }

    // A new file gets default permissions. Copy the original's so a 0600
    // events file does not quietly become world readable.
    struct stat original;
    if (stat(target, &original) == 0) chmod(temp_path, original.st_mode & PERMISSION_BITS);

    char buf[FORMAT_LINE_CAP];
    int lineno = 0;
    int at_line_start = 1;
    int skipping = 0;
    int found = 0;
    int write_failed = 0;

    while (fgets(buf, sizeof buf, in)) {
        size_t len = strlen(buf);

        // fgets can hand back a line in several pieces. Only the first piece
        // starts a new line, and only a piece ending in a newline ends it.
        if (at_line_start) {
            lineno++;

            if (lineno == line) {
                int whole_line = (len < sizeof buf - 1) || buf[len - 1] == '\n';
                Event event;

                // An overlong line is never a valid event, so it is kept.
                if (whole_line && format_parse_line(buf, &event) == FORMAT_OK) {
                    event.source_line = line;
                    if (removed) *removed = event;
                    skipping = 1;
                    found = 1;
                }
            }
        }

        at_line_start = (len > 0 && buf[len - 1] == '\n');

        if (!skipping && fputs(buf, out) == EOF) write_failed = 1;
        if (at_line_start) skipping = 0;
    }

    int read_failed = ferror(in);
    fclose(in);

    // ferror catches a write that failed earlier: once the buffer has been
    // flushed and failed, a later fflush finds nothing left to write and
    // reports success. fsync before the rename: without it, a power cut right
    // after the swap can leave an empty file on some filesystems.
    if (fflush(out) != 0 || ferror(out) || fsync(fileno(out)) != 0) write_failed = 1;
    if (fclose(out) != 0) write_failed = 1;

    if (read_failed || write_failed) {
        remove(temp_path);
        return REMOVE_ERROR;
    }
    if (!found) {
        remove(temp_path);
        return REMOVE_NO_SUCH_EVENT;
    }
    if (rename(temp_path, target) != 0) {
        remove(temp_path);
        return REMOVE_ERROR;
    }
    return REMOVE_OK;
}

RemoveStatus store_remove(const char *path, int line, Event *removed) {
    // Resolve symlinks first. Plenty of people keep ~/.cal_events as a symlink
    // into a dotfiles repo, and renaming a temp file over the link itself
    // would silently replace the link with a regular file.
    char *target = realpath(path, NULL);
    if (!target) return (errno == ENOENT) ? REMOVE_FILE_NOT_FOUND : REMOVE_ERROR;

    RemoveStatus status = remove_line(target, line, removed);
    free(target);
    return status;
}
