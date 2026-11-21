#include "render/style/style.h"

// Colors are gated here once, so render code never branches on use_color.
const char *style_apply(const RenderOptions *opts, const char *code) {
    return opts->use_color ? code : "";
}