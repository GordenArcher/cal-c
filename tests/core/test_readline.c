#define _POSIX_C_SOURCE 200809L

#include "support/test.h"

#include "core/file/readline.h"

static void test_reads_normal_lines(void) {
    FILE *f = tmpfile();
    fputs("one\ntwo\n", f);
    rewind(f);

    char buf[16];
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_OK);
    CHECK_STR(buf, "one\n");
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_OK);
    CHECK_STR(buf, "two\n");
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_EOF);

    fclose(f);
}

static void test_last_line_without_newline(void) {
    FILE *f = tmpfile();
    fputs("last", f);
    rewind(f);

    char buf[16];
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_OK);
    CHECK_STR(buf, "last");
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_EOF);

    fclose(f);
}

// The tail of an overlong line must be swallowed, or the next read would see
// it as a separate line and warn about a line that does not exist.
static void test_overlong_line_discards_the_rest(void) {
    FILE *f = tmpfile();
    for (int i = 0; i < 40; i++) fputc('x', f);
    fputs("\nnext\n", f);
    rewind(f);

    char buf[16];
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_TOO_LONG);
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_OK);
    CHECK_STR(buf, "next\n");
    CHECK_INT(file_read_line(f, buf, sizeof buf), LINE_EOF);

    fclose(f);
}

int main(void) {
    RUN_TEST(test_reads_normal_lines);
    RUN_TEST(test_last_line_without_newline);
    RUN_TEST(test_overlong_line_discards_the_rest);
    return test_report(__FILE__);
}
