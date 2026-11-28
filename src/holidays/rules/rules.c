#include "holidays/rules/rules.h"

#include <ctype.h>
#include <string.h>

#include "core/calendar/calendar.h"
#include "events/format/format.h"

enum {
    FULL_DATE_LEN = 10,      // YYYY-MM-DD
    YEARLY_DATE_LEN = 5,     // MM-DD
    YEAR_DIGITS = 4,
    // A movable feast never strays this far from Easter, and the cap keeps a
    // typo like easter+99999 from spinning the day-by-day shift.
    MAX_EASTER_OFFSET = 100,
    DECIMAL_BASE = 10,
    MAX_OFFSET_DIGITS = 3,
    RULE_PART_CAP = 10,      // "wednesday" and its NUL both fit
    RULE_PARTS = 3,
    ORDINAL_MAX = 5
};

// Kept lower case on purpose: holiday files are hand written, so accepting
// every case would make the format look looser than it is.
enum { EASTER_PREFIX_LEN = 6 };

static int is_digit(char c) {
    return isdigit((unsigned char)c) != 0;
}

// Same shape check as the events format, but this only decides whether to send
// the text to the strict date parser. A malformed date like 2026-13-01 still
// looks like a date, and should be reported as a bad date, not a bad rule.
static int looks_like_date(const char *text) {
    size_t len = strlen(text);

    if (len == YEARLY_DATE_LEN) {
        return is_digit(text[0]) && is_digit(text[1]) && text[2] == '-' &&
               is_digit(text[3]) && is_digit(text[4]);
    }

    if (len == FULL_DATE_LEN) {
        for (int i = 0; i < YEAR_DIGITS; i++) {
            if (!is_digit(text[i])) return 0;
        }
        if (text[4] != '-') return 0;
        if (!is_digit(text[5]) || !is_digit(text[6]) || text[7] != '-') return 0;
        return is_digit(text[8]) && is_digit(text[9]);
    }

    return 0;
}

