#include "core/path/path.h"

#include <stdio.h>

// snprintf returns the length it WANTED to write, so "fits" means it is
// positive and strictly below cap (room left for the terminating NUL).
static int fits(int written, size_t cap) {
    return written > 0 && (size_t)written < cap;
}

int path_first_available(const char *explicit_path, const char *from_env,
                         const char *home, const char *name,
                         char *buf, size_t cap) {
    if (explicit_path) return fits(snprintf(buf, cap, "%s", explicit_path), cap);

    if (from_env && from_env[0] != '\0') return fits(snprintf(buf, cap, "%s", from_env), cap);

    if (!home || home[0] == '\0') return 0;
    return fits(snprintf(buf, cap, "%s/%s", home, name), cap);
}
