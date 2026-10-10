#pragma once
#include <stdint.h>

#if defined(BOARD_MINI_S3_35)

#include "api.h"

// 30 days of usage history at 5-minute resolution, feeding every mini-s3-35
// chart screen (5h/7d/24h-rhythm/30d) from one ring buffer. Slots are
// addressed by absolute index (epoch/300) so gaps while the device was off
// stay visible as gaps. Persisted to LittleFS only when the slot advances
// (~288 writes/day — the same wear-irrelevant cadence class as history.cpp's
// 336-slot ring). Modeled directly on history.h/.cpp; kept separate so
// DUST_UI boards stay untouched.
static const uint16_t HIST_MINI_SLOTS    = 8640;
static const uint32_t HIST_MINI_SLOT_SEC = 300;
static const uint8_t  HIST_MINI_EMPTY    = 0xFF;

struct HistMiniSlot {
    uint8_t h5;   // 0..100, HIST_MINI_EMPTY = no sample
    uint8_t d7;
};

void historyMiniInit();                       // mount LittleFS (format on first boot), load ring
void historyMiniRecord(const UsageData& u);    // call after each successful fetch
// Fills out[HIST_MINI_SLOTS] oldest→newest; newestEpoch = end time of the newest slot.
void historyMiniSnapshot(HistMiniSlot* out, uint32_t& newestEpoch);

#endif // BOARD_MINI_S3_35
