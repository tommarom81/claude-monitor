#include "history_mini.h"

#if defined(BOARD_MINI_S3_35)

#include <Arduino.h>
#include <LittleFS.h>
#include <time.h>

// NTP sanity floor: epochs below this mean the clock isn't set and slot math
// would poison the ring.
static const uint32_t TIME_SANE_EPOCH = 1700000000UL;
static const char*    HIST_MINI_PATH  = "/hist_mini.bin";

struct HistMiniFile {
    uint32_t magic;         // 'CUH2'
    uint16_t version;
    uint16_t reserved;
    uint32_t lastAbsSlot;   // absolute slot (epoch/300) of the newest sample
    HistMiniSlot ring[HIST_MINI_SLOTS];
};
static const uint32_t HIST_MINI_MAGIC   = 0x32485543;   // "CUH2" little-endian
static const uint16_t HIST_MINI_VERSION = 1;

static HistMiniFile s_hist;
static bool         s_fsOk = false;

static void clearRing() {
    memset(s_hist.ring, HIST_MINI_EMPTY, sizeof(s_hist.ring));
    s_hist.lastAbsSlot = 0;
}

static void persist() {
    if (!s_fsOk) return;
    File f = LittleFS.open(HIST_MINI_PATH, "w");
    if (!f) return;
    f.write((const uint8_t*)&s_hist, sizeof(s_hist));
    f.close();
}

void historyMiniInit() {
    s_hist.magic   = HIST_MINI_MAGIC;
    s_hist.version = HIST_MINI_VERSION;
    clearRing();

    // The data partition is labeled "spiffs" in the table; LittleFS mounts it
    // fine (that label is arduino-esp32's default, passed explicitly to make
    // the dependency visible).
    s_fsOk = LittleFS.begin(false, "/littlefs", 10, "spiffs");
    if (!s_fsOk) {
        // formatOnFail=true only reformats when esp_vfs_littlefs_register()
        // returns exactly ESP_FAIL; a pre-existing but LittleFS-incompatible
        // partition (e.g. this board's factory image using "spiffs" for
        // something else) can surface a different error and skip that path
        // silently — seen on hardware as "Corrupted dir pair" / mount -84
        // with formatOnFail=true still leaving the partition unmounted. Force
        // the format explicitly instead of trusting the implicit one.
        Serial.println("[historyMini] mount failed, formatting...");
        if (LittleFS.format()) {
            s_fsOk = LittleFS.begin(false, "/littlefs", 10, "spiffs");
        }
        if (!s_fsOk) {
            Serial.println("[historyMini] format+mount failed — history disabled this boot");
            return;
        }
    }

    File f = LittleFS.open(HIST_MINI_PATH, "r");
    if (!f) return;
    // Read straight into s_hist rather than a local HistFile-sized temporary —
    // at HIST_MINI_SLOTS' size (~17KB) a stack copy blows the ~8KB loopTask
    // stack outright. A failed/invalid read is re-cleared below, same as the
    // clearRing() already done above for the mount-failed path.
    bool ok = f.read((uint8_t*)&s_hist, sizeof(s_hist)) == sizeof(s_hist) &&
              s_hist.magic == HIST_MINI_MAGIC && s_hist.version == HIST_MINI_VERSION;
    f.close();
    if (!ok) {
        s_hist.magic   = HIST_MINI_MAGIC;
        s_hist.version = HIST_MINI_VERSION;
        clearRing();
    }
}

void historyMiniRecord(const UsageData& u) {
    if (!u.ok) return;
    uint32_t now = (uint32_t)time(nullptr);
    if (now < TIME_SANE_EPOCH) return;

    uint32_t absSlot = now / HIST_MINI_SLOT_SEC;
    if (s_hist.lastAbsSlot == 0) {
        clearRing();
    } else if (absSlot > s_hist.lastAbsSlot) {
        // Blank everything skipped while the device was off/failing so old
        // wrap-around samples can't masquerade as fresh ones.
        uint32_t gap = absSlot - s_hist.lastAbsSlot;
        if (gap >= HIST_MINI_SLOTS) {
            clearRing();
        } else {
            for (uint32_t s = s_hist.lastAbsSlot + 1; s <= absSlot; s++) {
                s_hist.ring[s % HIST_MINI_SLOTS].h5 = HIST_MINI_EMPTY;
                s_hist.ring[s % HIST_MINI_SLOTS].d7 = HIST_MINI_EMPTY;
            }
        }
    }

    HistMiniSlot& slot = s_hist.ring[absSlot % HIST_MINI_SLOTS];
    slot.h5 = (uint8_t)constrain((int)(u.h5 + 0.5f), 0, 100);
    slot.d7 = (uint8_t)constrain((int)(u.d7 + 0.5f), 0, 100);

    if (absSlot != s_hist.lastAbsSlot) {
        s_hist.lastAbsSlot = absSlot;
        persist();
    }
}

void historyMiniSnapshot(HistMiniSlot* out, uint32_t& newestEpoch) {
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t nowAbs = (now >= TIME_SANE_EPOCH) ? now / HIST_MINI_SLOT_SEC : s_hist.lastAbsSlot;
    if (nowAbs == 0) nowAbs = HIST_MINI_SLOTS;   // nothing recorded and no clock: all-empty window
    newestEpoch = (nowAbs + 1) * HIST_MINI_SLOT_SEC;

    for (uint16_t i = 0; i < HIST_MINI_SLOTS; i++) {
        uint32_t absSlot = nowAbs - (HIST_MINI_SLOTS - 1) + i;
        bool valid = s_hist.lastAbsSlot != 0 &&
                     absSlot <= s_hist.lastAbsSlot &&
                     absSlot + HIST_MINI_SLOTS > s_hist.lastAbsSlot;
        out[i] = valid ? s_hist.ring[absSlot % HIST_MINI_SLOTS]
                       : HistMiniSlot{HIST_MINI_EMPTY, HIST_MINI_EMPTY};
    }
}

#endif // BOARD_MINI_S3_35
