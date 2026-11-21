#define _POSIX_C_SOURCE 200809L

#include "support/test.h"
#include "support/fixtures.h"

#include "core/clock/clock.h"
#include "events/model/event.h"
#include "render/style/options.h"
#include "render/style/style.h"
#include "render/view/view.h"

enum { OUTPUT_BUF = 16384 };

static const Today NO_TODAY = {0, 0, 0, 0};
static const RenderOptions PLAIN = {0, 0, 0};
static const RenderOptions COLOR = {0, 0, 1};

static void fill_october_events(EventList *events) {
    fixture_add_event(events, 2026, 10, 15, "Team offsite");
    fixture_add_event(events, 2026, 10, 15, "Second thing");
    fixture_add_event(events, EVERY_YEAR, 10, 31, "Halloween");
    fixture_add_event(events, 2027, 10, 20, "Next year");
    fixture_add_event(events, EVERY_YEAR, 11, 5, "Other month");
}

static void render_month(char *buf, size_t cap, const RenderOptions *opts, const EventList *events) {
    FILE *f = tmpfile();
    view_print_month(f, 2026, 10, opts, &NO_TODAY, events);
    test_slurp(f, buf, cap);
    fclose(f);
}

static void test_legend_lists_only_this_months_events_in_order(void) {
    EventList events;
    eventlist_init(&events);
    fill_october_events(&events);
    char out[OUTPUT_BUF];

    render_month(out, sizeof out, &PLAIN, &events);

    CHECK(strstr(out, "15  Team offsite") != NULL);
    CHECK(strstr(out, "15  Second thing") != NULL);
    CHECK(strstr(out, "31  Halloween") != NULL);
    CHECK(strstr(out, "Next year") == NULL);      // another year
    CHECK(strstr(out, "Other month") == NULL);    // another month

    // Day order first, then the order they appear in the file.
    const char *a = strstr(out, "Team offsite");
    const char *b = strstr(out, "Second thing");
    const char *c = strstr(out, "Halloween");
    CHECK(a && b && c && a < b && b < c);

    eventlist_free(&events);
}

static void test_legend_is_separated_from_the_grid_by_one_blank_line(void) {
    EventList events;
    eventlist_init(&events);
    fill_october_events(&events);
    char out[OUTPUT_BUF];

    render_month(out, sizeof out, &PLAIN, &events);

    CHECK(strstr(out, "30 31\n\n15  Team offsite") != NULL);

    eventlist_free(&events);
}

static void test_month_without_events_looks_exactly_like_no_events_at_all(void) {
    EventList only_november;
    eventlist_init(&only_november);
    fixture_add_event(&only_november, EVERY_YEAR, 11, 5, "Other month");

    char with_list[OUTPUT_BUF], with_null[OUTPUT_BUF];
    render_month(with_list, sizeof with_list, &PLAIN, &only_november);
    render_month(with_null, sizeof with_null, &PLAIN, NULL);

    CHECK_STR(with_list, with_null);
    CHECK_INT(test_count_char(with_null, '\n'), 7);   // title, weekdays, 5 week rows, nothing else

    eventlist_free(&only_november);
}

static void test_legend_numbers_are_underlined_with_color(void) {
    EventList events;
    eventlist_init(&events);
    fill_october_events(&events);
    char out[OUTPUT_BUF];

    render_month(out, sizeof out, &COLOR, &events);

    CHECK(strstr(out, ANSI_UNDERLINE "15" ANSI_RESET "  Team offsite") != NULL);

    eventlist_free(&events);
}

static void test_year_view_underlines_but_has_no_legend(void) {
    EventList events;
    eventlist_init(&events);
    fill_october_events(&events);
    char out[OUTPUT_BUF];

    FILE *f = tmpfile();
    view_print_year(f, 2026, &COLOR, &NO_TODAY, &events);
    test_slurp(f, out, sizeof out);
    fclose(f);

    // 2026 has Oct 15 (one-off, twice listed on the same day but one cell),
    // Oct 31 and Nov 5 (both yearly): three distinct underlined days.
    CHECK_INT(test_count_substr(out, ANSI_UNDERLINE), 3);
    CHECK(strstr(out, "Team offsite") == NULL);
}

static void test_year_view_shows_all_twelve_months(void) {
    char out[OUTPUT_BUF];
    FILE *f = tmpfile();

    view_print_year(f, 2026, &PLAIN, &NO_TODAY, NULL);
    test_slurp(f, out, sizeof out);
    fclose(f);

    const char *names[] = {"January", "February", "March", "April", "May", "June",
                           "July", "August", "September", "October", "November", "December"};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        CHECK(strstr(out, names[i]) != NULL);
    }
    CHECK(strstr(out, "2026") != NULL);
    CHECK(strchr(out, '\033') == NULL);
}

int main(void) {
    RUN_TEST(test_legend_lists_only_this_months_events_in_order);
    RUN_TEST(test_legend_is_separated_from_the_grid_by_one_blank_line);
    RUN_TEST(test_month_without_events_looks_exactly_like_no_events_at_all);
    RUN_TEST(test_legend_numbers_are_underlined_with_color);
    RUN_TEST(test_year_view_underlines_but_has_no_legend);
    RUN_TEST(test_year_view_shows_all_twelve_months);
    return test_report(__FILE__);
}