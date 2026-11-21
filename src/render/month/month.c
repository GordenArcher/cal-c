#include "render/month/month.h"

#include <stdio.h>
#include <string.h>

#include "core/calendar/calendar.h"
#include "render/style/style.h"

enum { TITLE_CAP = 32 };

int month_block_width(const RenderOptions *opts) {
    return DAYS_WIDTH + (opts->show_weeks ? WEEKNUM_WIDTH : 0);
}

int month_render(int year, int month, int with_year, const RenderOptions *opts,
                 const Today *today, Line out[LINES_PER_BLOCK]) {
    const int width = month_block_width(opts);
    const char *dim = style_apply(opts, ANSI_DIM);
    const char *reset = style_apply(opts, ANSI_RESET);

    for (int i = 0; i < LINES_PER_BLOCK; i++) line_reset(&out[i]);

    char title[TITLE_CAP];
    if (with_year) snprintf(title, sizeof title, "%s %d", MONTH_NAMES[month - 1], year);
    else           snprintf(title, sizeof title, "%s", MONTH_NAMES[month - 1]);

    int title_len = (int)strlen(title);
    int left = (width - title_len) / 2;
    line_pad(&out[0], left);
    line_add(&out[0], "%s%s%s", style_apply(opts, ANSI_BOLD), title, reset);
    line_pad(&out[0], width - left - title_len);

    if (opts->show_weeks) line_add(&out[1], "%sWk%s ", dim, reset);
    for (int c = 0; c < DAYS_PER_WEEK; c++) {
        int dow = opts->monday_first ? (c + 1) % DAYS_PER_WEEK : c;
        line_add(&out[1], "%s%s%s", dim, DAY_NAMES[dow], reset);
        if (c < DAYS_PER_WEEK - 1) line_add(&out[1], " ");
    }

    const int first_dow = date_day_of_week(year, month, 1);
    const int offset = opts->monday_first ? (first_dow + DAYS_PER_WEEK - 1) % DAYS_PER_WEEK : first_dow;
    const int total = date_days_in_month(year, month);
    int used_rows = 0;

    for (int r = 0; r < MAX_WEEK_ROWS; r++) {
        Line *ln = &out[HEADER_LINES + r];
        const int row_first = r * DAYS_PER_WEEK - offset + 1;
        int row_last = row_first + DAYS_PER_WEEK - 1;
        if (row_last > total) row_last = total;

        if (row_first > total) {
            line_pad(ln, width);
            continue;
        }
        used_rows++;

        // Week number comes from the LAST in-month day of the row. With a
        // Sunday start, the Sunday belongs to the previous ISO week but
        // Mon to Sat belong to the next, so the last day gives the right
        // answer for both start modes. Using the first day would be wrong
        // for Sunday-first layouts.
        if (opts->show_weeks) line_add(ln, "%s%2d%s ", dim, iso_week_number(year, month, row_last), reset);

        for (int c = 0; c < DAYS_PER_WEEK; c++) {
            int day = row_first + c;

            if (day < 1 || day > total) {
                line_pad(ln, CELL_WIDTH - 1);
            } else {
                int dow = (first_dow + day - 1) % DAYS_PER_WEEK;
                int weekend = (dow == SUNDAY || dow == SATURDAY);
                int is_today = today->valid && year == today->year &&
                               month == today->month && day == today->day;

                line_add(ln, "%s%s%2d%s",
                         is_today ? style_apply(opts, ANSI_REVERSE) : "",
                         weekend ? style_apply(opts, ANSI_RED) : "",
                         day, reset);
            }
            if (c < DAYS_PER_WEEK - 1) line_add(ln, " ");
        }
    }
    return used_rows;
}