static int word_is(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int weekday_index(const char *text) {
    static const char *const SHORT[DAYS_PER_WEEK] = {
        "sun", "mon", "tue", "wed", "thu", "fri", "sat"
    };
    static const char *const FULL[DAYS_PER_WEEK] = {
        "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"
    };

    for (int i = 0; i < DAYS_PER_WEEK; i++) {
        if (word_is(text, SHORT[i]) || word_is(text, FULL[i])) return i;
    }
    return -1;
}

static int ordinal_value(const char *text) {
    static const char *const SUFFIX[ORDINAL_MAX] = {"st", "nd", "rd", "th", "th"};

    for (int n = 1; n <= ORDINAL_MAX; n++) {
        if (text[0] == (char)('0' + n) && text[1] == SUFFIX[n - 1][0] &&
            text[2] == SUFFIX[n - 1][1] && text[3] == '\0') {
            return n;
        }
    }
    return 0;
}

// Splits "last-mon-5" style rules into exactly three parts. Rejecting the
// shape early is what lets one function own both the last and nth forms.
static int split_parts(const char *text, char parts[RULE_PARTS][RULE_PART_CAP]) {
    int part = 0;
    size_t len = 0;

    for (const char *p = text; ; p++) {
        if (*p == '-' || *p == '\0') {
            if (len == 0) return 0;
            parts[part][len] = '\0';
            part++;
            len = 0;

            if (*p == '\0') break;
            if (part >= RULE_PARTS) return 0;
            continue;
        }

        if (part >= RULE_PARTS || len >= RULE_PART_CAP - 1) return 0;
        parts[part][len++] = *p;
    }

    return part == RULE_PARTS;
}

// A three-part rule whose parts are all numeric is almost always a date with
// the wrong separators ("2026-1-1"), so report it that way instead of calling
// it an unknown rule.
static int all_digits(const char *text) {
    if (*text == '\0') return 0;
    for (; *text; text++) {
        if (!is_digit(*text)) return 0;
    }
    return 1;
}

static int parse_number(const char *text, int min, int max, int *out) {
    int value = 0;
    int digits = 0;

    for (; *text; text++) {
        if (!is_digit(*text)) return 0;
        value = value * DECIMAL_BASE + (*text - '0');
        if (++digits > 2) return 0;
    }

    if (digits == 0 || value < min || value > max) return 0;
    *out = value;
    return 1;
}

// Anonymous Gregorian computus (Meeus/Jones/Butcher). The calendar here is
// proleptic Gregorian, so applying this rule before 1583 is deliberate; a
// Julian special case would only add a branch that no displayed year uses.
static void easter_sunday(int year, int *month, int *day) {
    int a = year % 19;
    int b = year / 100;
    int c = year % 100;
    int d = b / 4;
    int e = b % 4;
    int f = (b + 8) / 25;
    int g = (b - f + 1) / 3;
    int h = (19 * a + b - d - g + 15) % 30;
    int i = c / 4;
    int k = c % 4;
    int l = (32 + 2 * e + 2 * i - h - k) % 7;
    int m = (a + 11 * h + 22 * l) / 451;
    int shifted = h + l - 7 * m + 114;

    *month = shifted / 31;
    *day = shifted % 31 + 1;
}

// Walks one day at a time. The offset is capped at 100, so the loop stays
// short and there is no month arithmetic to get wrong. The resulting year is
// returned because a negative offset can land in the previous year.
static int shift_days(int year, int month, int day, int delta,
                      int *out_year, int *out_month, int *out_day) {
    while (delta > 0) {
        if (day < date_days_in_month(year, month)) {
            day++;
        } else {
            day = 1;
            if (++month > MONTHS_PER_YEAR) {
                month = 1;
                if (++year > MAX_YEAR) return 0;
            }
        }
        delta--;
    }

    while (delta < 0) {
        if (day > 1) {
            day--;
        } else {
            if (--month < 1) {
                month = MONTHS_PER_YEAR;
                if (--year < 1) return 0;
            }
            day = date_days_in_month(year, month);
        }
        delta++;
    }

    *out_year = year;
    *out_month = month;
    *out_day = day;
    return 1;
}

static RuleStatus resolve_easter(const char *text, int year, int *month, int *day) {
    const char *tail = text + EASTER_PREFIX_LEN;
    int offset = 0;

    if (*tail == '+' || *tail == '-') {
        int sign = (*tail == '-') ? -1 : 1;
        tail++;

        int value = 0;
        int digits = 0;
        while (is_digit(*tail)) {
            value = value * DECIMAL_BASE + (*tail - '0');
            if (++digits > MAX_OFFSET_DIGITS) return RULE_BAD_RULE;
            tail++;
        }
        if (digits == 0 || value > MAX_EASTER_OFFSET) return RULE_BAD_RULE;
        offset = sign * value;
    }

    if (*tail != '\0') return RULE_BAD_RULE;

    int easter_month = 0;
    int easter_day = 0;
    easter_sunday(year, &easter_month, &easter_day);

    int moved_year = 0;
    if (!shift_days(year, easter_month, easter_day, offset, &moved_year, month, day)) {
        return RULE_SKIP;
    }

    // A negative offset can walk back past Jan 1. That day belongs to the
    // previous year's calendar, and loading for the current year must not
    // relabel it as this year's.
    return moved_year == year ? RULE_OK : RULE_SKIP;
}

static RuleStatus resolve_weekday(const char *text, int year, int *month, int *day) {
    char parts[RULE_PARTS][RULE_PART_CAP];
    if (!split_parts(text, parts)) return RULE_BAD_RULE;

    int target = weekday_index(parts[1]);
    if (target < 0) {
        if (all_digits(parts[0]) && all_digits(parts[1]) && all_digits(parts[2])) {
            return RULE_BAD_DATE;
        }
        return RULE_BAD_RULE;
    }

    int m;
    if (!parse_number(parts[2], 1, MONTHS_PER_YEAR, &m)) return RULE_BAD_DATE;

    const int days_in_month = date_days_in_month(year, m);

    if (word_is(parts[0], "last")) {
        int last_dow = date_day_of_week(year, m, days_in_month);
        *month = m;
        *day = days_in_month - (last_dow - target + DAYS_PER_WEEK) % DAYS_PER_WEEK;
        return RULE_OK;
    }

    int n = ordinal_value(parts[0]);
    if (n == 0) return RULE_BAD_RULE;

    int first_dow = date_day_of_week(year, m, 1);
    int resolved = 1 + (target - first_dow + DAYS_PER_WEEK) % DAYS_PER_WEEK +
                   (n - 1) * DAYS_PER_WEEK;

    // A 5th Monday only exists in some months. That is not an error in the
    // file, it just means this year has no such holiday.
    if (resolved > days_in_month) return RULE_SKIP;

    *month = m;
    *day = resolved;
    return RULE_OK;
}

RuleStatus rule_resolve(const char *text, int year, int *month, int *day) {
    if (looks_like_date(text)) {
        Event parsed;
        memset(&parsed, 0, sizeof parsed);

        if (format_parse_date(text, &parsed) != FORMAT_OK) return RULE_BAD_DATE;
        if (parsed.year != EVERY_YEAR && parsed.year != year) return RULE_SKIP;

        // MM-DD repeats every year, but Feb 29 has no day in a common year.
        if (parsed.day > date_days_in_month(year, parsed.month)) return RULE_SKIP;

        *month = parsed.month;
        *day = parsed.day;
        return RULE_OK;
    }

    if (strncmp(text, "easter", EASTER_PREFIX_LEN) == 0) {
        return resolve_easter(text, year, month, day);
    }

    return resolve_weekday(text, year, month, day);
}

const char *rule_status_message(RuleStatus status) {
    switch (status) {
    case RULE_OK:       return "ok";
    case RULE_SKIP:     return "skipped";
    case RULE_BAD_DATE: return "invalid date (use YYYY-MM-DD or MM-DD)";
    case RULE_BAD_RULE: return "unknown holiday rule (see --help)";
    }
    return "unknown error";
}
