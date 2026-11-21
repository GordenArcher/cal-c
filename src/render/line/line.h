#ifndef CAL_RENDER_LINE_LINE_H
#define CAL_RENDER_LINE_LINE_H

#include <stddef.h>

enum {
    // Worst case per line is 7 cells with reverse + color + reset codes plus
    // the week column, roughly 170 bytes. 384 leaves room so vsnprintf never
    // truncates an escape sequence in half and corrupts the terminal state.
    LINE_CAP = 384
};

// Fixed-capacity line buffer. Every rendered line is padded to the exact
// same visible width, so the year view can print blocks side by side without
// having to measure strings that contain invisible ANSI codes.
typedef struct {
    char text[LINE_CAP];
    size_t len;
} Line;

void line_reset(Line *line);
void line_add(Line *line, const char *fmt, ...);
void line_pad(Line *line, int count);

#endif