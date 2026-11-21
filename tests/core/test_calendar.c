#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "core/calendar/calendar.h"

enum {
    // Days from 0001-01-01 to 9999-12-31, taken from Python's date ordinals.
    // If the leap year rule is wrong anywhere in 9999 years, the walk below
    // lands on the wrong number of steps.
    DAYS_IN_FULL_RANGE = 3652058
};

static void test_leap_years(void) {
    CHECK(date_is_leap(2000));
    CHECK(date_is_leap(2024));
    CHECK(date_is_leap(1600));
    CHECK(date_is_leap(4));

    CHECK(!date_is_leap(1900));   // century, not divisible by 400
    CHECK(!date_is_leap(2100));
    CHECK(!date_is_leap(2023));
    CHECK(!date_is_leap(2026));
}

static void test_days_in_month(void) {
    CHECK_INT(date_days_in_month(2024, 2), 29);
    CHECK_INT(date_days_in_month(2023, 2), 28);
    CHECK_INT(date_days_in_month(1900, 2), 28);
    CHECK_INT(date_days_in_month(2000, 2), 29);

    CHECK_INT(date_days_in_month(2026, 1), 31);
    CHECK_INT(date_days_in_month(2026, 4), 30);
    CHECK_INT(date_days_in_month(2026, 12), 31);
}

// Expected weekdays come from Python's datetime, not from this code.
static void test_day_of_week_known_dates(void) {
    CHECK_INT(date_day_of_week(2000, 1, 1), 6);     // Saturday
    CHECK_INT(date_day_of_week(1970, 1, 1), 4);     // Thursday, the Unix epoch
    CHECK_INT(date_day_of_week(2026, 10, 1), 4);
    CHECK_INT(date_day_of_week(2024, 2, 29), 4);
    CHECK_INT(date_day_of_week(2026, 11, 21), 6);
    CHECK_INT(date_day_of_week(1900, 1, 1), 1);     // Monday
    CHECK_INT(date_day_of_week(2100, 3, 1), 1);
    CHECK_INT(date_day_of_week(1600, 2, 29), 2);
    CHECK_INT(date_day_of_week(1, 1, 1), 1);        // lower bound of the range
    CHECK_INT(date_day_of_week(9999, 12, 31), 5);   // upper bound of the range
}

static void test_day_of_year(void) {
    CHECK_INT(date_day_of_year(2026, 1, 1), 1);
    CHECK_INT(date_day_of_year(2026, 3, 1), 60);
    CHECK_INT(date_day_of_year(2023, 12, 31), 365);
    CHECK_INT(date_day_of_year(2024, 12, 31), 366);
}

// The cases that matter most: days near New Year that belong to the other
// year's week numbering.
static void test_iso_week_known_dates(void) {
    CHECK_INT(iso_week_number(2026, 1, 1), 1);
    CHECK_INT(iso_week_number(2026, 6, 15), 25);
    CHECK_INT(iso_week_number(2026, 10, 4), 40);    // Sunday closes week 40
    CHECK_INT(iso_week_number(2026, 10, 5), 41);    // Monday opens week 41

    CHECK_INT(iso_week_number(2020, 12, 31), 53);
    CHECK_INT(iso_week_number(2021, 1, 1), 53);     // Jan 1 still in last year's week
    CHECK_INT(iso_week_number(2021, 1, 3), 53);
    CHECK_INT(iso_week_number(2021, 1, 4), 1);

    CHECK_INT(iso_week_number(2024, 12, 30), 1);    // Dec 30 already in next year's week 1
    CHECK_INT(iso_week_number(2018, 12, 31), 1);
    CHECK_INT(iso_week_number(2015, 12, 31), 53);
    CHECK_INT(iso_week_number(2016, 1, 1), 53);
    CHECK_INT(iso_week_number(2026, 12, 31), 53);
    CHECK_INT(iso_week_number(2027, 1, 1), 53);

    CHECK_INT(iso_week_number(1, 1, 1), 1);
    CHECK_INT(iso_week_number(9999, 12, 31), 52);
}

static void next_day(int *year, int *month, int *day) {
    if (*day < date_days_in_month(*year, *month)) {
        (*day)++;
        return;
    }
    *day = 1;
    if (*month < MONTHS_PER_YEAR) {
        (*month)++;
        return;
    }
    *month = 1;
    (*year)++;
}

// Instead of trusting a handful of examples, walk every single day of the
// supported range and check the rules that must hold between neighbours.
static void test_walking_every_day_stays_consistent(void) {
    int year = 1, month = 1, day = 1;
    int prev_dow = date_day_of_week(year, month, day);
    int prev_week = iso_week_number(year, month, day);
    int prev_yday = date_day_of_year(year, month, day);

    long steps = 0;
    long bad_weekday = 0, bad_week = 0, bad_yday = 0;

    while (!(year == MAX_YEAR && month == MONTHS_PER_YEAR && day == 31)) {
        next_day(&year, &month, &day);
        steps++;

        int dow = date_day_of_week(year, month, day);
        int week = iso_week_number(year, month, day);
        int yday = date_day_of_year(year, month, day);

        if (dow != (prev_dow + 1) % DAYS_PER_WEEK) bad_weekday++;

        // Monday starts a new ISO week: either the next number, or week 1
        // right after a year's last week (52 or 53). Every other day keeps
        // the week number of the day before it.
        if (dow == 1) {
            int rolled_over = (week == 1 && (prev_week == 52 || prev_week == 53));
            if (week != prev_week + 1 && !rolled_over) bad_week++;
        } else if (week != prev_week) {
            bad_week++;
        }

        int expected_yday = (month == 1 && day == 1) ? 1 : prev_yday + 1;
        if (yday != expected_yday) bad_yday++;

        prev_dow = dow;
        prev_week = week;
        prev_yday = yday;
    }

    CHECK_INT(steps, DAYS_IN_FULL_RANGE);
    CHECK_INT(bad_weekday, 0);
    CHECK_INT(bad_week, 0);
    CHECK_INT(bad_yday, 0);
}

static void test_names(void) {
    CHECK_STR(MONTH_NAMES[0], "January");
    CHECK_STR(MONTH_NAMES[11], "December");
    CHECK_STR(DAY_NAMES[SUNDAY], "Su");
    CHECK_STR(DAY_NAMES[SATURDAY], "Sa");
}

int main(void) {
    RUN_TEST(test_leap_years);
    RUN_TEST(test_days_in_month);
    RUN_TEST(test_day_of_week_known_dates);
    RUN_TEST(test_day_of_year);
    RUN_TEST(test_iso_week_known_dates);
    RUN_TEST(test_walking_every_day_stays_consistent);
    RUN_TEST(test_names);
    return test_report(__FILE__);
}