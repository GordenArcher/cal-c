/*
 * Pure calendar arithmetic. No I/O, no system clock, no locale, which is why
 * it works for any Gregorian year from 1 to 9999 and why it can be tested in
 * isolation. Do not add anything here that touches the terminal or time().
 */

#include "core/calendar/calendar.h"

enum {
    FEBRUARY = 2,
    LEAP_FEBRUARY_DAYS = 29,
    ISO_SUNDAY = 7,
    LONG_ISO_YEAR_WEEKS = 53,
    SHORT_ISO_YEAR_WEEKS = 52,
    // Offsets from the ISO week formula, week = (doy - iso_wd + 10) / 7.
    // The +10 shifts the count so week 1 is the one holding the first Thursday.
    ISO_WEEK_SHIFT = 10,
    P_THURSDAY_START = 4,
    P_WEDNESDAY_END = 3
};

const char *const MONTH_NAMES[MONTHS_PER_YEAR] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

// Indexed by weekday where 0 = Sunday, matching date_day_of_week().
const char *const DAY_NAMES[DAYS_PER_WEEK] = {
    "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"
};

int date_is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int date_days_in_month(int year, int month) {
    static const int days[MONTHS_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == FEBRUARY && date_is_leap(year)) return LEAP_FEBRUARY_DAYS;
    return days[month - 1];
}

// Sakamoto's algorithm, 0 = Sunday. Picked over Zeller because it has no
// negative-modulo edge cases to trip over. The year-- for Jan/Feb works on a
// local copy, callers' years are untouched. The t[] table encodes month
// offsets, do not "tidy" it.
int date_day_of_week(int year, int month, int day) {
    static const int t[MONTHS_PER_YEAR] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year--;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % DAYS_PER_WEEK;
}

int date_day_of_year(int year, int month, int day) {
    int total = day;
    for (int i = 1; i < month; i++) total += date_days_in_month(year, i);
    return total;
}

// A year has 53 ISO weeks when Jan 1 is a Thursday, or a Wednesday in a leap
// year. This p() form of the rule avoids needing the weekday of Dec 31.
static int iso_p(int year) {
    return (year + year / 4 - year / 100 + year / 400) % DAYS_PER_WEEK;
}

static int weeks_in_iso_year(int year) {
    int long_year = (iso_p(year) == P_THURSDAY_START || iso_p(year - 1) == P_WEDNESDAY_END);
    return long_year ? LONG_ISO_YEAR_WEEKS : SHORT_ISO_YEAR_WEEKS;
}

// Early January days can belong to last year's final week and late December
// days can already be week 1 of next year, so the two edge branches below are
// load-bearing for every year-boundary row.
int iso_week_number(int year, int month, int day) {
    int wd = date_day_of_week(year, month, day);
    int iso_wd = (wd == SUNDAY) ? ISO_SUNDAY : wd;
    int week = (date_day_of_year(year, month, day) - iso_wd + ISO_WEEK_SHIFT) / DAYS_PER_WEEK;

    if (week < 1) return weeks_in_iso_year(year - 1);
    if (week > weeks_in_iso_year(year)) return 1;
    return week;
}