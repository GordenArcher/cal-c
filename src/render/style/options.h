#ifndef CAL_RENDER_STYLE_OPTIONS_H
#define CAL_RENDER_STYLE_OPTIONS_H

// Plain data shared by every renderer. Kept in its own header so cli can fill
// it in without pulling in any drawing code.
typedef struct {
    int monday_first;
    int show_weeks;
    int use_color;
} RenderOptions;

#endif