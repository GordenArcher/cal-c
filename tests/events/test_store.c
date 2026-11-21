#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "events/model/event.h"
#include "events/store/store.h"

enum {
    PATH_BUF = 128,
    OUTPUT_BUF = 4096,
    LONG_LINE = 300     // longer than FORMAT_LINE_CAP on purpose
};

static void test_load_keeps_good_lines_and_reports_bad_ones(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));

    test_write_file(path,
        "# my events\n"                      // 1
        "2026-12-25|Christmas\n"             // 2
        "12-31 | New Year's Eve party \n"    // 3
        "\n"                                 // 4
        "this line has no separator\n"       // 5
        "2026-13-01|Bad month\n"             // 6
        "2026-10-15|\n"                      // 7
        "2027-02-30|Impossible day\n"        // 8
        "10-31|Halloween\n");                // 9

    EventList list;
    eventlist_init(&list);
    FILE *warn = tmpfile();
    char out[OUTPUT_BUF];

    CHECK_INT(store_load(path, &list, warn), STORE_OK);
    test_slurp(warn, out, sizeof out);

    CHECK_INT(list.count, 3);
    CHECK_STR(list.items[0].label, "Christmas");
    CHECK_STR(list.items[1].label, "New Year's Eve party");
    CHECK_INT(list.items[1].year, EVERY_YEAR);
    CHECK_STR(list.items[2].label, "Halloween");

    // Exactly the four bad lines are reported, with their line numbers.
    CHECK_INT(test_count_char(out, '\n'), 4);
    CHECK(strstr(out, ":5:") != NULL);
    CHECK(strstr(out, ":6:") != NULL);
    CHECK(strstr(out, ":7:") != NULL);
    CHECK(strstr(out, ":8:") != NULL);
    CHECK(strstr(out, ":2:") == NULL);
    CHECK(strstr(out, ":9:") == NULL);

    fclose(warn);
    eventlist_free(&list);
    remove(path);
}

static void test_load_without_a_warning_stream_stays_quiet(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "garbage\n12-25|Christmas\n");

    EventList list;
    eventlist_init(&list);

    CHECK_INT(store_load(path, &list, NULL), STORE_OK);
    CHECK_INT(list.count, 1);

    eventlist_free(&list);
    remove(path);
}

static void test_missing_file_is_not_found(void) {
    EventList list;
    eventlist_init(&list);

    CHECK_INT(store_load("/tmp/cal_test_does_not_exist_anywhere", &list, NULL), STORE_NOT_FOUND);
    CHECK_INT(list.count, 0);

    eventlist_free(&list);
}

static void test_last_line_without_newline_is_read(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "2026-01-01|First\n2026-01-02|Second");

    EventList list;
    eventlist_init(&list);

    CHECK_INT(store_load(path, &list, NULL), STORE_OK);
    CHECK_INT(list.count, 2);
    CHECK_STR(list.items[1].label, "Second");

    eventlist_free(&list);
    remove(path);
}

// A line that overflows the read buffer must be dropped as ONE bad line.
// Without draining the rest, its tail would be parsed as a second line.
static void test_overlong_line_is_skipped_once(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));

    char text[LONG_LINE + 64];
    char filler[LONG_LINE + 1];
    memset(filler, 'x', LONG_LINE);
    filler[LONG_LINE] = '\0';
    snprintf(text, sizeof text, "%s\n12-25|After the long line\n", filler);
    test_write_file(path, text);

    EventList list;
    eventlist_init(&list);
    FILE *warn = tmpfile();
    char out[OUTPUT_BUF];

    CHECK_INT(store_load(path, &list, warn), STORE_OK);
    test_slurp(warn, out, sizeof out);

    CHECK_INT(list.count, 1);
    CHECK_STR(list.items[0].label, "After the long line");
    CHECK_INT(test_count_char(out, '\n'), 1);
    CHECK(strstr(out, "too long") != NULL);
    CHECK(strstr(out, ":1:") != NULL);

    fclose(warn);
    eventlist_free(&list);
    remove(path);
}

