#define _POSIX_C_SOURCE 200809L

#include <sys/stat.h>

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

    Event first = {.year = 2026, .month = 12, .day = 25, .label = "Christmas"};
    Event second = {.year = EVERY_YEAR, .month = 10, .day = 31, .label = "Halloween"};

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

    Event second = {.year = 2026, .month = 1, .day = 2, .label = "Second"};
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
    Event e = {.year = 2026, .month = 12, .day = 25, .label = "Christmas"};

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

// ---------------------------------------------------------------------------
// store_remove
// ---------------------------------------------------------------------------

static void read_whole_file(const char *path, char *buf, size_t cap) {
    FILE *f = fopen(path, "r");
    size_t n = f ? fread(buf, 1, cap - 1, f) : 0;
    buf[n] = '\0';
    if (f) fclose(f);
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) fclose(f);
    return f != NULL;
}

static void test_load_records_the_line_each_event_came_from(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "# comment\n2026-12-25|A\n\nnot an event\n12-31|B\n");

    EventList list;
    eventlist_init(&list);
    CHECK_INT(store_load(path, &list, NULL), STORE_OK);

    CHECK_INT(list.count, 2);
    CHECK_INT(list.items[0].source_line, 2);
    CHECK_INT(list.items[1].source_line, 5);    // blank and broken lines still count

    eventlist_free(&list);
    remove(path);
}

static void test_remove_drops_one_line_and_keeps_every_other_byte(void) {
    char path[PATH_BUF], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path,
        "# my events\n"                 // 1
        "2026-12-25|Christmas\n"        // 2
        "\n"                            // 3
        "   12-31 | spaced   \n"        // 4
        "broken line\n"                 // 5
        "10-31|Halloween\n");           // 6

    Event removed;
    CHECK_INT(store_remove(path, 4, &removed), REMOVE_OK);

    CHECK_STR(removed.label, "spaced");
    CHECK_INT(removed.year, EVERY_YEAR);
    CHECK_INT(removed.month, 12);
    CHECK_INT(removed.day, 31);
    CHECK_INT(removed.source_line, 4);

    // Comments, the blank line and the typo survive untouched.
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out,
        "# my events\n"
        "2026-12-25|Christmas\n"
        "\n"
        "broken line\n"
        "10-31|Halloween\n");

    remove(path);
}

static void test_remove_without_asking_what_was_removed(void) {
    char path[PATH_BUF], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "2026-01-01|A\n2026-01-02|B\n");

    CHECK_INT(store_remove(path, 1, NULL), REMOVE_OK);
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, "2026-01-02|B\n");

    remove(path);
}

// Anything that is not a real event must be refused, and the file must come
// out byte for byte the same. Deleting someone's comment by mistake is worse
// than refusing.
static void test_remove_refuses_lines_that_are_not_events(void) {
    const char *content =
        "# a comment\n"                 // 1
        "\n"                            // 2
        "broken line\n"                 // 3
        "2026-13-01|bad month\n"        // 4
        "2026-01-01|Real\n";            // 5
    int refused[] = {1, 2, 3, 4, 0, -3, 6, 99};

    char path[PATH_BUF], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, content);

    for (size_t i = 0; i < sizeof refused / sizeof refused[0]; i++) {
        CHECK_INT(store_remove(path, refused[i], NULL), REMOVE_NO_SUCH_EVENT);
        read_whole_file(path, out, sizeof out);
        CHECK_STR(out, content);
    }

    remove(path);
}

static void test_remove_from_a_missing_file(void) {
    CHECK_INT(store_remove("/tmp/cal_test_does_not_exist_anywhere", 1, NULL), REMOVE_FILE_NOT_FOUND);
}

static void test_remove_when_the_last_line_has_no_newline(void) {
    char path[PATH_BUF], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));

    test_write_file(path, "2026-01-01|A\n2026-01-02|B");
    CHECK_INT(store_remove(path, 2, NULL), REMOVE_OK);
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, "2026-01-01|A\n");

    // Removing an earlier line must not invent a newline the file never had.
    test_write_file(path, "2026-01-01|A\n2026-01-02|B");
    CHECK_INT(store_remove(path, 1, NULL), REMOVE_OK);
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, "2026-01-02|B");

    remove(path);
}

