#include "core/file/readline.h"

#include <string.h>

LineStatus file_read_line(FILE *f, char *buf, size_t cap) {
    if (!fgets(buf, cap, f)) return LINE_EOF;

    size_t len = strlen(buf);
    if (len == cap - 1 && buf[len - 1] != '\n') {
        int c;
        while ((c = fgetc(f)) != EOF && c != '\n') {}
        return LINE_TOO_LONG;
    }

    return LINE_OK;
}
