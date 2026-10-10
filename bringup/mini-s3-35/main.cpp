// Bring-up sketch for BOARD_MINI_S3_35 (Guition/stemedu JC3248W535C_I,
// ESP32-S3 + AXS15231B over QSPI). Not part of the normal firmware build —
// run with `pio run -e mini-s3-35-bringup -t upload && pio device monitor`.
//
// Goal: confirm the pin map and panel init before investing in the full UI
// (see wiki/Mini-ESP32-S3-35.md and the hardware notes in README.md).
//
//   1. I2C scan            — expect the touch controller at 0x3B
//   2. Color bars over QSPI — confirms wiring and byte order (no swap needed
//                             if bars show RED/GREEN/BLUE/WHITE as labeled)
//   3. Raw touch coordinates — printed on every touch event
//
// Uses Arduino_GFX's own Arduino_AXS15231B class directly (plain fillRect/
// fillScreen calls) at the vendored v1.6.0 — v1.6.1+ has a documented
// AXS15231B QSPI init regression on this exact panel (Guition JC3248W535,
// moononournation/Arduino_GFX#803); v1.6.0 is the community-verified
// last-good version, so it's the vendored copy under lib/.
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "touch_axs15231b.h"

#define QSPI_CS    45
#define QSPI_SCK   47
#define QSPI_D0    21
#define QSPI_D1    48
#define QSPI_D2    40
#define QSPI_D3    39
#define BL_PIN     1
#define TOUCH_SDA  4
#define TOUCH_SCL  8
#define TOUCH_INT  3
#define TOUCH_ADDR 0x3B
#define QSPI_FREQ  40000000UL

Arduino_GFX *panel = nullptr;

// rotation=1 matches the software rotate direction the real firmware's
// halFlush() uses (hardware-verified; see wiki page).
AXS15231B_Touch touch_dev(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_ADDR, 1);

static void i2cScan() {
    Serial.println("--- I2C scan ---");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  found device at 0x%02X%s\n", addr,
                          addr == TOUCH_ADDR ? "  <-- expected touch controller" : "");
            found++;
        }
    }
    if (found == 0) Serial.println("  no I2C devices found — check TOUCH_SDA/TOUCH_SCL wiring");
    Serial.println("--- I2C scan done ---");
}

// Composes all 6 bands into one PSRAM buffer, then pushes it in a single
// draw16bitRGBBitmap() call — matching how the real firmware's halFlush()
// always pushes one whole 320x480 frame per update, never a sequence of
// separate small fillRect() calls. (A first version of this test used 6
// separate fillRect() calls and only the last one rendered — that's a
// different, multi-partial-update code path this project's real usage
// never exercises, so it's not worth chasing further here.)
static void colorBars() {
    const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, YELLOW, CYAN};
    const char *names[]     = {"RED", "GREEN", "BLUE", "WHITE", "YELLOW", "CYAN"};
    const int n = sizeof(colors) / sizeof(colors[0]);
    const int bandH = 480 / n;

    uint16_t *buf = (uint16_t *)heap_caps_malloc((size_t)320 * 480 * 2, MALLOC_CAP_SPIRAM);
    if (!buf) {
        Serial.println("FATAL: color bar buffer alloc failed (PSRAM)");
        return;
    }
    for (int i = 0; i < n; i++) {
        uint16_t *band = buf + (size_t)i * bandH * 320;
        for (int p = 0; p < bandH * 320; p++) band[p] = colors[i];
        Serial.printf("  band %d: %s\n", i, names[i]);
    }
    panel->draw16bitRGBBitmap(0, 0, buf, 320, 480);
    free(buf);
}

void setup() {
    Serial.begin(115200);
    delay(1200);
    Serial.println("\n=== mini-s3-35 bring-up ===");

    // touch_dev.begin() owns Wire init (Wire.begin(TOUCH_SDA, TOUCH_SCL, 400000))
    // — do not call Wire.begin() again before this, double-init locks up the
    // ESP32 I2C driver.
    bool touchOk = touch_dev.begin();
    Serial.printf("Touch begin(): %s\n", touchOk ? "OK" : "FAILED");
    i2cScan();

    Arduino_DataBus *bus = new Arduino_ESP32QSPI(QSPI_CS, QSPI_SCK, QSPI_D0, QSPI_D1, QSPI_D2, QSPI_D3);
    // rotation=0, native portrait 320x480 — a non-zero controller rotation
    // corrupts colors on this panel (see wiki page); keep it at 0.
    panel = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, 320, 480);
    if (!panel->begin(QSPI_FREQ)) {
        Serial.println("FATAL: panel->begin() failed");
        while (1) delay(1000);
    }
    Serial.println("Panel: OK");

    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, HIGH);

    Serial.println("--- single fill test: RED (hold 4s) ---");
    panel->fillScreen(RED);
    delay(4000);

    Serial.println("--- single fill test: GREEN (hold 4s) ---");
    panel->fillScreen(GREEN);
    delay(4000);

    Serial.println("--- color bars ---");
    colorBars();

    Serial.println("=== bring-up ready — touch the screen ===");
}

void loop() {
    uint16_t x, y;
    if (touch_dev.touched()) {
        touch_dev.readData(&x, &y);
        Serial.printf("touch: x=%u y=%u\n", x, y);
    }
    delay(20);
}
