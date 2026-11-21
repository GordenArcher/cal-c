#ifndef CAL_RENDER_VIEW_VIEW_H
#define CAL_RENDER_VIEW_VIEW_H

#include "core/clock/clock.h"
#include "render/style/options.h"

void view_print_month(int year, int month, const RenderOptions *opts, const Today *today);
void view_print_year(int year, const RenderOptions *opts, const Today *today);

#endif