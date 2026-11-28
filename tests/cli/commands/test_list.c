#define _POSIX_C_SOURCE 200809L

#include "support/command.h"
#include "support/test.h"

#include "cli/commands/add/add.h"
#include "cli/commands/list/list.h"

enum { PATH_BUF = 128 };

static void run(CommandResult *r, int argc, char *argv[]) {
    command_run(list_run, r, argc, argv);
}

static void run_add(CommandResult *r, int argc, char *argv[]) {
    command_run(add_run, r, argc, argv);
}

static void test_lists_events_with_their_file_line_numbers(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path,
        "# my events\n"                         // 1
        "2026-12-25|Christmas\n"                // 2
        "\n"                                    // 3
        "12-31 | New Year's Eve\n"              // 4
        "oops, a typo\n"                        // 5
        "2026-10-15|Team offsite\n");           // 6

    char *argv[] = {"-e", path};
    CommandResult r;
    run(&r, 2, argv);

    CHECK_INT(r.code, EXIT_SUCCESS);

    // Numbers are file lines (2, 4, 6), not 1, 2, 3, so they match the editor
    // and the warning about line 5. Yearly events say "every".
    CHECK_STR(r.out,
        "   2  2026-12-25   Christmas\n"
        "   4  every 12-31  New Year's Eve\n"
        "   6  2026-10-15   Team offsite\n");

    // The typo is reported, but it does not stop the listing.
    CHECK(strstr(r.err, ":5:") != NULL);

    remove(path);
}

static void test_long_option_name_works_too(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "12-25|Christmas\n");

    char *argv[] = {"--events", path};
    CommandResult r;
    run(&r, 2, argv);

    CHECK_INT(r.code, EXIT_SUCCESS);
    CHECK(strstr(r.out, "every 12-25") != NULL);

    remove(path);
}

static void test_a_file_with_only_comments_has_no_events(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "# nothing yet\n\n");

    char *argv[] = {"-e", path};
    CommandResult r;
    run(&r, 2, argv);

    CHECK_INT(r.code, EXIT_SUCCESS);
    CHECK(strstr(r.out, "No events in") != NULL);

    remove(path);
}

// No default file yet is the normal state of a new install, not an error.
static void test_missing_default_file_is_friendly(void) {
    setenv("CAL_EVENTS", "/tmp/cal_test_no_such_events_file", 1);
    CommandResult r;

    run(&r, 0, NULL);
    CHECK_INT(r.code, EXIT_SUCCESS);
    CHECK(strstr(r.out, "No events yet") != NULL);
    CHECK(strstr(r.out, "mycal add 12-25 Christmas") != NULL);
    CHECK_STR(r.err, "");

    unsetenv("CAL_EVENTS");
}

static void test_missing_named_file_is_an_error(void) {
    char *argv[] = {"-e", "/tmp/cal_test_no_such_events_file"};
    CommandResult r;

    run(&r, 2, argv);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Cannot open events file") != NULL);
    CHECK_STR(r.out, "");
}

static void test_wrong_arguments_print_usage(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    CommandResult r;

    char *extra[] = {"-e", path, "surprise"};
    run(&r, 3, extra);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage: mycal list") != NULL);

    char *stray[] = {"surprise"};
    run(&r, 1, stray);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage:") != NULL);

    char *no_value[] = {"-e"};
    run(&r, 1, no_value);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "needs a file name") != NULL);

    remove(path);
}

static void test_no_file_anywhere_is_an_error(void) {
    unsetenv("CAL_EVENTS");
    unsetenv("HOME");
    CommandResult r;

    run(&r, 0, NULL);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "No events file") != NULL);
}

// The two commands have to agree: whatever add writes, list shows.
static void test_list_shows_what_add_wrote(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    remove(path);
    CommandResult r;

    char *first[] = {"-e", path, "2026-10-15", "Team", "offsite"};
    run_add(&r, 5, first);
    CHECK_INT(r.code, EXIT_SUCCESS);

    char *second[] = {"-e", path, "12-25", "Christmas"};
    run_add(&r, 4, second);
    CHECK_INT(r.code, EXIT_SUCCESS);

    char *list_args[] = {"-e", path};
    run(&r, 2, list_args);
    CHECK_STR(r.out,
        "   1  2026-10-15   Team offsite\n"
        "   2  every 12-25  Christmas\n");

    remove(path);
}

int main(void) {
    RUN_TEST(test_lists_events_with_their_file_line_numbers);
    RUN_TEST(test_long_option_name_works_too);
    RUN_TEST(test_a_file_with_only_comments_has_no_events);
    RUN_TEST(test_missing_default_file_is_friendly);
    RUN_TEST(test_missing_named_file_is_an_error);
    RUN_TEST(test_wrong_arguments_print_usage);
    RUN_TEST(test_no_file_anywhere_is_an_error);
    RUN_TEST(test_list_shows_what_add_wrote);
    return test_report(__FILE__);
}
