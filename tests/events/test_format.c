#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "events/format/format.h"
#include "events/model/event.h"

static void test_parse_date_accepts_both_forms(void) {
    Event e;

    CHECK_INT(format_parse_date("2026-12-25", &e), FORMAT_OK);
    CHECK_INT(e.year, 2026);
    CHECK_INT(e.month, 12);
    CHECK_INT(e.day, 25);

    CHECK_INT(format_parse_date("12-25", &e), FORMAT_OK);
    CHECK_INT(e.year, EVERY_YEAR);
    CHECK_INT(e.month, 12);
    CHECK_INT(e.day, 25);

    CHECK_INT(format_parse_date("9999-12-31", &e), FORMAT_OK);
}

static void test_parse_date_checks_the_calendar(void) {
    Event e;

    CHECK_INT(format_parse_date("2024-02-29", &e), FORMAT_OK);        // leap year
    CHECK_INT(format_parse_date("2023-02-29", &e), FORMAT_BAD_DATE);  // not a leap year
    CHECK_INT(format_parse_date("2100-02-29", &e), FORMAT_BAD_DATE);  // century rule
    CHECK_INT(format_parse_date("02-29", &e), FORMAT_OK);             // yearly, leap day allowed
    CHECK_INT(format_parse_date("02-30", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("04-31", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("2026-13-01", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("13-01", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("2026-00-10", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("2026-10-00", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("0000-01-01", &e), FORMAT_BAD_DATE);  // year 0 is not valid
}

static void test_parse_date_rejects_wrong_shapes(void) {
    Event e;

    CHECK_INT(format_parse_date("", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("2026-1-05", &e), FORMAT_BAD_DATE);   // no zero padding
    CHECK_INT(format_parse_date("20261005", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("2026/10/05", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("2026-10-5x", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("ab-cd", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("12-2", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_date("+2-25", &e), FORMAT_BAD_DATE);       // strtol would take this, we must not
}

static void test_parse_line_ok(void) {
    Event e;

    CHECK_INT(format_parse_line("2026-12-25|Christmas", &e), FORMAT_OK);
    CHECK_INT(e.year, 2026);
    CHECK_STR(e.label, "Christmas");

    CHECK_INT(format_parse_line("12-31|New Year's Eve", &e), FORMAT_OK);
    CHECK_INT(e.year, EVERY_YEAR);
    CHECK_STR(e.label, "New Year's Eve");
}

static void test_parse_line_trims_whitespace(void) {
    Event e;

    CHECK_INT(format_parse_line("  12-31 |  Party time  \n", &e), FORMAT_OK);
    CHECK_STR(e.label, "Party time");

    // Windows line endings must not end up inside the label.
    CHECK_INT(format_parse_line("10-31|Halloween\r\n", &e), FORMAT_OK);
    CHECK_STR(e.label, "Halloween");
}

static void test_parse_line_skips_blanks_and_comments(void) {
    Event e;

    CHECK_INT(format_parse_line("", &e), FORMAT_SKIP);
    CHECK_INT(format_parse_line("\n", &e), FORMAT_SKIP);
    CHECK_INT(format_parse_line("   \t \n", &e), FORMAT_SKIP);
    CHECK_INT(format_parse_line("# 2026-12-25|not an event", &e), FORMAT_SKIP);
    CHECK_INT(format_parse_line("   # indented comment", &e), FORMAT_SKIP);
}

static void test_parse_line_reports_each_kind_of_error(void) {
    Event e;

    CHECK_INT(format_parse_line("no separator here", &e), FORMAT_NO_SEPARATOR);
    CHECK_INT(format_parse_line("2026-13-01|x", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_line("|label without date", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_line("2026-10-15-extra|x", &e), FORMAT_BAD_DATE);
    CHECK_INT(format_parse_line("2026-10-15|", &e), FORMAT_EMPTY_LABEL);
    CHECK_INT(format_parse_line("2026-10-15|   \n", &e), FORMAT_EMPTY_LABEL);
}

static void test_parse_line_only_splits_on_the_first_pipe(void) {
    Event e;

    CHECK_INT(format_parse_line("10-31|trick|treat", &e), FORMAT_OK);
    CHECK_STR(e.label, "trick|treat");
}

static void test_parse_line_label_length_limit(void) {
    char line[FORMAT_LINE_CAP + 64];
    char label[EVENT_LABEL_CAP + 8];
    Event e;

    // The longest label that fits is CAP - 1 characters, one more is refused.
    memset(label, 'x', EVENT_LABEL_CAP - 1);
    label[EVENT_LABEL_CAP - 1] = '\0';
    snprintf(line, sizeof line, "10-31|%s", label);
    CHECK_INT(format_parse_line(line, &e), FORMAT_OK);
    CHECK_INT(strlen(e.label), EVENT_LABEL_CAP - 1);

    memset(label, 'x', EVENT_LABEL_CAP);
    label[EVENT_LABEL_CAP] = '\0';
    snprintf(line, sizeof line, "10-31|%s", label);
    CHECK_INT(format_parse_line(line, &e), FORMAT_LABEL_TOO_LONG);
}

static void test_format_line(void) {
    char buf[FORMAT_LINE_CAP];

    Event yearly = {.year = EVERY_YEAR, .month = 12, .day = 25, .label = "Christmas"};
    CHECK_INT(format_line(&yearly, buf, sizeof buf), 15);
    CHECK_STR(buf, "12-25|Christmas");

    Event once = {.year = 2026, .month = 1, .day = 5, .label = "Dentist"};
    format_line(&once, buf, sizeof buf);
    CHECK_STR(buf, "2026-01-05|Dentist");   // zero padded

    Event early = {.year = 5, .month = 1, .day = 1, .label = "x"};
    format_line(&early, buf, sizeof buf);
    CHECK_STR(buf, "0005-01-01|x");         // year padded to 4 digits so it parses back
}

static void test_format_line_refuses_to_truncate(void) {
    char small[8];
    Event e = {.year = 2026, .month = 12, .day = 25, .label = "Christmas"};

    CHECK_INT(format_line(&e, small, sizeof small), -1);
}

static void test_format_then_parse_round_trips(void) {
    Event originals[] = {
        {.year = EVERY_YEAR, .month = 2, .day = 29, .label = "Leap day"},
        {.year = 2026, .month = 10, .day = 15, .label = "Team offsite"},
        {.year = 1, .month = 1, .day = 1, .label = "First day"},
        {.year = 9999, .month = 12, .day = 31, .label = "Last day"},
        {.year = EVERY_YEAR, .month = 10, .day = 31, .label = "trick|treat"},
    };

    for (size_t i = 0; i < sizeof originals / sizeof originals[0]; i++) {
        char buf[FORMAT_LINE_CAP];
        Event back;

        CHECK(format_line(&originals[i], buf, sizeof buf) > 0);
        CHECK_INT(format_parse_line(buf, &back), FORMAT_OK);
        CHECK_INT(back.year, originals[i].year);
        CHECK_INT(back.month, originals[i].month);
        CHECK_INT(back.day, originals[i].day);
        CHECK_STR(back.label, originals[i].label);
    }
}

static void test_every_status_has_a_message(void) {
    FormatStatus all[] = {
        FORMAT_OK, FORMAT_SKIP, FORMAT_NO_SEPARATOR,
        FORMAT_BAD_DATE, FORMAT_EMPTY_LABEL, FORMAT_LABEL_TOO_LONG
    };

    for (size_t i = 0; i < sizeof all / sizeof all[0]; i++) {
        const char *msg = format_status_message(all[i]);
        CHECK(msg != NULL && msg[0] != '\0');
    }
}

int main(void) {
    RUN_TEST(test_parse_date_accepts_both_forms);
    RUN_TEST(test_parse_date_checks_the_calendar);
    RUN_TEST(test_parse_date_rejects_wrong_shapes);
    RUN_TEST(test_parse_line_ok);
    RUN_TEST(test_parse_line_trims_whitespace);
    RUN_TEST(test_parse_line_skips_blanks_and_comments);
    RUN_TEST(test_parse_line_reports_each_kind_of_error);
    RUN_TEST(test_parse_line_only_splits_on_the_first_pipe);
    RUN_TEST(test_parse_line_label_length_limit);
    RUN_TEST(test_format_line);
    RUN_TEST(test_format_line_refuses_to_truncate);
    RUN_TEST(test_format_then_parse_round_trips);
    RUN_TEST(test_every_status_has_a_message);
    return test_report(__FILE__);
}