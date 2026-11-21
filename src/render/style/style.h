#ifndef CAL_RENDER_STYLE_STYLE_H
#define CAL_RENDER_STYLE_STYLE_H

#include "render/style/options.h"

#define ANSI_RESET     "\033[0m"
#define ANSI_BOLD      "\033[1m"
#define ANSI_DIM       "\033[2m"
#define ANSI_UNDERLINE "\033[4m"
#define ANSI_REVERSE   "\033[7m"
#define ANSI_RED       "\033[31m"

// Returns the code when color is on, "" when off.
const char *style_apply(const RenderOptions *opts, const char *code);

#endif