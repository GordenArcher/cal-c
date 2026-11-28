#define _POSIX_C_SOURCE 200809L

#include "support/command.h"
#include "support/test.h"

#include "cli/commands/add/add.h"
#include "cli/commands/list/list.h"
#include "cli/commands/remove/remove.h"

enum {
    PATH_BUF = 128,
    FILE_BUF = 4096
};

static void run(CommandResult *r, int argc, char *argv[]) {
    command_run(remove_run, r, argc, argv);
}

static void run_list(CommandResult *r, int argc, char *argv[]) {
    command_run(list_run, r, argc, argv);
}

static void run_add(CommandResult *r, int argc, char *argv[]) {
    command_run(add_run, r, argc, argv);
}

static void read_whole_file(const char *path, char *buf, size_t cap) {
    FILE *f = fopen(path, "r");
    size_t n = f ? fread(buf, 1, cap - 1, f) : 0;
    buf[n] = '\0';
    if (f) fclose(f);
}

static void test_removes_the_event_and_says_which_one(void) {
    char path[PATH_BUF], contents[FILE_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path,
        "# my events\n"                 // 1
        "2026-12-25|Christmas\n"        // 2
        "\n"                            // 3
        "12-31 | New Year's Eve\n"      // 4
        "2026-10-15|Team offsite\n");   // 5

    char *argv[] = {"-e", path, "4"};
    CommandResult r;
    run(&r, 3, argv);

    CHECK_INT(r.code, EXIT_SUCCESS);
    CHECK(strstr(r.out, "Removed from") != NULL);
    CHECK(strstr(r.out, "12-31|New Year's Eve") != NULL);
    CHECK(strstr(r.out, "moved up") != NULL);       // the renumbering warning
    CHECK_STR(r.err, "");

    read_whole_file(path, contents, sizeof contents);
    CHECK_STR(contents,
        "# my events\n"
        "2026-12-25|Christmas\n"
        "\n"
        "2026-10-15|Team offsite\n");

    remove(path);
}

// The whole point of the numbers: whatever list shows, remove accepts.
static void test_list_then_remove_then_list(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    remove(path);
    CommandResult r;

    char *a[] = {"-e", path, "2026-10-15", "Team", "offsite"};
    char *b[] = {"-e", path, "12-25", "Christmas"};
    char *c[] = {"-e", path, "10-31", "Halloween"};
    run_add(&r, 5, a);
    run_add(&r, 4, b);
    run_add(&r, 4, c);

    char *list_args[] = {"-e", path};
    run_list(&r, 2, list_args);
    CHECK_STR(r.out,
        "   1  2026-10-15   Team offsite\n"
        "   2  every 12-25  Christmas\n"
        "   3  every 10-31  Halloween\n");

    char *remove_second[] = {"-e", path, "2"};
    run(&r, 3, remove_second);
    CHECK_INT(r.code, EXIT_SUCCESS);

    // Christmas is gone and Halloween moved from line 3 to line 2.
    run_list(&r, 2, list_args);
    CHECK_STR(r.out,
        "   1  2026-10-15   Team offsite\n"
        "   2  every 10-31  Halloween\n");

    remove(path);
}

static void test_refuses_a_line_that_is_not_an_event(void) {
    const char *content = "# a comment\n\nnot an event\n2026-01-01|Real\n";
    char path[PATH_BUF], contents[FILE_BUF];
    CHECK(test_make_temp(path, sizeof path));
    CommandResult r;

    int lines[] = {1, 2, 3, 5, 99};
    for (size_t i = 0; i < sizeof lines / sizeof lines[0]; i++) {
        char number[16];
        snprintf(number, sizeof number, "%d", lines[i]);
        test_write_file(path, content);

        char *argv[] = {"-e", path, number};
        run(&r, 3, argv);

        CHECK_INT(r.code, EXIT_FAILURE);
        CHECK(strstr(r.err, "is not an event") != NULL);
        CHECK(strstr(r.err, "mycal list") != NULL);
        CHECK_STR(r.out, "");

        read_whole_file(path, contents, sizeof contents);
        CHECK_STR(contents, content);               // nothing was touched
    }

    remove(path);
}

static void test_rejects_bad_line_numbers_without_touching_the_file(void) {
    const char *content = "2026-01-01|Real\n";
    char path[PATH_BUF], contents[FILE_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, content);
    CommandResult r;

    const char *bad[] = {"abc", "0", "-1", "2x", "1.5", "", "99999999999999999999"};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        char *argv[] = {"-e", path, (char *)bad[i]};
        run(&r, 3, argv);

        CHECK_INT(r.code, EXIT_FAILURE);
        CHECK(strstr(r.err, "Invalid line number") != NULL);
        CHECK(strstr(r.err, "Usage: mycal remove") != NULL);
    }

    read_whole_file(path, contents, sizeof contents);
    CHECK_STR(contents, content);
    remove(path);
}

static void test_wrong_number_of_arguments_prints_usage(void) {
    char path[PATH_BUF];
    CHECK(test_make_temp(path, sizeof path));
    CommandResult r;

    run(&r, 0, NULL);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage: mycal remove") != NULL);

    char *only_file[] = {"-e", path};
    run(&r, 2, only_file);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage:") != NULL);

    char *two_numbers[] = {"-e", path, "1", "2"};     // removing two at once is not supported
    run(&r, 4, two_numbers);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Usage:") != NULL);

    char *no_value[] = {"-e"};
    run(&r, 1, no_value);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "needs a file name") != NULL);

    remove(path);
}

static void test_missing_file_is_reported(void) {
    char *argv[] = {"-e", "/tmp/cal_test_no_such_events_file", "1"};
    CommandResult r;

    run(&r, 3, argv);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "Cannot open events file") != NULL);
}

static void test_uses_the_environment_and_the_long_option(void) {
    char path[PATH_BUF], contents[FILE_BUF];
    CHECK(test_make_temp(path, sizeof path));
    test_write_file(path, "2026-01-01|A\n2026-01-02|B\n");
    CommandResult r;

    setenv("CAL_EVENTS", path, 1);
    char *from_env[] = {"1"};
    run(&r, 1, from_env);
    CHECK_INT(r.code, EXIT_SUCCESS);
    unsetenv("CAL_EVENTS");

    char *long_option[] = {"--events", path, "1"};
    run(&r, 3, long_option);
    CHECK_INT(r.code, EXIT_SUCCESS);

    read_whole_file(path, contents, sizeof contents);
    CHECK_STR(contents, "");
    remove(path);
}

static void test_no_file_anywhere_is_an_error(void) {
    unsetenv("CAL_EVENTS");
    unsetenv("HOME");
    char *argv[] = {"1"};
    CommandResult r;

    run(&r, 1, argv);
    CHECK_INT(r.code, EXIT_FAILURE);
    CHECK(strstr(r.err, "No events file") != NULL);
}

int main(void) {
    RUN_TEST(test_removes_the_event_and_says_which_one);
    RUN_TEST(test_list_then_remove_then_list);
    RUN_TEST(test_refuses_a_line_that_is_not_an_event);
    RUN_TEST(test_rejects_bad_line_numbers_without_touching_the_file);
    RUN_TEST(test_wrong_number_of_arguments_prints_usage);
    RUN_TEST(test_missing_file_is_reported);
    RUN_TEST(test_uses_the_environment_and_the_long_option);
    RUN_TEST(test_no_file_anywhere_is_an_error);
    return test_report(__FILE__);
}
