#include "render/line/line.h"

#include <stdarg.h>
#include <stdio.h>

void line_reset(Line *line) {
    line->len = 0;
    line->text[0] = '\0';
}

void line_add(Line *line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line->text + line->len, LINE_CAP - line->len, fmt, ap);
    va_end(ap);

    if (n < 0) return;
    line->len += (size_t)n;
    // vsnprintf reports the length it wanted, not what fit. Clamp so a later
    // add never computes a negative remaining size.
    if (line->len >= LINE_CAP) line->len = LINE_CAP - 1;
}

void line_pad(Line *line, int count) {
    for (int i = 0; i < count; i++) line_add(line, " ");
}