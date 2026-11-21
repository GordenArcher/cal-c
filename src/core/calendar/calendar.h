#ifndef CAL_CORE_CALENDAR_CALENDAR_H
#define CAL_CORE_CALENDAR_CALENDAR_H

enum {
    DAYS_PER_WEEK = 7,
    MONTHS_PER_YEAR = 12,
    MAX_YEAR = 9999,
    SUNDAY = 0,
    SATURDAY = 6
};

// Names live here because both sides need them: render prints them, cli
// parses user input against them. One copy means they can never disagree.
extern const char *const MONTH_NAMES[MONTHS_PER_YEAR];
extern const char *const DAY_NAMES[DAYS_PER_WEEK];

int date_is_leap(int year);
int date_days_in_month(int year, int month);

// 0 = Sunday ... 6 = Saturday
int date_day_of_week(int year, int month, int day);

// 1-based, Jan 1 is day 1
int date_day_of_year(int year, int month, int day);

// ISO 8601 week number, 1 to 53.
int iso_week_number(int year, int month, int day);

#endif