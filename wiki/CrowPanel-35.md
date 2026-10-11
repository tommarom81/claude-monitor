# Elecrow CrowPanel ESP32 Display 3.5" HMI

Part of this project. Elecrow's basic 3.5" HMI panel — the same 480×320 screen as the [CrowPanel Advance 3.5"](CrowPanel-Advance-35), but on a classic ESP32 with resistive touch instead of capacitive. It runs the same 480×320 layout and the same left/right touch zones as the Advance.

## Advance or basic? Check before you flash

The two boards look alike and share a name, but they need different firmware. Pick the wrong card in the web flasher and it stops with **"Your ESP board is not supported"**.

| | CrowPanel 3.5" (this page) | [CrowPanel Advance 3.5"](CrowPanel-Advance-35) |
| --- | --- | --- |
| MCU | ESP32 (WROVER-B module) | ESP32-S3 |
| Touch | Resistive — press firmly, a stylus works | Capacitive — a light tap |
| Factory demo | "ESP32 Display-3.5" LVGL demo with a touch-calibration screen | — |
| Web flasher card | **CrowPanel 3.5" (ESP32)** | **CrowPanel Advance 3.5"** |

Not sure? `esptool --port <port> chip-id` prints `ESP32-D0WD…` for this board and `ESP32-S3` for the Advance.

## Specs

| | |
| --- | --- |
| MCU | ESP32-WROVER-B (ESP32-D0WD, dual-core, WiFi + BT, 4 MB PSRAM usable) |
| Flash | 4 or 8 MB (the firmware uses a 4 MB layout, so either works) |
| Display | 3.5" ILI9488 TFT, 480×320, SPI (HSPI) |
| Touch | XPT2046 resistive, on the display's SPI bus |
| Battery | External (percentage not shown) |
| Buttons | None — touch zones, see Controls |
| USB | CH340 UART bridge (not native USB) |
| PlatformIO env | `crowpanel-35` |

### Pins

| Signal | GPIO |
| --- | --- |
| TFT MOSI / SCLK / CS / DC | 13 / 14 / 15 / 2 |
| TFT reset | — (not on a GPIO) |
| Backlight | 27 |
| SPI MISO | 33 |
| Touch CS | 12 |

Verified on hardware. Elecrow's wiki lists MISO 12 and touch CS 33 — the other way round — but with that wiring the tested board read no touches at all, and owners of the v2.2 revision report the same swap. If touch is dead on your board, swap `TFT_MISO` and `TOUCH_CS` in `platformio.ini` and rebuild, then open an issue with your board revision.

## Flash

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e crowpanel-35 -t upload     # firmware
```

## Controls — touch zones

There are no physical user buttons, so the two-button UX maps to halves of the touch panel. It's a resistive panel, so press rather than tap lightly.

| Touch | Acts as | PIN entry | Dashboard |
| ----- | ------- | --------- | --------- |
| Press **LEFT** half | Button A | Cycle the current digit | Cycle brightness |
| Press **RIGHT** half | Button B | Confirm digit | Force refresh |

## Notes

- **No on-boot factory reset** — the A+B combo needs two simultaneous inputs and the panel reports a single touch point. Re-flash to wipe NVS.
- **No battery readout** — battery percentage isn't shown on the dashboard.
- **No touch calibration** — only a left/right split is needed, so raw touch readings are used directly. Building with `-DCROWPANEL_TOUCH_DEBUG` logs the PSRAM size and raw touch points on serial (115200).
- The dashboard renders into a PSRAM sprite and is pushed in one transfer, so it doesn't flicker, just like the Advance.
- During setup, the WiFi AP password is shown on the device screen.

---

**CrowPanel 3.5"** · env `crowpanel-35` · [All boards](Supported-Boards)
