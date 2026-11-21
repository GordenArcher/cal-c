#include "cli/add/add.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "events/format/format.h"
#include "events/model/event.h"
#include "events/store/store.h"

typedef enum {
    LABEL_OK,
    LABEL_EMPTY,
    LABEL_TOO_LONG,
    LABEL_BAD_CHAR
} LabelStatus;

static void print_add_usage(const char *prog, FILE *to) {
    fprintf(to,
        "Usage: %s add [-e FILE] DATE LABEL...\n"
        "\n"
        "  DATE   YYYY-MM-DD for a single day, MM-DD to repeat every year\n"
        "  LABEL  what happens that day, quotes are optional\n"
        "\n"
        "Example: %s add 12-25 Christmas\n",
        prog, prog);
}

// Joins the remaining arguments with single spaces, so both
//   cal add 12-25 Christmas Day   and   cal add 12-25 "Christmas Day"
// work. Control characters are refused because a newline inside a label would
// split one event into two lines of the file.
static LabelStatus build_label(int count, char *words[], char *out, size_t cap) {
    size_t len = 0;

    for (int i = 0; i < count; i++) {
        for (const char *p = words[i]; *p; p++) {
            if (iscntrl((unsigned char)*p)) return LABEL_BAD_CHAR;
        }

        size_t word_len = strlen(words[i]);
        size_t separator = (len > 0) ? 1 : 0;
        if (len + separator + word_len >= cap) return LABEL_TOO_LONG;

        if (separator) out[len++] = ' ';
        memcpy(out + len, words[i], word_len);
        len += word_len;
    }
    out[len] = '\0';

    // Leading and trailing spaces would be trimmed when the file is read back,
    // so trim now and the label we confirm is the label that gets loaded.
    size_t start = 0;
    while (out[start] == ' ') start++;
    while (len > start && out[len - 1] == ' ') len--;

    memmove(out, out + start, len - start);
    out[len - start] = '\0';

    return (len == start) ? LABEL_EMPTY : LABEL_OK;
}

int add_run(const char *prog, int argc, char *argv[], FILE *out, FILE *err) {
    const char *explicit_path = NULL;
    int next = 0;

    if (argc > 0 && strcmp(argv[0], "-e") == 0) {
        if (argc < 2) {
            fprintf(err, "Option -e needs a file name\n");
            print_add_usage(prog, err);
            return EXIT_FAILURE;
        }
        explicit_path = argv[1];
        next = 2;
    }

    if (argc - next < 2) {
        print_add_usage(prog, err);
        return EXIT_FAILURE;
    }

    Event event;
    if (format_parse_date(argv[next], &event) != FORMAT_OK) {
        fprintf(err, "Invalid date: %s\n", argv[next]);
        print_add_usage(prog, err);
        return EXIT_FAILURE;
    }

    switch (build_label(argc - next - 1, argv + next + 1, event.label, sizeof event.label)) {
    case LABEL_OK:
        break;
    case LABEL_EMPTY:
        fprintf(err, "The label is empty\n");
        return EXIT_FAILURE;
    case LABEL_TOO_LONG:
        fprintf(err, "The label is too long (max %d characters)\n", EVENT_LABEL_CAP - 1);
        return EXIT_FAILURE;
    case LABEL_BAD_CHAR:
        fprintf(err, "The label cannot contain line breaks or other control characters\n");
        return EXIT_FAILURE;
    }

    char path[STORE_PATH_CAP];
    if (!store_resolve_path(explicit_path, path, sizeof path)) {
        fprintf(err, "No events file: use -e FILE, set CAL_EVENTS, or set HOME\n");
        return EXIT_FAILURE;
    }

    if (store_append(path, &event) != STORE_OK) {
        fprintf(err, "Could not write to %s\n", path);
        return EXIT_FAILURE;
    }

    char line[FORMAT_LINE_CAP];
    format_line(&event, line, sizeof line);
    fprintf(out, "Added to %s: %s\n", path, line);
    return EXIT_SUCCESS;
}