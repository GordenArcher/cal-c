#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "holidays/rules/rules.h"

static RuleStatus resolve(const char *text, int year, int *month, int *day) {
    *month = 0;
    *day = 0;
    return rule_resolve(text, year, month, day);
}

static void test_fixed_dates(void) {
    int month, day;

    CHECK_INT(resolve("10-31", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 10);
    CHECK_INT(day, 31);

    CHECK_INT(resolve("2026-10-31", 2026, &month, &day), RULE_OK);

    // A full date is pinned to its year, a MM-DD repeats.
    CHECK_INT(resolve("2027-10-31", 2026, &month, &day), RULE_SKIP);
    CHECK_INT(resolve("02-29", 2024, &month, &day), RULE_OK);
    CHECK_INT(resolve("02-29", 2025, &month, &day), RULE_SKIP);

    CHECK_INT(resolve("2026-02-29", 2026, &month, &day), RULE_BAD_DATE);
    CHECK_INT(resolve("13-01", 2026, &month, &day), RULE_BAD_DATE);
    CHECK_INT(resolve("00-10", 2026, &month, &day), RULE_BAD_DATE);

    // A date split with the wrong separators is still a date mistake.
    CHECK_INT(resolve("2026-1-1", 2026, &month, &day), RULE_BAD_DATE);
}

static void test_easter(void) {
    int month, day;

    // Known Gregorian Easter Sundays.
    CHECK_INT(resolve("easter", 2024, &month, &day), RULE_OK);
    CHECK_INT(month, 3);
    CHECK_INT(day, 31);

    CHECK_INT(resolve("easter", 2025, &month, &day), RULE_OK);
    CHECK_INT(month, 4);
    CHECK_INT(day, 20);

    CHECK_INT(resolve("easter", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 4);
    CHECK_INT(day, 5);

    CHECK_INT(resolve("easter", 2027, &month, &day), RULE_OK);
    CHECK_INT(month, 3);
    CHECK_INT(day, 28);

    CHECK_INT(resolve("easter", 2038, &month, &day), RULE_OK);
    CHECK_INT(month, 4);
    CHECK_INT(day, 25);
}

static void test_easter_offsets(void) {
    int month, day;

    CHECK_INT(resolve("easter+1", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 4);
    CHECK_INT(day, 6);

    CHECK_INT(resolve("easter-2", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 4);
    CHECK_INT(day, 3);

    // Crosses back over a month boundary.
    CHECK_INT(resolve("easter-32", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 3);
    CHECK_INT(day, 4);

    CHECK_INT(resolve("easter+50", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 5);
    CHECK_INT(day, 25);

    // Still in the requested year.
    CHECK_INT(resolve("easter-90", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 1);
    CHECK_INT(day, 5);

    // This one lands in December 2025: not a 2026 holiday.
    CHECK_INT(resolve("easter-100", 2026, &month, &day), RULE_SKIP);

    CHECK_INT(resolve("easter+", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("easterx", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("easter+101", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("easter+9999", 2026, &month, &day), RULE_BAD_RULE);
}

static void test_nth_weekday(void) {
    int month, day;

    CHECK_INT(resolve("4th-thu-11", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 11);
    CHECK_INT(day, 26);

    CHECK_INT(resolve("1st-mon-9", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 9);
    CHECK_INT(day, 7);

    CHECK_INT(resolve("5th-sat-1", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 1);
    CHECK_INT(day, 31);

    // A 5th Monday that February does not have is not an error, just absent.
    CHECK_INT(resolve("5th-mon-2", 2026, &month, &day), RULE_SKIP);

    // Weekday names are case-insensitive and may be spelled out.
    CHECK_INT(resolve("4th-THU-11", 2026, &month, &day), RULE_OK);
    CHECK_INT(resolve("4th-thursday-11", 2026, &month, &day), RULE_OK);

    CHECK_INT(resolve("6th-mon-1", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("4th-xxx-11", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("1st-mon-13", 2026, &month, &day), RULE_BAD_DATE);
}

static void test_last_weekday(void) {
    int month, day;

    CHECK_INT(resolve("last-mon-5", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 5);
    CHECK_INT(day, 25);

    CHECK_INT(resolve("last-sun-3", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 3);
    CHECK_INT(day, 29);

    CHECK_INT(resolve("last-fri-12", 2026, &month, &day), RULE_OK);
    CHECK_INT(month, 12);
    CHECK_INT(day, 25);
}

static void test_garbage(void) {
    int month, day;

    CHECK_INT(resolve("", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("hello", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("4th-thu", 2026, &month, &day), RULE_BAD_RULE);
    CHECK_INT(resolve("4th-thu-11-extra", 2026, &month, &day), RULE_BAD_RULE);
}

int main(void) {
    RUN_TEST(test_fixed_dates);
    RUN_TEST(test_easter);
    RUN_TEST(test_easter_offsets);
    RUN_TEST(test_nth_weekday);
    RUN_TEST(test_last_weekday);
    RUN_TEST(test_garbage);
    return test_report(__FILE__);
}
