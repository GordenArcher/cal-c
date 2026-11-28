#include "render/view/view.h"

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

// Lists the month's marks under the grid, in day order. Holidays come before
// events on a shared day and carry a '*' so they read as shared calendar
// context; events keep their underlined day. The blank separator line only
// appears when there is something to list, so a month without marks looks
// exactly like it did before events and holidays existed.
static void print_legend(FILE *out, int year, int month, const RenderOptions *opts,
                         const EventList *events, const EventList *holidays) {
    if (!events && !holidays) return;

    const int total = date_days_in_month(year, month);
    int printed_any = 0;

    for (int day = 1; day <= total; day++) {
        if (holidays) {
            for (size_t i = 0; i < holidays->count; i++) {
                const Event *holiday = &holidays->items[i];
                if (!event_occurs_on(holiday, year, month, day)) continue;

                if (!printed_any) fputc('\n', out);
                printed_any = 1;

                fprintf(out, "%s*%2d%s  %s\n",
                        style_apply(opts, ANSI_BOLD), day, style_apply(opts, ANSI_RESET),
                        holiday->label);
            }
        }

        if (events) {
            for (size_t i = 0; i < events->count; i++) {
                const Event *event = &events->items[i];
                if (!event_occurs_on(event, year, month, day)) continue;

                if (!printed_any) fputc('\n', out);
                printed_any = 1;

                fprintf(out, "%s%2d%s  %s\n",
                        style_apply(opts, ANSI_UNDERLINE), day, style_apply(opts, ANSI_RESET),
                        event->label);
            }
        }
    }
}

void view_print_month(FILE *out, int year, int month, const RenderOptions *opts,
                      const Today *today, const EventList *events, const EventList *holidays) {
    Line block[LINES_PER_BLOCK];
    int used = month_render(year, month, 1, opts, today, events, holidays, block);

    for (int i = 0; i < HEADER_LINES + used; i++) {
        fputs(block[i].text, out);
        fputc('\n', out);
    }
    print_legend(out, year, month, opts, events, holidays);
}

// Builds three blocks per row group and prints them line by line. This only
// works because month_render pads every line to a constant visible width. If
// that padding ever changes, the columns will drift apart.
void view_print_year(FILE *out, int year, const RenderOptions *opts,
                     const Today *today, const EventList *events, const EventList *holidays) {
    const int width = month_block_width(opts);
    const int total_width = YEAR_COLUMNS * width + (YEAR_COLUMNS - 1) * COLUMN_GAP;

    char title[TITLE_CAP];
    snprintf(title, sizeof title, "%d", year);
    int left = (total_width - (int)strlen(title)) / 2;
    fprintf(out, "%*s%s%s%s\n\n", left, "", style_apply(opts, ANSI_BOLD), title, style_apply(opts, ANSI_RESET));

    for (int m = 1; m <= MONTHS_PER_YEAR; m += YEAR_COLUMNS) {
        Line blocks[YEAR_COLUMNS][LINES_PER_BLOCK];
        int rows = HEADER_LINES;

        for (int k = 0; k < YEAR_COLUMNS; k++) {
            int used = month_render(year, m + k, 0, opts, today, events, holidays, blocks[k]);
            if (HEADER_LINES + used > rows) rows = HEADER_LINES + used;
        }

        for (int i = 0; i < rows; i++) {
            for (int k = 0; k < YEAR_COLUMNS; k++) {
                fputs(blocks[k][i].text, out);
                if (k < YEAR_COLUMNS - 1) fprintf(out, "%*s", COLUMN_GAP, "");
            }
            fputc('\n', out);
        }
        if (m + YEAR_COLUMNS <= MONTHS_PER_YEAR) fputc('\n', out);
    }
}