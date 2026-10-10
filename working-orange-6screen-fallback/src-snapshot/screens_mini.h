#pragma once
#include <stdint.h>

#if defined(BOARD_MINI_S3_35)

// Screen orchestration for the mini-s3-35 six-screen dashboard. Drawing lives
// in ui.cpp; this module owns which screen is current and the ago-timer beat.
// No dwell/carousel timer — navigation is tap-zone only (left/right half),
// mirroring this board's PIN entry gesture rather than screens.cpp's
// auto-advancing carousel.
enum MiniScreen : uint8_t {
    MSCR_NOW = 0, MSCR_MODELS, MSCR_CHART5H, MSCR_CHART7D, MSCR_RHYTHM24, MSCR_CHART30D, MSCR_COUNT
};

void       screensMiniInit(const char* ip);   // header net info + boot screen
MiniScreen screensMiniCurrent();
void       screensMiniShow(MiniScreen s);   // full redraw of s (becomes current)
void       screensMiniNext();               // right tap-zone, wraps
void       screensMiniPrev();               // left tap-zone, wraps
void       screensMiniOnData();             // after a usage fetch: records history + redraws current screen
void       screensMiniTick();               // per-loop: 10s header (ago-timer) beat

#endif // BOARD_MINI_S3_35
