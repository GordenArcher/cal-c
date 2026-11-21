#include "render/view/view.h"

#include <stdio.h>
#include <string.h>

#include "core/calendar/calendar.h"
#include "render/line/line.h"
#include "render/month/month.h"
#include "render/style/style.h"

enum {
    YEAR_COLUMNS = 3,
    COLUMN_GAP = 3,
    TITLE_CAP = 32
};

void view_print_month(int year, int month, const RenderOptions *opts, const Today *today) {
    Line block[LINES_PER_BLOCK];
    int used = month_render(year, month, 1, opts, today, block);

    for (int i = 0; i < HEADER_LINES + used; i++) puts(block[i].text);
}

// Builds three blocks per row group and prints them line by line. This only
// works because month_render pads every line to a constant visible width. If
// that padding ever changes, the columns will drift apart.
void view_print_year(int year, const RenderOptions *opts, const Today *today) {
    const int width = month_block_width(opts);
    const int total_width = YEAR_COLUMNS * width + (YEAR_COLUMNS - 1) * COLUMN_GAP;

    char title[TITLE_CAP];
    snprintf(title, sizeof title, "%d", year);
    int left = (total_width - (int)strlen(title)) / 2;
    printf("%*s%s%s%s\n\n", left, "", style_apply(opts, ANSI_BOLD), title, style_apply(opts, ANSI_RESET));

    for (int m = 1; m <= MONTHS_PER_YEAR; m += YEAR_COLUMNS) {
        Line blocks[YEAR_COLUMNS][LINES_PER_BLOCK];
        int rows = HEADER_LINES;

        for (int k = 0; k < YEAR_COLUMNS; k++) {
            int used = month_render(year, m + k, 0, opts, today, blocks[k]);
            if (HEADER_LINES + used > rows) rows = HEADER_LINES + used;
        }

        for (int i = 0; i < rows; i++) {
            for (int k = 0; k < YEAR_COLUMNS; k++) {
                fputs(blocks[k][i].text, stdout);
                if (k < YEAR_COLUMNS - 1) printf("%*s", COLUMN_GAP, "");
            }
            putchar('\n');
        }
        if (m + YEAR_COLUMNS <= MONTHS_PER_YEAR) putchar('\n');
    }
}