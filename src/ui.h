#pragma once
#include "api.h"
#ifdef MANGO_UI
#include "status.h"
#endif
#include "history.h"        // self-guarded: empty on non-S3 boards
#include "history_mini.h"   // self-guarded: empty off mini-s3-35
#include "screens_mini.h"   // self-guarded: empty off mini-s3-35
#include "news.h"

void uiInit();
void uiBootProgress(int percent, const char* label);
void uiSetupScreen(const char* apName, const char* apPass, bool reconfigure = false);
void uiPinScreen(int pos, const int digits[4]);
void uiConnecting(const char* ssid, int attempt = 0);
void uiDashboard(const UsageData& data, unsigned long lastFetchMs, int rssi, int batPct);
// Lightweight in-place update of the clock + reset countdowns (no bars, no full clear)
// so the periodic refresh doesn't flicker. Call when only time has passed, not data.
void uiDashboardClock(const UsageData& data, unsigned long lastFetchMs, int rssi);
void uiError(const char* title, const char* detail = nullptr);
// Lockout is drawn in two parts so the caller owns the wait: Static paints the
// chrome once, Tick repaints the remaining seconds. This lets the wait loop be
// sliced (and, on the T-Display S3, pump the web panel between slices).
void uiLockoutStatic(int attempts, int maxAttempts, int lockoutSec);
void uiLockoutTick(int secondsLeft);
#ifdef DUST_UI
// LAN URL shown on the PIN screen and alternated into the dashboard header.
void uiSetNetInfo(const char* url);
// Header label (from dev_name, uppercased); empty falls back to "CLAUDE USAGE".
void uiSetHeaderLabel(const char* name);
// Which model mascots render (bit per model, panel setting).
void uiSetModelMask(uint8_t mask);
// Flip the header's left text between label and URL (screensTick's 10s beat).
void uiHeaderAlternate();
// v3 carousel screens — all sprite-rendered, full-push, flicker-free.
void uiChartScreen(const HistSlot* slots, uint32_t newestEpoch,
                   unsigned long lastFetchMs, int rssi);
void uiNewsScreen(const NewsState& news, unsigned long lastFetchMs, int rssi);
void uiClockScreen(const UsageData& data, unsigned long lastFetchMs, int rssi);
#endif
#if defined(BOARD_CROWPANEL_ADV_35) || defined(BOARD_CROWPANEL_35)
// LAN IP shown in the dashboard header in place of the static "CLAUDE USAGE"
// label, cached once at boot.
void uiSetNetInfo(const char* ip);
#endif
#ifdef MANGO_UI
// Latest model health for the dashboard's mascot row; cached until the next call.
void uiSetModelStatus(const ModelStatus& s);
// Apply (or undo) the 180° flip and clear; caller redraws the current screen.
void uiApplyRotation(bool flipped);
// Close (true) or open (false) the healthy mascots' eyes on the dashboard.
void uiBlinkTick(bool closed);
#endif // MANGO_UI

#if defined(BOARD_MINI_S3_35)
// Six-screen dashboard header: device label + IP, cached once at boot
// (WiFi.localIP() doesn't change without a reconnect).
void uiMiniSetNetInfo(const char* label, const char* ip);
// Screen 1 — home: two side-by-side 5-HOUR/7-DAY cards, plus the existing
// MODELS breakdown retained along the bottom.
void uiMiniNow(const UsageData& data, unsigned long lastFetchMs, int rssi, MiniScreen current);
// Screen 2 — full-screen model health/availability detail (no per-model
// consumption data exists on this API — see the plan's header-dump experiment).
void uiMiniModels(unsigned long lastFetchMs, int rssi, MiniScreen current);
// Screens 3/4 — one parameterized window chart shared by both fields.
void uiMiniWindowChart(bool isH5, const UsageData& data, const HistMiniSlot* slots,
                       uint32_t newestEpoch, unsigned long lastFetchMs, int rssi, MiniScreen current);
// Screen 5 — 24-bar histogram of quota burned per local hour of day, today.
void uiMiniRhythm24(const HistMiniSlot* slots, uint32_t newestEpoch,
                    unsigned long lastFetchMs, int rssi, MiniScreen current);
// Screen 6 — 30-bar histogram of quota burned per calendar day.
void uiMiniChart30D(const HistMiniSlot* slots, uint32_t newestEpoch,
                    unsigned long lastFetchMs, int rssi, MiniScreen current);
// Lightweight header-only repaint for the 10s ago-timer beat (screensMiniTick()).
void uiMiniHeaderTick(unsigned long lastFetchMs, int rssi);
#endif // BOARD_MINI_S3_35
