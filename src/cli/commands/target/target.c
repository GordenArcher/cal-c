#include "cli/commands/target/target.h"

#include <string.h>

#include "events/store/store.h"

int target_parse(int argc, char *argv[], const char **path, int *consumed, FILE *err) {
    *path = NULL;
    *consumed = 0;

    if (argc == 0) return 1;
    if (strcmp(argv[0], "-e") != 0 && strcmp(argv[0], "--events") != 0) return 1;

    if (argc < 2) {
        fprintf(err, "Option %s needs a file name\n", argv[0]);
        return 0;
    }

    *path = argv[1];
    *consumed = 2;
    return 1;
}

int target_resolve(const char *explicit_path, char *buf, size_t cap, FILE *err) {
    if (store_resolve_path(explicit_path, buf, cap)) return 1;

    fprintf(err, "No events file: use -e FILE, set CAL_EVENTS, or set HOME\n");
    return 0;
}
