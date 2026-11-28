#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "cli/add/add.h"
#include "events/model/event.h"
#include "events/store/store.h"

enum {
    PATH_BUF = 128,
    OUTPUT_BUF = 4096,
    LONG_LABEL = 200
};

typedef struct {
    int code;
    char out[OUTPUT_BUF];
    char err[OUTPUT_BUF];
} Result;

static void run(Result *r, int argc, char *argv[]) {
    FILE *out = tmpfile();
    FILE *err = tmpfile();

    r->code = add_run("mycal", argc, argv, out, err);
    test_slurp(out, r->out, sizeof r->out);
    test_slurp(err, r->err, sizeof r->err);

    fclose(out);
    fclose(err);
}

static int load(const char *path, EventList *list) {
    eventlist_init(list);
    return store_load(path, list, NULL) == STORE_OK;
}

// A path that is free to use: created, then removed again.
static void fresh_path(char *path, size_t cap) {
    test_make_temp(path, cap);
    remove(path);
}

static void test_adds_a_one_off_event(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    char *argv[] = {"-e", path, "2026-12-25", "Christmas"};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_SUCCESS);
    CHECK(strstr(r.out, "Added to") != NULL);
    CHECK(strstr(r.out, "2026-12-25|Christmas") != NULL);
    CHECK_STR(r.err, "");

    EventList list;
    CHECK(load(path, &list));
    CHECK_INT(list.count, 1);
    CHECK_INT(list.items[0].year, 2026);
    CHECK_INT(list.items[0].month, 12);
    CHECK_STR(list.items[0].label, "Christmas");

    eventlist_free(&list);
    remove(path);
}

static void test_adds_a_yearly_event(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    char *argv[] = {"-e", path, "12-25", "Christmas"};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_SUCCESS);

    EventList list;
    CHECK(load(path, &list));
    CHECK_INT(list.items[0].year, EVERY_YEAR);

    eventlist_free(&list);
    remove(path);
}

// Both of these must produce the same label:
//   mycal add 12-25 Christmas Day      mycal add 12-25 "Christmas Day"
static void test_label_words_are_joined_with_single_spaces(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    char *loose[] = {"-e", path, "12-25", "Christmas", "Day", "party"};
    char *quoted[] = {"-e", path, "12-26", "Boxing Day"};
    Result r;

    run(&r, 6, loose);
    run(&r, 4, quoted);

    EventList list;
    CHECK(load(path, &list));
    CHECK_INT(list.count, 2);
    CHECK_STR(list.items[0].label, "Christmas Day party");
    CHECK_STR(list.items[1].label, "Boxing Day");

    eventlist_free(&list);
    remove(path);
}

static void test_label_is_trimmed_so_it_matches_what_gets_loaded(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    char *argv[] = {"-e", path, "12-25", "  padded  "};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_SUCCESS);
    CHECK(strstr(r.out, "12-25|padded") != NULL);

    EventList list;
    CHECK(load(path, &list));
    CHECK_STR(list.items[0].label, "padded");

    eventlist_free(&list);
    remove(path);
}

static void test_label_may_contain_a_pipe(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    char *argv[] = {"-e", path, "10-31", "trick|treat"};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_SUCCESS);

    EventList list;
    CHECK(load(path, &list));
    CHECK_STR(list.items[0].label, "trick|treat");

    eventlist_free(&list);
    remove(path);
}

static void test_appends_without_touching_existing_events(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "# mine\n2026-01-01|Existing\n");
    char *argv[] = {"-e", path, "01-02", "New"};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_SUCCESS);

    EventList list;
    CHECK(load(path, &list));
    CHECK_INT(list.count, 2);
    CHECK_STR(list.items[0].label, "Existing");
    CHECK_STR(list.items[1].label, "New");

    eventlist_free(&list);
    remove(path);
}

static void test_bad_date_is_refused_and_nothing_is_written(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    char *argv[] = {"-e", path, "2026-02-30", "Impossible"};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Invalid date: 2026-02-30") != NULL);
    CHECK(strstr(r.err, "Usage:") != NULL);
    CHECK_STR(r.out, "");

    FILE *f = fopen(path, "r");
    CHECK(f == NULL);               // the file must not even have been created
    if (f) fclose(f);
}

static void test_missing_arguments_print_usage(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    Result r;

    run(&r, 0, NULL);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage:") != NULL);

    char *date_only[] = {"-e", path, "12-25"};
    run(&r, 3, date_only);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage:") != NULL);

    char *no_value[] = {"-e"};
    run(&r, 1, no_value);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "needs a file name") != NULL);
}

static void test_bad_labels_are_refused(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    Result r;

    char *blank[] = {"-e", path, "12-25", "   "};
    run(&r, 4, blank);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "empty") != NULL);

    // A newline in a label would turn one event into two lines of the file.
    char *newline[] = {"-e", path, "12-25", "two\nlines"};
    run(&r, 4, newline);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "control characters") != NULL);

    char long_label[LONG_LABEL + 1];
    memset(long_label, 'x', LONG_LABEL);
    long_label[LONG_LABEL] = '\0';
    char *too_long[] = {"-e", path, "12-25", long_label};
    run(&r, 4, too_long);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "too long") != NULL);

    FILE *f = fopen(path, "r");
    CHECK(f == NULL);               // none of the three wrote anything
    if (f) fclose(f);
}

static void test_unwritable_file_is_reported(void) {
    char *argv[] = {"-e", "/tmp/cal_test_no_such_dir/events.txt", "12-25", "Christmas"};
    Result r;

    run(&r, 4, argv);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Could not write") != NULL);
}

static void test_uses_the_environment_when_no_file_is_given(void) {
    char path[PATH_BUF];
    fresh_path(path, sizeof path);
    setenv("CAL_EVENTS", path, 1);
    char *argv[] = {"12-25", "Christmas"};
    Result r;

    run(&r, 2, argv);
    CHECK_INT(r.code, EXIT_SUCCESS);

    EventList list;
    CHECK(load(path, &list));
    CHECK_INT(list.count, 1);

    eventlist_free(&list);
    unsetenv("CAL_EVENTS");
    remove(path);
}

static void test_no_file_anywhere_is_an_error(void) {
    unsetenv("CAL_EVENTS");
    unsetenv("HOME");
    char *argv[] = {"12-25", "Christmas"};
    Result r;

    run(&r, 2, argv);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "No events file") != NULL);
}

int main(void) {
    RUN_TEST(test_adds_a_one_off_event);
    RUN_TEST(test_adds_a_yearly_event);
    RUN_TEST(test_label_words_are_joined_with_single_spaces);
    RUN_TEST(test_label_is_trimmed_so_it_matches_what_gets_loaded);
    RUN_TEST(test_label_may_contain_a_pipe);
    RUN_TEST(test_appends_without_touching_existing_events);
    RUN_TEST(test_bad_date_is_refused_and_nothing_is_written);
    RUN_TEST(test_missing_arguments_print_usage);
    RUN_TEST(test_bad_labels_are_refused);
    RUN_TEST(test_unwritable_file_is_reported);
    RUN_TEST(test_uses_the_environment_when_no_file_is_given);
    RUN_TEST(test_no_file_anywhere_is_an_error);
    return test_report(__FILE__);
}