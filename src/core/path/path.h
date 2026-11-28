#ifndef CAL_CORE_PATH_PATH_H
#define CAL_CORE_PATH_PATH_H

#include <stddef.h>

// Picks the first usable file path: explicit_path, then from_env, then
// home/name. An empty environment value counts as unset. The caller passes the
// getenv() results in, which keeps this pure string work and testable without
// touching the environment or the filesystem.
// Returns 1 on success, 0 when nothing was available or it did not fit in cap.
int path_first_available(const char *explicit_path, const char *from_env,
                         const char *home, const char *name,
                         char *buf, size_t cap);

#endif