static void test_append_creates_the_file_and_keeps_order(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    remove(path);   // start from "no file at all"

    Event first = {2026, 12, 25, "Christmas"};
    Event second = {EVERY_YEAR, 10, 31, "Halloween"};

    CHECK_INT(store_append(path, &first), STORE_OK);
    CHECK_INT(store_append(path, &second), STORE_OK);

    EventList list;
    eventlist_init(&list);
    FILE *warn = tmpfile();
    char out[OUTPUT_BUF];

    CHECK_INT(store_load(path, &list, warn), STORE_OK);
    test_slurp(warn, out, sizeof out);

    CHECK_INT(list.count, 2);
    CHECK_STR(list.items[0].label, "Christmas");
    CHECK_INT(list.items[1].year, EVERY_YEAR);
    CHECK_STR(list.items[1].label, "Halloween");
    CHECK_STR(out, "");

    fclose(warn);
    eventlist_free(&list);
    remove(path);
}

// A hand-edited file may end without a newline. Appending must not glue the
// new event onto the last one.
static void test_append_after_a_file_with_no_trailing_newline(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "2026-01-01|First");

    Event second = {2026, 1, 2, "Second"};
    CHECK_INT(store_append(path, &second), STORE_OK);

    EventList list;
    eventlist_init(&list);
    FILE *warn = tmpfile();
    char out[OUTPUT_BUF];

    CHECK_INT(store_load(path, &list, warn), STORE_OK);
    test_slurp(warn, out, sizeof out);

    CHECK_INT(list.count, 2);
    CHECK_STR(list.items[0].label, "First");
    CHECK_STR(list.items[1].label, "Second");
    CHECK_STR(out, "");

    fclose(warn);
    eventlist_free(&list);
    remove(path);
}

static void test_append_to_an_unwritable_place_fails(void) {
    Event e = {2026, 12, 25, "Christmas"};

    CHECK_INT(store_append("/tmp/cal_test_no_such_dir/events.txt", &e), STORE_ERROR);
}

static void test_resolve_path_precedence(void) {
    char buf[STORE_PATH_CAP];

    unsetenv("CAL_EVENTS");
    setenv("HOME", "/home/tester", 1);

    // Nothing else set: the default file in $HOME.
    CHECK(store_resolve_path(NULL, buf, sizeof buf));
    CHECK_STR(buf, "/home/tester/.cal_events");

    // $CAL_EVENTS beats the default.
    setenv("CAL_EVENTS", "/data/events.txt", 1);
    CHECK(store_resolve_path(NULL, buf, sizeof buf));
    CHECK_STR(buf, "/data/events.txt");

    // An explicit path beats both.
    CHECK(store_resolve_path("mine.txt", buf, sizeof buf));
    CHECK_STR(buf, "mine.txt");

    // An empty variable counts as unset.
    setenv("CAL_EVENTS", "", 1);
    CHECK(store_resolve_path(NULL, buf, sizeof buf));
    CHECK_STR(buf, "/home/tester/.cal_events");
}

static void test_resolve_path_failure_cases(void) {
    char buf[STORE_PATH_CAP];
    char tiny[8];

    unsetenv("CAL_EVENTS");
    unsetenv("HOME");
    CHECK(!store_resolve_path(NULL, buf, sizeof buf));   // nowhere to look

    // A path that does not fit must be refused, never silently cut short.
    CHECK(!store_resolve_path("/a/very/long/path/to/events.txt", tiny, sizeof tiny));
}

int main(void) {
    RUN_TEST(test_load_keeps_good_lines_and_reports_bad_ones);
    RUN_TEST(test_load_without_a_warning_stream_stays_quiet);
    RUN_TEST(test_missing_file_is_not_found);
    RUN_TEST(test_last_line_without_newline_is_read);
    RUN_TEST(test_overlong_line_is_skipped_once);
    RUN_TEST(test_append_creates_the_file_and_keeps_order);
    RUN_TEST(test_append_after_a_file_with_no_trailing_newline);
    RUN_TEST(test_append_to_an_unwritable_place_fails);
    RUN_TEST(test_resolve_path_precedence);
    RUN_TEST(test_resolve_path_failure_cases);
    return test_report(__FILE__);
}