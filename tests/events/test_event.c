#define _POSIX_C_SOURCE 200809L

#include "support/test.h"
#include "support/fixtures.h"

#include "events/model/event.h"

enum { MANY_EVENTS = 100 };

static void test_one_off_matches_only_its_year(void) {
    Event e = {.year = 2026, .month = 12, .day = 25, .label = "Christmas"};

    CHECK(event_occurs_on(&e, 2026, 12, 25));
    CHECK(!event_occurs_on(&e, 2027, 12, 25));
    CHECK(!event_occurs_on(&e, 2026, 12, 24));
    CHECK(!event_occurs_on(&e, 2026, 11, 25));
}

static void test_yearly_matches_every_year(void) {
    Event e = {.year = EVERY_YEAR, .month = 12, .day = 25, .label = "Christmas"};

    CHECK(event_occurs_on(&e, 1, 12, 25));
    CHECK(event_occurs_on(&e, 2026, 12, 25));
    CHECK(event_occurs_on(&e, 9999, 12, 25));
    CHECK(!event_occurs_on(&e, 2026, 12, 26));
}

static void test_yearly_leap_day_skips_non_leap_years(void) {
    Event e = {.year = EVERY_YEAR, .month = 2, .day = 29, .label = "Leap day"};

    CHECK(event_occurs_on(&e, 2024, 2, 29));
    // It does not slide to Feb 28 or Mar 1 in years without a Feb 29.
    CHECK(!event_occurs_on(&e, 2023, 2, 28));
    CHECK(!event_occurs_on(&e, 2023, 3, 1));
}

static void test_empty_list_and_null_list_have_no_events(void) {
    EventList list;
    eventlist_init(&list);

    CHECK_INT(list.count, 0);
    CHECK(!eventlist_has_on(&list, 2026, 12, 25));
    CHECK(!eventlist_has_on(NULL, 2026, 12, 25));

    eventlist_free(&list);
}

static void test_has_on_finds_events(void) {
    EventList list;
    eventlist_init(&list);
    fixture_add_event(&list, 2026, 10, 15, "Offsite");
    fixture_add_event(&list, EVERY_YEAR, 10, 31, "Halloween");

    CHECK(eventlist_has_on(&list, 2026, 10, 15));
    CHECK(!eventlist_has_on(&list, 2027, 10, 15));
    CHECK(eventlist_has_on(&list, 2027, 10, 31));
    CHECK(!eventlist_has_on(&list, 2026, 10, 16));

    eventlist_free(&list);
}

// Past the initial capacity the list has to reallocate, and every event
// added before the move must still be intact afterwards.
static void test_list_grows_and_keeps_its_data(void) {
    EventList list;
    eventlist_init(&list);

    for (int i = 0; i < MANY_EVENTS; i++) {
        char label[EVENT_LABEL_CAP];
        snprintf(label, sizeof label, "event %d", i);
        fixture_add_event(&list, 2026, 1 + i % 12, 1 + i % 28, label);
    }

    CHECK_INT(list.count, MANY_EVENTS);
    CHECK(list.capacity >= list.count);
    CHECK_STR(list.items[0].label, "event 0");
    CHECK_STR(list.items[MANY_EVENTS - 1].label, "event 99");
    CHECK_INT(list.items[50].day, 1 + 50 % 28);

    eventlist_free(&list);
    CHECK(list.items == NULL);
    CHECK_INT(list.count, 0);
}

int main(void) {
    RUN_TEST(test_one_off_matches_only_its_year);
    RUN_TEST(test_yearly_matches_every_year);
    RUN_TEST(test_yearly_leap_day_skips_non_leap_years);
    RUN_TEST(test_empty_list_and_null_list_have_no_events);
    RUN_TEST(test_has_on_finds_events);
    RUN_TEST(test_list_grows_and_keeps_its_data);
    return test_report(__FILE__);
}