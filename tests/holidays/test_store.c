#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "events/model/event.h"
#include "holidays/store/store.h"

static HolidaysStatus load_text(const char *text, int year, EventList *out,
                                char *warnings, size_t cap) {
    char path[64];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, text);

    FILE *warn = tmpfile();
    HolidaysStatus status = holidays_load(path, year, out, warn);
    if (warnings) test_slurp(warn, warnings, cap);
    fclose(warn);
    remove(path);
    return status;
}

static void test_loads_fixed_dates_and_skips_other_years(void) {
    EventList list;
    eventlist_init(&list);
    char warnings[2048] = "";

    HolidaysStatus status = load_text(
        "# shared holidays\n"
        "\n"
        "12-25|Christmas\n"
        "2026-10-15|Offsite\n"
        "2027-01-01|Next year\n",
        2026, &list, warnings, sizeof warnings);

    CHECK_INT(status, HOLIDAYS_OK);
    CHECK_INT(list.count, 2);

    CHECK_STR(list.items[0].label, "Christmas");
    CHECK_INT(list.items[0].year, 2026);
    CHECK_INT(list.items[0].month, 12);
    CHECK_INT(list.items[0].day, 25);
    CHECK_INT(list.items[0].source_line, 3);

    CHECK_STR(list.items[1].label, "Offsite");
    CHECK_INT(list.items[1].month, 10);
    CHECK_INT(list.items[1].source_line, 4);

    CHECK_STR(warnings, "");      // a date for another year is not an error
    eventlist_free(&list);
}

static void test_expands_rules_for_the_requested_year(void) {
    EventList list;
    eventlist_init(&list);

    HolidaysStatus status = load_text(
        "easter|Easter\n"
        "4th-thu-11|Thanksgiving\n"
        "last-mon-5|Memorial Day\n"
        "5th-mon-2|Leap Monday\n",
        2026, &list, NULL, 0);

    CHECK_INT(status, HOLIDAYS_OK);
    CHECK_INT(list.count, 3);     // the 5th Monday of Feb does not exist

    CHECK_STR(list.items[0].label, "Easter");
    CHECK_INT(list.items[0].month, 4);
    CHECK_INT(list.items[0].day, 5);

    CHECK_STR(list.items[1].label, "Thanksgiving");
    CHECK_INT(list.items[1].month, 11);
    CHECK_INT(list.items[1].day, 26);

    CHECK_STR(list.items[2].label, "Memorial Day");
    CHECK_INT(list.items[2].month, 5);
    CHECK_INT(list.items[2].day, 25);

    eventlist_free(&list);
}

static void test_bad_lines_warn_and_do_not_stop_the_file(void) {
    EventList list;
    eventlist_init(&list);
    char warnings[2048] = "";

    // Enough 'a's to overflow the label without overflowing the line.
    char long_label[300];
    memset(long_label, 'a', 200);
    long_label[200] = '\0';

    char text[700];
    snprintf(text, sizeof text,
             "garbage\n"
             "13-01|Bad date\n"
             "04-05|\n"
             "04-06|%s\n"
             "04-07|Kept\n",
             long_label);

    HolidaysStatus status = load_text(text, 2026, &list, warnings, sizeof warnings);

    CHECK_INT(status, HOLIDAYS_OK);
    CHECK_INT(list.count, 1);
    CHECK_STR(list.items[0].label, "Kept");
    CHECK_INT(list.items[0].source_line, 5);

    CHECK(strstr(warnings, ":1: missing '|'") != NULL);
    CHECK(strstr(warnings, ":2: invalid date") != NULL);
    CHECK(strstr(warnings, ":3: empty label") != NULL);
    CHECK(strstr(warnings, ":4: label too long") != NULL);

    eventlist_free(&list);
}

static void test_overlong_line_is_discarded_and_warned(void) {
    EventList list;
    eventlist_init(&list);
    char warnings[2048] = "";

    char text[400];
    int written = snprintf(text, sizeof text, "04-06|");
    memset(text + written, 'a', 280);
    text[written + 280] = '\0';
    strcat(text, "\n04-07|Kept\n");

    HolidaysStatus status = load_text(text, 2026, &list, warnings, sizeof warnings);

    CHECK_INT(status, HOLIDAYS_OK);
    CHECK_INT(list.count, 1);
    CHECK_STR(list.items[0].label, "Kept");
    CHECK(strstr(warnings, ":1: line too long") != NULL);

    eventlist_free(&list);
}

static void test_missing_file(void) {
    EventList list;
    eventlist_init(&list);

    HolidaysStatus status = holidays_load("/tmp/cal_test_no_such_holidays_file",
                                          2026, &list, NULL);
    CHECK_INT(status, HOLIDAYS_NOT_FOUND);
    CHECK_INT(list.count, 0);

    eventlist_free(&list);
}

static void test_path_resolution(void) {
    char buf[256];
    char home[] = "/home/tester";

    setenv("HOME", home, 1);
    setenv("CAL_HOLIDAYS", "/data/holidays.txt", 1);

    CHECK(holidays_resolve_path(NULL, buf, sizeof buf));
    CHECK_STR(buf, "/data/holidays.txt");

    CHECK(holidays_resolve_path("mine.txt", buf, sizeof buf));
    CHECK_STR(buf, "mine.txt");

    setenv("CAL_HOLIDAYS", "", 1);        // empty means unset
    CHECK(holidays_resolve_path(NULL, buf, sizeof buf));
    CHECK_STR(buf, "/home/tester/.cal_holidays");

    unsetenv("HOME");
    CHECK(!holidays_resolve_path(NULL, buf, sizeof buf));

    char tiny[4];
    CHECK(!holidays_resolve_path("/a/very/long/path/holidays.txt", tiny, sizeof tiny));
}

int main(void) {
    RUN_TEST(test_loads_fixed_dates_and_skips_other_years);
    RUN_TEST(test_expands_rules_for_the_requested_year);
    RUN_TEST(test_bad_lines_warn_and_do_not_stop_the_file);
    RUN_TEST(test_overlong_line_is_discarded_and_warned);
    RUN_TEST(test_missing_file);
    RUN_TEST(test_path_resolution);
    return test_report(__FILE__);
}
