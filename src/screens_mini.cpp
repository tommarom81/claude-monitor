#include "screens_mini.h"

#if defined(BOARD_MINI_S3_35)

#include <Arduino.h>
#include <WiFi.h>
#include "app_state.h"
#include "hal.h"
#include "ui.h"
#include "history_mini.h"

static MiniScreen    s_current    = MSCR_NOW;
static unsigned long s_lastBeatMs = 0;   // 10s ago-timer beat
// Shared 17KB snapshot scratch buffer for every chart/histogram screen — only
// one screen is ever on-display at a time, so one buffer (not one per case)
// keeps this off the precious internal-RAM budget threefold.
static HistMiniSlot s_snapSlots[HIST_MINI_SLOTS];

void screensMiniInit(const char* ip) {
    uiMiniSetNetInfo(g_settings.devName, ip);
    historyMiniInit();
    s_current = MSCR_NOW;
}

MiniScreen screensMiniCurrent() { return s_current; }

void screensMiniShow(MiniScreen s) {
    s_current = s;
    int rssi = WiFi.RSSI();

    switch (s) {
        case MSCR_NOW:
            uiMiniNow(g_usage, g_lastFetchMs, rssi, s_current);
            break;
        case MSCR_MODELS:
            uiMiniModels(g_lastFetchMs, rssi, s_current);
            break;
        case MSCR_CHART5H:
        case MSCR_CHART7D: {
            uint32_t newest;
            historyMiniSnapshot(s_snapSlots, newest);
            uiMiniWindowChart(s == MSCR_CHART5H, g_usage, s_snapSlots, newest, g_lastFetchMs, rssi, s_current);
            break;
        }
        case MSCR_RHYTHM24: {
            uint32_t newest;
            historyMiniSnapshot(s_snapSlots, newest);
            uiMiniRhythm24(s_snapSlots, newest, g_lastFetchMs, rssi, s_current);
            break;
        }
        case MSCR_CHART30D: {
            uint32_t newest;
            historyMiniSnapshot(s_snapSlots, newest);
            uiMiniChart30D(s_snapSlots, newest, g_lastFetchMs, rssi, s_current);
            break;
        }
        default:
            break;
    }
}

void screensMiniNext() {
    screensMiniShow((MiniScreen)((s_current + 1) % MSCR_COUNT));
}

void screensMiniPrev() {
    screensMiniShow((MiniScreen)((s_current + MSCR_COUNT - 1) % MSCR_COUNT));
}

void screensMiniOnData() {
    historyMiniRecord(g_usage);
    screensMiniShow(s_current);   // no dwell timer to preserve — always redraw live
}

void screensMiniTick() {
    unsigned long nowMs = millis();
    if (nowMs - s_lastBeatMs >= 10000) {
        s_lastBeatMs = nowMs;
        uiMiniHeaderTick(g_lastFetchMs, WiFi.RSSI());
    }
}

#endif // BOARD_MINI_S3_35
