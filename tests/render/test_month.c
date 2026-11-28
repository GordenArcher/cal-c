#define _POSIX_C_SOURCE 200809L

#include "support/test.h"
#include "support/fixtures.h"

#include "core/clock/clock.h"
#include "events/model/event.h"
#include "render/line/line.h"
#include "render/month/month.h"
#include "render/style/options.h"
#include "render/style/style.h"

static const Today NO_TODAY = {0, 0, 0, 0};

static RenderOptions options(int monday_first, int show_weeks, int use_color) {
    RenderOptions o;
    o.monday_first = monday_first;
    o.show_weeks = show_weeks;
    o.use_color = use_color;
    return o;
}

static int count_in_block(Line block[LINES_PER_BLOCK], const char *needle) {
    int total = 0;
    for (int i = 0; i < LINES_PER_BLOCK; i++) total += test_count_substr(block[i].text, needle);
    return total;
}

static void test_week_row_counts(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions sunday = options(0, 0, 0);
    RenderOptions monday = options(1, 0, 0);

    CHECK_INT(month_render(2026, 10, 1, &sunday, &NO_TODAY, NULL, NULL, block), 5);
    CHECK_INT(month_render(2026, 2, 1, &sunday, &NO_TODAY, NULL, NULL, block), 4);   // 28 days starting on Sunday
    CHECK_INT(month_render(2026, 8, 1, &sunday, &NO_TODAY, NULL, NULL, block), 6);   // needs the maximum
    CHECK_INT(month_render(2026, 2, 1, &monday, &NO_TODAY, NULL, NULL, block), 5);   // same month, other start day
}

// The year view lines blocks up side by side by trusting that every line has
// the same visible width. Check that for every month and every layout.
static void test_every_line_has_the_same_visible_width(void) {
    for (int weeks = 0; weeks <= 1; weeks++) {
        for (int monday = 0; monday <= 1; monday++) {
            RenderOptions opts = options(monday, weeks, 0);

            for (int month = 1; month <= MONTHS_PER_YEAR; month++) {
                Line block[LINES_PER_BLOCK];
                month_render(2026, month, 1, &opts, &NO_TODAY, NULL, NULL, block);

                for (int i = 0; i < LINES_PER_BLOCK; i++) {
                    CHECK_INT(strlen(block[i].text), month_block_width(&opts));
                }
            }
        }
    }
}

static void test_widths_with_and_without_week_numbers(void) {
    RenderOptions plain = options(0, 0, 0);
    RenderOptions weeks = options(0, 1, 0);

    CHECK_INT(month_block_width(&plain), 20);
    CHECK_INT(month_block_width(&weeks), 23);
}

static void test_title_with_and_without_year(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 0, 0);

    month_render(2026, 10, 1, &opts, &NO_TODAY, NULL, NULL, block);
    CHECK(strstr(block[0].text, "October 2026") != NULL);

    month_render(2026, 10, 0, &opts, &NO_TODAY, NULL, NULL, block);
    CHECK(strstr(block[0].text, "October") != NULL);
    CHECK(strstr(block[0].text, "2026") == NULL);
}

static void test_first_row_is_positioned_under_the_right_weekday(void) {
    Line block[LINES_PER_BLOCK];

    // Oct 1 2026 is a Thursday: four empty cells, then 1 2 3.
    month_render(2026, 10, 1, &(RenderOptions){0, 0, 0}, &NO_TODAY, NULL, NULL, block);
    CHECK_STR(block[1].text, "Su Mo Tu We Th Fr Sa");
    CHECK_STR(block[2].text, "             1  2  3");

    // Monday first: the same Thursday sits in the fourth column.
    month_render(2026, 10, 1, &(RenderOptions){1, 0, 0}, &NO_TODAY, NULL, NULL, block);
    CHECK_STR(block[1].text, "Mo Tu We Th Fr Sa Su");
    CHECK_STR(block[2].text, "          1  2  3  4");
}

static void test_week_numbers_column(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(1, 1, 0);

    month_render(2026, 10, 1, &opts, &NO_TODAY, NULL, NULL, block);

    CHECK(strncmp(block[1].text, "Wk ", 3) == 0);
    CHECK(strncmp(block[2].text, "40 ", 3) == 0);   // Sep 28 to Oct 4
    CHECK(strncmp(block[3].text, "41 ", 3) == 0);
    CHECK(strncmp(block[6].text, "44 ", 3) == 0);   // Oct 26 to Nov 1
}

static void test_today_is_highlighted_only_in_its_own_month(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 0, 1);
    Today today = {1, 2026, 10, 15};

    month_render(2026, 10, 1, &opts, &today, NULL, NULL, block);
    CHECK_INT(count_in_block(block, ANSI_REVERSE), 1);
    CHECK_INT(count_in_block(block, ANSI_REVERSE "15" ANSI_RESET), 1);

    month_render(2026, 11, 1, &opts, &today, NULL, NULL, block);    // other month
    CHECK_INT(count_in_block(block, ANSI_REVERSE), 0);

    month_render(2027, 10, 1, &opts, &today, NULL, NULL, block);    // same month, other year
    CHECK_INT(count_in_block(block, ANSI_REVERSE), 0);

    month_render(2026, 10, 1, &opts, &NO_TODAY, NULL, NULL, block); // clock unavailable
    CHECK_INT(count_in_block(block, ANSI_REVERSE), 0);
}

