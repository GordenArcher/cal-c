#include "holidays/store/store.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "core/file/readline.h"
#include "core/path/path.h"
#include "events/format/format.h"
#include "holidays/rules/rules.h"

#define DEFAULT_FILE_NAME    ".cal_holidays"
#define ENV_HOLIDAYS_PATH    "CAL_HOLIDAYS"

int holidays_resolve_path(const char *explicit_path, char *buf, size_t cap) {
    return path_first_available(explicit_path, getenv(ENV_HOLIDAYS_PATH),
                                getenv("HOME"), DEFAULT_FILE_NAME, buf, cap);
}

static void warn_line(FILE *warn, const char *path, int lineno, const char *reason) {
    if (warn) fprintf(warn, "%s:%d: %s, line skipped\n", path, lineno, reason);
}

HolidaysStatus holidays_load(const char *path, int year, EventList *out, FILE *warn) {
    FILE *f = fopen(path, "r");
    if (!f) return (errno == ENOENT) ? HOLIDAYS_NOT_FOUND : HOLIDAYS_ERROR;

    char buf[FORMAT_LINE_CAP];
    int lineno = 0;

    LineStatus line_status;
    while ((line_status = file_read_line(f, buf, sizeof buf)) != LINE_EOF) {
        lineno++;

        if (line_status == LINE_TOO_LONG) {
            warn_line(warn, path, lineno, "line too long");
            continue;
        }

        const char *start = buf;
        while (isspace((unsigned char)*start)) start++;
        if (*start == '\0' || *start == '#') continue;

        // Split on the FIRST '|' only, so a label may contain '|' itself.
        const char *sep = strchr(start, '|');
        if (!sep) {
            warn_line(warn, path, lineno, format_status_message(FORMAT_NO_SEPARATOR));
            continue;
        }

        size_t date_len = (size_t)(sep - start);
        while (date_len > 0 && isspace((unsigned char)start[date_len - 1])) date_len--;
        if (date_len == 0) {
            warn_line(warn, path, lineno, format_status_message(FORMAT_BAD_DATE));
            continue;
        }

        char date[FORMAT_LINE_CAP];
        memcpy(date, start, date_len);
        date[date_len] = '\0';

        int month = 0;
        int day = 0;
        RuleStatus rule = rule_resolve(date, year, &month, &day);
        if (rule == RULE_SKIP) continue;
        if (rule != RULE_OK) {
            warn_line(warn, path, lineno, rule_status_message(rule));
            continue;
        }

        // Zeroed first so every field this loader does not set is 0, matching
        // what the events loader guarantees for its source_line.
        Event holiday;
        memset(&holiday, 0, sizeof holiday);
        holiday.year = year;
        holiday.month = month;
        holiday.day = day;

        FormatStatus label = format_parse_label(sep + 1, holiday.label, sizeof holiday.label);
        if (label != FORMAT_OK) {
            warn_line(warn, path, lineno, format_status_message(label));
            continue;
        }

        holiday.source_line = lineno;
        if (eventlist_add(out, &holiday) != 0) {
            fclose(f);
            return HOLIDAYS_ERROR;
        }
    }

    int failed = ferror(f);
    fclose(f);
    return failed ? HOLIDAYS_ERROR : HOLIDAYS_OK;
}
