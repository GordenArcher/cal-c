#ifndef CAL_EVENTS_FORMAT_FORMAT_H
#define CAL_EVENTS_FORMAT_FORMAT_H

#include <stddef.h>

#include "events/model/event.h"

enum {
    // Longest valid line is 10 (date) + 1 (|) + 127 (label) + newline. 256
    // leaves headroom, and anything longer is rejected as a mistake.
    FORMAT_LINE_CAP = 256
};

typedef enum {
    FORMAT_OK,
    FORMAT_SKIP,            // blank line or comment, not an error
    FORMAT_NO_SEPARATOR,
    FORMAT_BAD_DATE,
    FORMAT_EMPTY_LABEL,
    FORMAT_LABEL_TOO_LONG
} FormatStatus;

// Accepts YYYY-MM-DD (one day) or MM-DD (every year). Only fills year, month
// and day of *out, the label is left alone.
FormatStatus format_parse_date(const char *text, Event *out);

// Parses one line of the events file: "DATE|label". Pure text in, struct out,
// no I/O, so every edge case can be tested without touching a file.
FormatStatus format_parse_line(const char *line, Event *out);

// Trims and validates the part after the '|' of a line. Shared with the
// holidays store, whose lines carry the same labels but a different date
// syntax, so both files refuse the same labels for the same reasons.
FormatStatus format_parse_label(const char *text, char *out, size_t cap);

// Writes the file representation of an event, without a newline. Returns the
// length written, or -1 when it does not fit in cap.
int format_line(const Event *event, char *buf, size_t cap);

const char *format_status_message(FormatStatus status);

#endif