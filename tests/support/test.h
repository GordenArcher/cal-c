#ifndef CAL_TESTS_SUPPORT_TEST_H
#define CAL_TESTS_SUPPORT_TEST_H

// Minimal test helpers, no framework. Every test file is its own program, so
// plain statics are enough for the counters. A test file must define
// _POSIX_C_SOURCE 200809L before including this (mkstemp needs it).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int test_checks = 0;
static int test_failures = 0;
static const char *test_current = "";

// Remembers which test function is running so a failure can name it.
#define RUN_TEST(fn) do { test_current = #fn; fn(); } while (0)

#define CHECK(cond) \
    do { \
        test_checks++; \
        if (!(cond)) { \
            test_failures++; \
            fprintf(stderr, "  FAIL %s (%s:%d)\n    check: %s\n", \
                    test_current, __FILE__, __LINE__, #cond); \
        } \
    } while (0)

#define CHECK_INT(actual, expected) \
    do { \
        long actual_ = (long)(actual); \
        long expected_ = (long)(expected); \
        test_checks++; \
        if (actual_ != expected_) { \
            test_failures++; \
            fprintf(stderr, "  FAIL %s (%s:%d)\n    %s\n    got %ld, want %ld\n", \
                    test_current, __FILE__, __LINE__, #actual, actual_, expected_); \
        } \
    } while (0)

#define CHECK_STR(actual, expected) \
    do { \
        const char *actual_ = (actual); \
        const char *expected_ = (expected); \
        test_checks++; \
        if (!actual_ || strcmp(actual_, expected_) != 0) { \
            test_failures++; \
            fprintf(stderr, "  FAIL %s (%s:%d)\n    %s\n    got \"%s\", want \"%s\"\n", \
                    test_current, __FILE__, __LINE__, #actual, \
                    actual_ ? actual_ : "(null)", expected_); \
        } \
    } while (0)

// Returns the process exit code: 0 when every check passed.
static inline int test_report(const char *file) {
    if (test_failures == 0) {
        printf("ok    %s (%d checks)\n", file, test_checks);
        return 0;
    }
    printf("FAIL  %s (%d of %d checks failed)\n", file, test_failures, test_checks);
    return 1;
}

// Reads everything written to f so far into buf, always NUL terminated.
static inline void test_slurp(FILE *f, char *buf, size_t cap) {
    fflush(f);
    rewind(f);
    size_t n = fread(buf, 1, cap - 1, f);
    buf[n] = '\0';
}

// Creates an empty unique file under /tmp and stores its path in path.
static inline int test_make_temp(char *path, size_t cap) {
    snprintf(path, cap, "/tmp/cal_test_XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0) return 0;
    close(fd);
    return 1;
}

static inline void test_write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

static inline int test_count_char(const char *s, char c) {
    int n = 0;
    for (; *s; s++) if (*s == c) n++;
    return n;
}

static inline int test_count_substr(const char *haystack, const char *needle) {
    int n = 0;
    size_t len = strlen(needle);
    for (const char *p = strstr(haystack, needle); p; p = strstr(p + len, needle)) n++;
    return n;
}

#endif