static void test_remove_keeps_windows_line_endings(void) {
    char path[PATH_BUF], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "2026-01-01|A\r\n2026-01-02|B\r\n");

    Event removed;
    CHECK_INT(store_remove(path, 1, &removed), REMOVE_OK);
    CHECK_STR(removed.label, "A");              // no stray \r in the label
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, "2026-01-02|B\r\n");

    remove(path);
}

// A line longer than the read buffer arrives in pieces. It must still count
// as ONE line, stay intact when it is not the target, and never be removable.
static void test_remove_handles_overlong_lines(void) {
    char filler[LONG_LINE + 1];
    memset(filler, 'x', LONG_LINE);
    filler[LONG_LINE] = '\0';

    char content[LONG_LINE + 128];
    snprintf(content, sizeof content, "%s\n2026-01-01|A\n12-25|B\n", filler);

    char path[PATH_BUF], out[OUTPUT_BUF * 2];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, content);

    // "list" and "remove" must agree on the numbers, so ask the loader.
    EventList list;
    eventlist_init(&list);
    CHECK_INT(store_load(path, &list, NULL), STORE_OK);
    CHECK_INT(list.count, 2);
    CHECK_INT(list.items[0].source_line, 2);
    eventlist_free(&list);

    CHECK_INT(store_remove(path, 1, NULL), REMOVE_NO_SUCH_EVENT);   // the overlong line itself
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, content);

    CHECK_INT(store_remove(path, 2, NULL), REMOVE_OK);
    snprintf(content, sizeof content, "%s\n12-25|B\n", filler);
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, content);

    remove(path);
}

static void test_remove_follows_a_symlink_instead_of_replacing_it(void) {
    char real[PATH_BUF], link[PATH_BUF + 8], out[OUTPUT_BUF];
    CHECK(test_make_temp(real, sizeof real));
    snprintf(link, sizeof link, "%s.link", real);
    test_write_file(real, "2026-01-01|A\n2026-01-02|B\n");
    CHECK(symlink(real, link) == 0);

    CHECK_INT(store_remove(link, 1, NULL), REMOVE_OK);

    struct stat st;
    CHECK(lstat(link, &st) == 0);
    CHECK(S_ISLNK(st.st_mode));                 // still a link, not a regular file now
    read_whole_file(real, out, sizeof out);
    CHECK_STR(out, "2026-01-02|B\n");           // the file it points to was updated

    remove(link);
    remove(real);
}

static void test_remove_keeps_the_file_permissions(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "2026-01-01|A\n2026-01-02|B\n");
    CHECK(chmod(path, 0640) == 0);              // not what a new file would get by default

    CHECK_INT(store_remove(path, 1, NULL), REMOVE_OK);

    struct stat st;
    CHECK(stat(path, &st) == 0);
    CHECK_INT(st.st_mode & 0777, 0640);

    remove(path);
}

static void test_remove_never_leaves_a_temp_file_behind(void) {
    char path[PATH_BUF], temp[PATH_BUF + 8];
    CHECK(test_make_temp(path, sizeof path));
    snprintf(temp, sizeof temp, "%s.tmp", path);
    test_write_file(path, "# note\n2026-01-01|A\n");

    CHECK_INT(store_remove(path, 2, NULL), REMOVE_OK);
    CHECK(!file_exists(temp));

    CHECK_INT(store_remove(path, 1, NULL), REMOVE_NO_SUCH_EVENT);   // the comment
    CHECK(!file_exists(temp));

    remove(path);
}

