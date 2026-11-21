#ifndef CAL_RENDER_VIEW_VIEW_H
#define CAL_RENDER_VIEW_VIEW_H

#include <stdio.h>

#include "core/clock/clock.h"
#include "events/model/event.h"
#include "render/style/options.h"

// Both views write to out instead of assuming stdout, so tests can capture
// the output in a temp file. events may be NULL.
void view_print_month(FILE *out, int year, int month, const RenderOptions *opts,
                      const Today *today, const EventList *events);

// The year view only underlines event days. A legend for twelve months at
// once would bury the grid, use the month view to read the labels.
void view_print_year(FILE *out, int year, const RenderOptions *opts,
                     const Today *today, const EventList *events);

#endif