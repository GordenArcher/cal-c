#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "cli/parse/parse.h"

static void test_month_by_number(void) {
    int m = 0;

    CHECK(parse_month("1", &m));  CHECK_INT(m, 1);
    CHECK(parse_month("10", &m)); CHECK_INT(m, 10);
    CHECK(parse_month("12", &m)); CHECK_INT(m, 12);

    CHECK(!parse_month("0", &m));
    CHECK(!parse_month("13", &m));
    CHECK(!parse_month("-1", &m));
}

static void test_month_by_name(void) {
    int m = 0;

    CHECK(parse_month("october", &m));  CHECK_INT(m, 10);
    CHECK(parse_month("OCTOBER", &m));  CHECK_INT(m, 10);   // case does not matter
    CHECK(parse_month("Oct", &m));      CHECK_INT(m, 10);
    CHECK(parse_month("octo", &m));     CHECK_INT(m, 10);
    CHECK(parse_month("jan", &m));      CHECK_INT(m, 1);
    CHECK(parse_month("december", &m)); CHECK_INT(m, 12);
}

// "ma" would be March or May, so prefixes under 3 letters are refused, and
// "mar" / "may" must each hit exactly the right month.
static void test_month_name_prefix_rules(void) {
    int m = 0;

    CHECK(!parse_month("ma", &m));
    CHECK(!parse_month("o", &m));
    CHECK(!parse_month("", &m));

    CHECK(parse_month("mar", &m)); CHECK_INT(m, 3);
    CHECK(parse_month("may", &m)); CHECK_INT(m, 5);
    CHECK(parse_month("jun", &m)); CHECK_INT(m, 6);
    CHECK(parse_month("jul", &m)); CHECK_INT(m, 7);

    CHECK(!parse_month("octobers", &m));   // longer than the name
    CHECK(!parse_month("abc", &m));
}

static void test_month_rejects_trailing_garbage(void) {
    int m = 0;

    CHECK(!parse_month("10abc", &m));
    CHECK(!parse_month("1 0", &m));
    CHECK(!parse_month("10.5", &m));
}

static void test_year_range(void) {
    int y = 0;

    CHECK(parse_year("2026", &y)); CHECK_INT(y, 2026);
    CHECK(parse_year("1", &y));    CHECK_INT(y, 1);
    CHECK(parse_year("9999", &y)); CHECK_INT(y, 9999);

    CHECK(!parse_year("0", &y));
    CHECK(!parse_year("10000", &y));
    CHECK(!parse_year("-5", &y));
    CHECK(!parse_year("", &y));
    CHECK(!parse_year("20x6", &y));
    CHECK(!parse_year("2026.0", &y));
    CHECK(!parse_year("99999999999999999999", &y));   // overflows long, must not wrap into range
}

int main(void) {
    RUN_TEST(test_month_by_number);
    RUN_TEST(test_month_by_name);
    RUN_TEST(test_month_name_prefix_rules);
    RUN_TEST(test_month_rejects_trailing_garbage);
    RUN_TEST(test_year_range);
    return test_report(__FILE__);
}