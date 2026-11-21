#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>

#include "support/test.h"

#include "cli/args/args.h"
#include "core/clock/clock.h"

enum { MAX_TEST_ARGS = 16 };

static const Today TODAY = {1, 2026, 11, 21};
static const Today NO_CLOCK = {0, 0, 0, 0};

// Calls args_parse as if the user typed: cal <words...>. The list of words
// ends with NULL.
static ArgsStatus run(Config *cfg, const Today *today, ...) {
    char *argv[MAX_TEST_ARGS];
    int argc = 0;
    argv[argc++] = "cal";

    va_list ap;
    va_start(ap, today);
    const char *word;
    while ((word = va_arg(ap, const char *)) != NULL) argv[argc++] = (char *)word;
    va_end(ap);

    return args_parse(argc, argv, today, cfg);
}

static void test_no_arguments_means_the_current_month(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, NULL), ARGS_OK);
    CHECK_INT(cfg.view, VIEW_MONTH);
    CHECK_INT(cfg.month, 11);
    CHECK_INT(cfg.year, 2026);
}

static void test_month_and_year(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "10", "2026", NULL), ARGS_OK);
    CHECK_INT(cfg.view, VIEW_MONTH);
    CHECK_INT(cfg.month, 10);
    CHECK_INT(cfg.year, 2026);

    CHECK_INT(run(&cfg, &TODAY, "oct", "2027", NULL), ARGS_OK);
    CHECK_INT(cfg.month, 10);
    CHECK_INT(cfg.year, 2027);
}

// The documented rule: one argument that parses as a month is a month of the
// current year, anything else is a year.
static void test_single_argument_is_a_month_or_a_year(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "10", NULL), ARGS_OK);
    CHECK_INT(cfg.view, VIEW_MONTH);
    CHECK_INT(cfg.month, 10);
    CHECK_INT(cfg.year, 2026);          // taken from the clock

    CHECK_INT(run(&cfg, &TODAY, "march", NULL), ARGS_OK);
    CHECK_INT(cfg.view, VIEW_MONTH);
    CHECK_INT(cfg.month, 3);

    CHECK_INT(run(&cfg, &TODAY, "2026", NULL), ARGS_OK);
    CHECK_INT(cfg.view, VIEW_YEAR);
    CHECK_INT(cfg.year, 2026);

    CHECK_INT(run(&cfg, &TODAY, "13", NULL), ARGS_OK);   // not a month, so a year
    CHECK_INT(cfg.view, VIEW_YEAR);
    CHECK_INT(cfg.year, 13);
}

static void test_layout_flags(void) {
    Config cfg;

    run(&cfg, &TODAY, NULL);
    CHECK_INT(cfg.render.monday_first, 0);
    CHECK_INT(cfg.render.show_weeks, 0);

    run(&cfg, &TODAY, "-m", NULL);
    CHECK_INT(cfg.render.monday_first, 1);
    CHECK_INT(cfg.render.show_weeks, 0);

    run(&cfg, &TODAY, "-w", NULL);
    CHECK_INT(cfg.render.monday_first, 0);
    CHECK_INT(cfg.render.show_weeks, 1);

    run(&cfg, &TODAY, "-mw", NULL);     // combined
    CHECK_INT(cfg.render.monday_first, 1);
    CHECK_INT(cfg.render.show_weeks, 1);

    run(&cfg, &TODAY, "-m", "-w", NULL);
    CHECK_INT(cfg.render.monday_first, 1);
    CHECK_INT(cfg.render.show_weeks, 1);
}

static void test_flags_work_before_and_after_positionals(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "10", "2026", "-m", NULL), ARGS_OK);
    CHECK_INT(cfg.render.monday_first, 1);
    CHECK_INT(cfg.month, 10);

    CHECK_INT(run(&cfg, &TODAY, "-w", "10", "-m", "2026", NULL), ARGS_OK);
    CHECK_INT(cfg.render.show_weeks, 1);
    CHECK_INT(cfg.render.monday_first, 1);
    CHECK_INT(cfg.year, 2026);
}

static void test_color_flags(void) {
    Config cfg;

    run(&cfg, &TODAY, "-n", NULL);
    CHECK_INT(cfg.render.use_color, 0);

    run(&cfg, &TODAY, "--no-color", NULL);
    CHECK_INT(cfg.render.use_color, 0);

    run(&cfg, &TODAY, "-mn", "10", NULL);
    CHECK_INT(cfg.render.use_color, 0);
    CHECK_INT(cfg.render.monday_first, 1);
}