// Removing from the front over and over walks the whole file, and the result
// has to be an empty file that still works.
static void test_remove_until_the_file_is_empty(void) {
    char path[PATH_BUF], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));
    remove(path);

    Event a = {.year = 2026, .month = 1, .day = 1, .label = "A"}, b = {.year = 2026, .month = 1, .day = 2, .label = "B"}, c = {.year = EVERY_YEAR, .month = 1, .day = 3, .label = "C"};
    CHECK_INT(store_append(path, &a), STORE_OK);
    CHECK_INT(store_append(path, &b), STORE_OK);
    CHECK_INT(store_append(path, &c), STORE_OK);

    Event removed;
    CHECK_INT(store_remove(path, 1, &removed), REMOVE_OK);
    CHECK_STR(removed.label, "A");
    CHECK_INT(store_remove(path, 1, &removed), REMOVE_OK);
    CHECK_STR(removed.label, "B");
    CHECK_INT(store_remove(path, 1, &removed), REMOVE_OK);
    CHECK_STR(removed.label, "C");

    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, "");
    CHECK_INT(store_remove(path, 1, NULL), REMOVE_NO_SUCH_EVENT);

    remove(path);
}

// "2026-01-01|A" followed by spaces reads as a valid event in its first piece,
// but the whole line is too long and the loader rejects it. remove has to
// agree, or "list" and "remove" would disagree about what an event is.
static void test_remove_agrees_with_load_about_overlong_lines(void) {
    char pad[LONG_LINE + 1];
    memset(pad, ' ', LONG_LINE);
    pad[LONG_LINE] = '\0';

    char content[LONG_LINE + 64], path[PATH_BUF], out[OUTPUT_BUF * 2];
    snprintf(content, sizeof content, "2026-01-01|A%s\n2026-01-02|B\n", pad);
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, content);

    EventList list;
    eventlist_init(&list);
    CHECK_INT(store_load(path, &list, NULL), STORE_OK);
    CHECK_INT(list.count, 1);                       // only B counts as an event
    CHECK_INT(list.items[0].source_line, 2);
    eventlist_free(&list);

    CHECK_INT(store_remove(path, 1, NULL), REMOVE_NO_SUCH_EVENT);
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, content);

    CHECK_INT(store_remove(path, 2, NULL), REMOVE_OK);

    remove(path);
}

// A write that fails halfway (disk full) must leave the original file alone.
static void test_remove_survives_a_failing_write(void) {
    char path[PATH_BUF], temp[PATH_BUF + 8], out[OUTPUT_BUF];
    CHECK(test_make_temp(path, sizeof path));
    snprintf(temp, sizeof temp, "%s.tmp", path);
    test_write_file(path, "2026-01-01|A\n2026-01-02|B\n");

    // The temp name is predictable, so point it at /dev/full: every write
    // there fails with "no space left on device". macOS has no /dev/full, so
    // there the open itself fails, which the code must survive just the same.
    CHECK(symlink("/dev/full", temp) == 0);

    CHECK_INT(store_remove(path, 1, NULL), REMOVE_ERROR);
    read_whole_file(path, out, sizeof out);
    CHECK_STR(out, "2026-01-01|A\n2026-01-02|B\n");

    remove(temp);
    remove(path);
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
    RUN_TEST(test_load_records_the_line_each_event_came_from);
    RUN_TEST(test_remove_drops_one_line_and_keeps_every_other_byte);
    RUN_TEST(test_remove_without_asking_what_was_removed);
    RUN_TEST(test_remove_refuses_lines_that_are_not_events);
    RUN_TEST(test_remove_from_a_missing_file);
    RUN_TEST(test_remove_when_the_last_line_has_no_newline);
    RUN_TEST(test_remove_keeps_windows_line_endings);
    RUN_TEST(test_remove_handles_overlong_lines);
    RUN_TEST(test_remove_follows_a_symlink_instead_of_replacing_it);
    RUN_TEST(test_remove_keeps_the_file_permissions);
    RUN_TEST(test_remove_never_leaves_a_temp_file_behind);
    RUN_TEST(test_remove_until_the_file_is_empty);
    RUN_TEST(test_remove_agrees_with_load_about_overlong_lines);
    RUN_TEST(test_remove_survives_a_failing_write);
    return test_report(__FILE__);
}