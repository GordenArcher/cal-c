#include "cli/commands/remove/remove.h"

#include <stdlib.h>

#include "cli/commands/target/target.h"
#include "cli/parse/parse.h"
#include "events/format/format.h"
#include "events/model/event.h"
#include "events/store/store.h"

static void print_remove_usage(const char *prog, FILE *to) {
    fprintf(to,
        "Usage: %s remove [-e FILE] LINE\n"
        "\n"
        "  LINE  the number shown by '%s list'\n",
        prog, prog);
}

int remove_run(const char *prog, int argc, char *argv[], FILE *out, FILE *err) {
    const char *explicit_path;
    int next;

    if (!target_parse(argc, argv, &explicit_path, &next, err) || argc - next != 1) {
        print_remove_usage(prog, err);
        return EXIT_FAILURE;
    }

    int line;
    if (!parse_line_number(argv[next], &line)) {
        fprintf(err, "Invalid line number: %s\n", argv[next]);
        print_remove_usage(prog, err);
        return EXIT_FAILURE;
    }

    char path[STORE_PATH_CAP];
    if (!target_resolve(explicit_path, path, sizeof path, err)) return EXIT_FAILURE;

    Event removed;
    switch (store_remove(path, line, &removed)) {
    case REMOVE_OK: {
        char text[FORMAT_LINE_CAP];
        format_line(&removed, text, sizeof text);
        fprintf(out, "Removed from %s: %s\n", path, text);

        // Numbers are file lines, so everything below the removed one moved
        // up. Saying so is cheaper than someone deleting the wrong event
        // because they trusted an old listing.
        fprintf(out, "Line numbers below it have moved up, run '%s list' to see them again.\n", prog);
        return EXIT_SUCCESS;
    }
    case REMOVE_NO_SUCH_EVENT:
        fprintf(err, "Line %d of %s is not an event (run '%s list' to see the numbers)\n",
                line, path, prog);
        return EXIT_FAILURE;
    case REMOVE_FILE_NOT_FOUND:
        fprintf(err, "Cannot open events file: %s\n", path);
        return EXIT_FAILURE;
    case REMOVE_ERROR:
        fprintf(err, "Could not update %s\n", path);
        return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}