static void test_event_options(void) {
    Config cfg;

    run(&cfg, &TODAY, NULL);
    CHECK_INT(cfg.use_events, 1);
    CHECK(cfg.events_path == NULL);

    CHECK_INT(run(&cfg, &TODAY, "-e", "work.txt", NULL), ARGS_OK);
    CHECK_STR(cfg.events_path, "work.txt");
    CHECK_INT(cfg.use_events, 1);

    CHECK_INT(run(&cfg, &TODAY, "--events", "/tmp/e.txt", "10", "2026", NULL), ARGS_OK);
    CHECK_STR(cfg.events_path, "/tmp/e.txt");
    CHECK_INT(cfg.month, 10);               // the file name must not be read as a month

    CHECK_INT(run(&cfg, &TODAY, "--no-events", NULL), ARGS_OK);
    CHECK_INT(cfg.use_events, 0);
}

static void test_events_option_needs_a_value(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "-e", NULL), ARGS_ERROR);
    CHECK(strstr(cfg.error, "needs a file name") != NULL);
    CHECK_INT(cfg.show_usage, 1);

    CHECK_INT(run(&cfg, &TODAY, "10", "--events", NULL), ARGS_ERROR);
}

static void test_help(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "-h", NULL), ARGS_HELP);
    CHECK_INT(run(&cfg, &TODAY, "--help", NULL), ARGS_HELP);
    CHECK_INT(run(&cfg, &TODAY, "-mh", NULL), ARGS_HELP);
    CHECK_INT(run(&cfg, &TODAY, "10", "2026", "-h", NULL), ARGS_HELP);
}

static void test_bad_input_gives_a_message(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "abc", NULL), ARGS_ERROR);
    CHECK_STR(cfg.error, "Invalid month or year: abc");
    CHECK_INT(cfg.show_usage, 0);

    CHECK_INT(run(&cfg, &TODAY, "13", "2026", NULL), ARGS_ERROR);
    CHECK_STR(cfg.error, "Invalid month: 13");

    CHECK_INT(run(&cfg, &TODAY, "10", "0", NULL), ARGS_ERROR);
    CHECK_STR(cfg.error, "Invalid year: 0 (use 1 to 9999)");

    CHECK_INT(run(&cfg, &TODAY, "10", "10000", NULL), ARGS_ERROR);
    CHECK_INT(run(&cfg, &TODAY, "10abc", NULL), ARGS_ERROR);
}

static void test_unknown_option_and_too_many_arguments_show_usage(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &TODAY, "-x", NULL), ARGS_ERROR);
    CHECK_STR(cfg.error, "Unknown option: -x");
    CHECK_INT(cfg.show_usage, 1);

    CHECK_INT(run(&cfg, &TODAY, "-mz", NULL), ARGS_ERROR);
    CHECK_STR(cfg.error, "Unknown option: -z");

    CHECK_INT(run(&cfg, &TODAY, "1", "2", "3", NULL), ARGS_ERROR);
    CHECK_STR(cfg.error, "");           // the usage text says it all
    CHECK_INT(cfg.show_usage, 1);
}

// Without a readable clock the program cannot guess the month or year, but
// anything the user spells out completely must still work.
static void test_unreadable_clock(void) {
    Config cfg;

    CHECK_INT(run(&cfg, &NO_CLOCK, NULL), ARGS_ERROR);
    CHECK(strstr(cfg.error, "system clock") != NULL);

    CHECK_INT(run(&cfg, &NO_CLOCK, "10", NULL), ARGS_ERROR);      // needs the current year

    CHECK_INT(run(&cfg, &NO_CLOCK, "10", "2026", NULL), ARGS_OK);
    CHECK_INT(run(&cfg, &NO_CLOCK, "2026", NULL), ARGS_OK);
    CHECK_INT(cfg.view, VIEW_YEAR);
}

int main(void) {
    RUN_TEST(test_no_arguments_means_the_current_month);
    RUN_TEST(test_month_and_year);
    RUN_TEST(test_single_argument_is_a_month_or_a_year);
    RUN_TEST(test_layout_flags);
    RUN_TEST(test_flags_work_before_and_after_positionals);
    RUN_TEST(test_color_flags);
    RUN_TEST(test_event_options);
    RUN_TEST(test_events_option_needs_a_value);
    RUN_TEST(test_help);
    RUN_TEST(test_bad_input_gives_a_message);
    RUN_TEST(test_unknown_option_and_too_many_arguments_show_usage);
    RUN_TEST(test_unreadable_clock);
    return test_report(__FILE__);
}