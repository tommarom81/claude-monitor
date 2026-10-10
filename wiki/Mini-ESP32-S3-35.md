# Mini ESP32-S3 3.5" (Guition/stemedu JC3248W535C_I, AXS15231B)

Part of [Claude Usage Stick](Home). A 3.5" 480×320 IPS capacitive-touch panel driven over **QSPI** by an **AXS15231B** controller (also handling touch, on I²C), no physical buttons. Native panel orientation is 320×480 portrait; the firmware runs it landscape by rotating pixels in software on every frame. Hardware-validated: pin map, rotation direction, and colors are all confirmed on a real unit, not just from reference docs.

## Specs

| | |
| --- | --- |
| MCU | ESP32-S3 (dual-core, WiFi, 8 MB PSRAM, 16 MB flash) |
| Display | 3.5" AXS15231B IPS, 480×320 landscape (native 320×480 portrait), QSPI |
| Touch | AXS15231B capacitive, I²C, addr `0x3B` |
| Battery | Board supports Li-ion power; ADC pin identified (GPIO 5) but readout not yet implemented (see Notes) |
| Buttons | None — touch zones, see Controls |
| PlatformIO env | `mini-s3-35` |

## How the display is driven

TFT_eSPI has no AXS15231B driver, so — the same trick upstream already uses for the T-Display S3 AMOLED — TFT_eSPI is only a dependency, used purely as an in-memory `TFT_eSprite` canvas. The UI in `ui.cpp` draws into that sprite exactly as it does for every other board; the AXS15231B-specific part is pushing the finished frame to the panel:

1. The panel (`Arduino_AXS15231B` via `Arduino_GFX`) is opened at **rotation 0**, its native 320×480 portrait orientation, and pushed to in one shot per frame (`draw16bitRGBBitmap`) — never a sequence of small partial updates. That distinction matters: this exact panel's QSPI mode only reliably supports single whole-frame pushes (see below).
2. `halFlush()` rotates the 480×320 landscape sprite into a second, native-orientation 320×480 PSRAM buffer, then pushes that buffer over QSPI in one transfer. The rotation direction is hardware-verified (a color-bar test confirmed the bands land in the right order).
3. Touch is read at **rotation 1**, matching the display's software rotation direction. Display and touch orientation must always move together — get one without the other and touch ends up mirrored relative to what's on screen.

### The GFX Library version matters — pin it at 1.6.0

`moononournation/GFX Library for Arduino` **1.6.1 and later have a real regression** in the AXS15231B QSPI init sequence for this exact panel ([moononournation/Arduino_GFX#803](https://github.com/moononournation/Arduino_GFX/issues/803), [esp-arduino-libs/ESP32_Display_Panel#164](https://github.com/esp-arduino-libs/ESP32_Display_Panel/issues/164)) — on 1.6.1 the screen shows static/noise with at most one correctly-rendered region; 1.6.0 is the community-verified last-good version and is what this project vendors (see `lib/GFX_Library_for_Arduino/`, checked into the repo rather than pulled via `lib_deps`, with one unrelated file patched — see the comment block above `[env:mini-s3-35]` in `platformio.ini`).

A second, separate finding from testing on real hardware: even at a working library version, **only a single full-frame push renders correctly** — a sequence of several smaller `fillRect()` calls at different Y offsets (as a naive multi-band test does) only shows the last one. This doesn't affect the real firmware (`halFlush()` always pushes one whole frame), but matters if you extend this driver: don't call multiple partial updates in a row against this panel. This lines up with an [espressif/esp-bsp issue](https://github.com/espressif/esp-bsp/issues/724) noting the official vendor driver deliberately skips the row-address-set (RASET) command in QSPI mode, which breaks partial-region updates by design — Arduino_GFX's driver doesn't skip it, but something in the difference between a single full-window write and several sequential ones on this controller still only keeps the most recent one.

## Pin map

| Function | Pin | Function | Pin |
| --- | --- | --- | --- |
| QSPI CS | 45 | QSPI SCK | 47 |
| QSPI D0 | 21 | QSPI D1 | 48 |
| QSPI D2 | 40 | QSPI D3 | 39 |
| Backlight | 1 | Touch SDA | 4 |
| Touch SCL | 8 | Touch INT | 3 |
| Touch addr | `0x3B` | Battery ADC | 5 |

Confirmed against Guition's own vendor demo package for this board (not just a third-party reference) — same values either way. These live in a clearly-marked, editable block at the top of the board's section in `hal.cpp` — if a board revision changes a pin, it's a one-line fix in one place.

## Flash

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e mini-s3-35 -t upload     # firmware
```

This board isn't in the web flasher yet — building from source is the only path for now.

## Controls — touch zones

There are no physical user buttons, so the two-button UX maps to halves of the touch panel (matching the on-screen "tap left / tap right" legend), the same scheme as the CrowPanel Advance 3.5":

| Touch | Acts as | PIN entry | Dashboard |
| ----- | ------- | --------- | --------- |
| Tap **LEFT** half | Button A | Cycle the current digit | Cycle brightness |
| Tap **RIGHT** half | Button B | Confirm digit | Force refresh |

## Notes

- **No on-boot factory reset** — the touch driver reports a single point per interrupt, so the A+B combo needs two simultaneous inputs that aren't available. Re-flash to wipe NVS.
- **No battery readout yet** — GPIO 5 is the ADC pin (confirmed from Guition's own demo firmware), but the voltage-divider ratio needed to turn a raw reading into a percentage hasn't been validated against real battery hardware. `halBatPercent()` is stubbed to `-1` (matching the CrowPanel Advance 3.5"); wiring it up once that's confirmed is a small, self-contained change.
- **New board, bring-up first.** Before flashing the full firmware, run the dedicated bring-up env — it does an I²C scan (expect `0x3B`), drives color/fill tests over QSPI, and prints raw touch coordinates, so a wrong pin fails loudly instead of showing a blank screen:
  ```bash
  pio run -e mini-s3-35-bringup -t upload && pio device monitor
  ```
- During setup, the WiFi AP password is shown on the device screen.

---

**Mini ESP32-S3 3.5"** · env `mini-s3-35` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
