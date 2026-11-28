#ifndef CAL_CORE_FILE_READLINE_H
#define CAL_CORE_FILE_READLINE_H

#include <stdio.h>

typedef enum {
    LINE_OK,
    LINE_TOO_LONG,  // the rest of the line was discarded, nothing to parse
    LINE_EOF
} LineStatus;

// Reads one logical line into buf. fgets stops at cap, not at the newline, so
// a line that fills the buffer without a newline is longer than any line we
// accept. The leftover tail is discarded here; otherwise the next read would
// treat the tail as a line and report a confusing error for it.
LineStatus file_read_line(FILE *f, char *buf, size_t cap);

#endif
