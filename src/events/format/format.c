#include "events/format/format.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "core/calendar/calendar.h"

enum {
    FULL_DATE_LEN = 10,     // YYYY-MM-DD
    YEARLY_DATE_LEN = 5,    // MM-DD
    YEAR_DIGITS = 4,
    PART_DIGITS = 2,
    DECIMAL_BASE = 10,
    // MM-DD has no year to check Feb 29 against. A leap year is used as the
    // reference so 02-29 is accepted, it just only shows up in leap years.
    LEAP_REFERENCE_YEAR = 2000
};

static int parse_digits(const char *s, int count, int *out) {
    int value = 0;

    for (int i = 0; i < count; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
        value = value * DECIMAL_BASE + (s[i] - '0');
    }
    *out = value;
    return 1;
}

FormatStatus format_parse_date(const char *text, Event *out) {
    size_t len = strlen(text);
    int year = EVERY_YEAR;
    int month = 0;
    int day = 0;

    if (len == FULL_DATE_LEN) {
        if (!parse_digits(text, YEAR_DIGITS, &year) || text[4] != '-' ||
            !parse_digits(text + 5, PART_DIGITS, &month) || text[7] != '-' ||
            !parse_digits(text + 8, PART_DIGITS, &day)) {
            return FORMAT_BAD_DATE;
        }
        if (year < 1 || year > MAX_YEAR) return FORMAT_BAD_DATE;
    } else if (len == YEARLY_DATE_LEN) {
        if (!parse_digits(text, PART_DIGITS, &month) || text[2] != '-' ||
            !parse_digits(text + 3, PART_DIGITS, &day)) {
            return FORMAT_BAD_DATE;
        }
    } else {
        return FORMAT_BAD_DATE;
    }

    if (month < 1 || month > MONTHS_PER_YEAR) return FORMAT_BAD_DATE;

    int check_year = (year == EVERY_YEAR) ? LEAP_REFERENCE_YEAR : year;
    if (day < 1 || day > date_days_in_month(check_year, month)) return FORMAT_BAD_DATE;

    out->year = year;
    out->month = month;
    out->day = day;
    return FORMAT_OK;
}

FormatStatus format_parse_line(const char *line, Event *out) {
    const char *start = line;
    while (isspace((unsigned char)*start)) start++;
    if (*start == '\0' || *start == '#') return FORMAT_SKIP;

    // Split on the FIRST '|' only, so a label may contain '|' itself.
    const char *sep = strchr(start, '|');
    if (!sep) return FORMAT_NO_SEPARATOR;

    size_t date_len = (size_t)(sep - start);
    while (date_len > 0 && isspace((unsigned char)start[date_len - 1])) date_len--;
    if (date_len > FULL_DATE_LEN) return FORMAT_BAD_DATE;

    char date[FULL_DATE_LEN + 1];
    memcpy(date, start, date_len);
    date[date_len] = '\0';

    // Zeroed first so the fields this function does not set (like
    // source_line) are 0 and not whatever was on the stack.
    Event parsed;
    memset(&parsed, 0, sizeof parsed);
    FormatStatus status = format_parse_date(date, &parsed);
    if (status != FORMAT_OK) return status;

    FormatStatus label_status = format_parse_label(sep + 1, parsed.label, sizeof parsed.label);
    if (label_status != FORMAT_OK) return label_status;

    *out = parsed;
    return FORMAT_OK;
}

FormatStatus format_parse_label(const char *text, char *out, size_t cap) {
    const char *label = text;
    while (isspace((unsigned char)*label)) label++;

    // Trailing whitespace includes the \r of a Windows line ending, which is
    // the only line ending a label can end with once the \n is gone.
    size_t label_len = strlen(label);
    while (label_len > 0 && isspace((unsigned char)label[label_len - 1])) label_len--;

    if (label_len == 0) return FORMAT_EMPTY_LABEL;
    if (label_len >= cap) return FORMAT_LABEL_TOO_LONG;

    memcpy(out, label, label_len);
    out[label_len] = '\0';
    return FORMAT_OK;
}

int format_line(const Event *event, char *buf, size_t cap) {
    int written;

    if (event->year == EVERY_YEAR) {
        written = snprintf(buf, cap, "%02d-%02d|%s", event->month, event->day, event->label);
    } else {
        written = snprintf(buf, cap, "%04d-%02d-%02d|%s",
                           event->year, event->month, event->day, event->label);
    }

    if (written < 0 || (size_t)written >= cap) return -1;
    return written;
}

const char *format_status_message(FormatStatus status) {
    switch (status) {
    case FORMAT_OK:             return "ok";
    case FORMAT_SKIP:           return "blank or comment";
    case FORMAT_NO_SEPARATOR:   return "missing '|' between date and label";
    case FORMAT_BAD_DATE:       return "invalid date (use YYYY-MM-DD or MM-DD)";
    case FORMAT_EMPTY_LABEL:    return "empty label";
    case FORMAT_LABEL_TOO_LONG: return "label too long";
    }
    return "unknown error";
}