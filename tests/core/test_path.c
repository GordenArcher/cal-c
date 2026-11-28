#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "core/path/path.h"

static void test_first_available_wins(void) {
    char buf[64];

    CHECK(path_first_available("/explicit", "/env", "/home", ".default", buf, sizeof buf));
    CHECK_STR(buf, "/explicit");

    CHECK(path_first_available(NULL, "/env", "/home", ".default", buf, sizeof buf));
    CHECK_STR(buf, "/env");

    CHECK(path_first_available(NULL, "", "/home", ".default", buf, sizeof buf));
    CHECK_STR(buf, "/home/.default");
}

static void test_nothing_available(void) {
    char buf[16];

    CHECK(!path_first_available(NULL, NULL, NULL, ".default", buf, sizeof buf));
    CHECK(!path_first_available(NULL, NULL, "", ".default", buf, sizeof buf));
    CHECK(!path_first_available("", NULL, NULL, ".default", buf, sizeof buf));
    CHECK(!path_first_available("/a/very/long/path", NULL, NULL, ".default", buf, sizeof buf));
}

// The buffer must fit the path plus the terminating NUL.
static void test_exact_capacity(void) {
    char exactly[5];
    char too_small[4];

    CHECK(path_first_available("abcd", NULL, NULL, ".default", exactly, sizeof exactly));
    CHECK_STR(exactly, "abcd");
    CHECK(!path_first_available("abcd", NULL, NULL, ".default", too_small, sizeof too_small));
}

int main(void) {
    RUN_TEST(test_first_available_wins);
    RUN_TEST(test_nothing_available);
    RUN_TEST(test_exact_capacity);
    return test_report(__FILE__);
}