static void test_weekends_are_red(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 0, 1);

    month_render(2026, 10, 1, &opts, &NO_TODAY, NULL, NULL, block);

    // Oct 2026 has 5 Saturdays and 4 Sundays.
    CHECK_INT(count_in_block(block, ANSI_RED), 9);
    CHECK_INT(count_in_block(block, ANSI_RED " 3" ANSI_RESET), 1);   // Saturday the 3rd
}

static void test_event_days_are_underlined(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 0, 1);
    EventList events;
    eventlist_init(&events);

    fixture_add_event(&events, 2026, 10, 15, "Offsite");        // this year only
    fixture_add_event(&events, EVERY_YEAR, 10, 31, "Halloween");
    fixture_add_event(&events, 2027, 10, 20, "Next year");      // must not show in 2026
    fixture_add_event(&events, EVERY_YEAR, 11, 5, "Other month");

    month_render(2026, 10, 1, &opts, &NO_TODAY, &events, NULL, block);

    CHECK_INT(count_in_block(block, ANSI_UNDERLINE), 2);
    CHECK_INT(count_in_block(block, ANSI_UNDERLINE "15" ANSI_RESET), 1);
    // The 31st is a Saturday: it must keep its red AND gain the underline.
    CHECK_INT(count_in_block(block, ANSI_RED ANSI_UNDERLINE "31" ANSI_RESET), 1);

    eventlist_free(&events);
}

static void test_event_on_today_keeps_both_styles(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 0, 1);
    Today today = {1, 2026, 10, 15};
    EventList events;
    eventlist_init(&events);
    fixture_add_event(&events, 2026, 10, 15, "Offsite");

    month_render(2026, 10, 1, &opts, &today, &events, NULL, block);
    CHECK_INT(count_in_block(block, ANSI_REVERSE ANSI_UNDERLINE "15" ANSI_RESET), 1);

    eventlist_free(&events);
}

static void test_holiday_days_are_bold_and_stack(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 0, 1);
    Today today = {1, 2026, 10, 15};
    EventList events;
    EventList holidays;
    eventlist_init(&events);
    eventlist_init(&holidays);

    fixture_add_event(&events, 2026, 10, 6, "Plain event");
    fixture_add_event(&events, 2026, 10, 20, "Launch");
    fixture_add_event(&holidays, 2026, 10, 15, "Holiday today");
    fixture_add_event(&holidays, 2026, 10, 20, "Shared day");
    fixture_add_event(&holidays, 2026, 10, 31, "Weekend holiday");

    month_render(2026, 10, 1, &opts, &today, &events, &holidays, block);

    // A holiday is bold. It must not erase the reverse of today, the red of a
    // weekend, or the underline of an event on the same day.
    CHECK_INT(count_in_block(block, ANSI_REVERSE ANSI_BOLD "15" ANSI_RESET), 1);
    CHECK_INT(count_in_block(block, ANSI_BOLD ANSI_UNDERLINE "20" ANSI_RESET), 1);
    CHECK_INT(count_in_block(block, ANSI_RED ANSI_BOLD "31" ANSI_RESET), 1);

    // A day with only an event stays underlined and not bold.
    CHECK_INT(count_in_block(block, ANSI_UNDERLINE " 6" ANSI_RESET), 1);
    CHECK_INT(count_in_block(block, ANSI_BOLD " 6" ANSI_RESET), 0);

    eventlist_free(&events);
    eventlist_free(&holidays);
}

static void test_no_color_means_no_escape_codes_at_all(void) {
    Line block[LINES_PER_BLOCK];
    RenderOptions opts = options(0, 1, 0);
    Today today = {1, 2026, 10, 15};
    EventList events;
    eventlist_init(&events);
    fixture_add_event(&events, 2026, 10, 15, "Offsite");

    month_render(2026, 10, 1, &opts, &today, &events, NULL, block);

    for (int i = 0; i < LINES_PER_BLOCK; i++) CHECK(strchr(block[i].text, '\033') == NULL);

    eventlist_free(&events);
}

int main(void) {
    RUN_TEST(test_week_row_counts);
    RUN_TEST(test_every_line_has_the_same_visible_width);
    RUN_TEST(test_widths_with_and_without_week_numbers);
    RUN_TEST(test_title_with_and_without_year);
    RUN_TEST(test_first_row_is_positioned_under_the_right_weekday);
    RUN_TEST(test_week_numbers_column);
    RUN_TEST(test_today_is_highlighted_only_in_its_own_month);
    RUN_TEST(test_weekends_are_red);
    RUN_TEST(test_event_days_are_underlined);
    RUN_TEST(test_event_on_today_keeps_both_styles);
    RUN_TEST(test_holiday_days_are_bold_and_stack);
    RUN_TEST(test_no_color_means_no_escape_codes_at_all);
    return test_report(__FILE__);
}