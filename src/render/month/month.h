#ifndef CAL_RENDER_MONTH_MONTH_H
#define CAL_RENDER_MONTH_MONTH_H

#include "core/calendar/calendar.h"
#include "core/clock/clock.h"
#include "events/model/event.h"
#include "render/line/line.h"
#include "render/style/options.h"

enum {
    HEADER_LINES = 2,
    MAX_WEEK_ROWS = 6,
    LINES_PER_BLOCK = HEADER_LINES + MAX_WEEK_ROWS,
    CELL_WIDTH = 3,
    DAYS_WIDTH = DAYS_PER_WEEK * CELL_WIDTH - 1,
    WEEKNUM_WIDTH = 3
};

// Visible width of one rendered month block, escape codes not counted.
int month_block_width(const RenderOptions *opts);

// Renders one month into LINES_PER_BLOCK fixed-width lines and returns how
// many week rows actually have days. The year view needs all blocks the same
// height to interleave them, the single view uses the return value to skip
// the empty trailing rows. Days with an event are underlined, events may be
// NULL when none were loaded.
int month_render(int year, int month, int with_year, const RenderOptions *opts,
                 const Today *today, const EventList *events,
                 Line out[LINES_PER_BLOCK]);

#endif