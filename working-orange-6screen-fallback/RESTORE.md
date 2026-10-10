# Fallback snapshot — mini-s3-35 (working, orange, six-screen dashboard)

This is a known-good, flashable snapshot of the `mini-s3-35` firmware as it stood
once the color fix and the full six-screen dashboard (NOW / MODELS / 5-HOUR CHART /
7-DAY CHART / 24-HOUR RHYTHM / 30-DAY CHART) were confirmed working on real
hardware — user-confirmed "looks good" on 2026-09-20. Use it to recover the board
to this exact state if later work goes wrong.

This **supersedes** `working-green-version-fallback/` (the pre-color-fix, single-
screen snapshot) as the nearer restore point, but that folder is left in place too
— it's still a valid fallback further back.

## What state this is

- Base git commit: `32720a0` (`Add mini-s3-35 board: Guition JC3248W535C_I (ESP32-S3, AXS15231B QSPI)`)
- Plus all uncommitted working-tree changes on top of that commit at snapshot
  time — none of this has been committed to git yet. A full copy of every
  touched/new source file is saved in `src-snapshot/` alongside this file:
  `hal.cpp/h`, `ui.cpp/h`, `main.cpp`, `api.cpp/h`, `platformio.ini`, and the
  three new modules `history_mini.h/.cpp`, `screens_mini.h/.cpp`. To rebuild
  from source instead of flashing the binaries below, copy these files back
  into `src/` (and `platformio.ini` to the repo root) over whatever's there
  at restore time.
- What this snapshot includes, relative to the original single-screen orange fix:
  - Step 1 byte-order color fix (`halFlush()` in `hal.cpp`) — Clawd and all
    UI accents render true orange, not green.
  - New 30-day ring buffer (`history_mini.h/.cpp`) at 5-minute resolution,
    persisted to `/hist_mini.bin` on the `spiffs` LittleFS partition.
  - New six-screen dashboard (`screens_mini.h/.cpp` + new `ui.cpp` draw
    functions): NOW (5-HOUR/7-DAY cards + MODELS breakdown), MODELS (health
    detail), 5-HOUR CHART, 7-DAY CHART, 24-HOUR RHYTHM, 30-DAY CHART.
  - Tap-zone paging (left/right half of screen), long-press left = brightness,
    long-press right = force refresh, dot indicator along the bottom.
  - New header: device label + IP only (no SSID, no battery) + WiFi icon +
    "Ns ago" timer.
  - `Serial.begin(115200)` added for this board (its console is the native
    USB-Serial-JTAG peripheral, which unlike a UART-bridge board needs an
    explicit `begin()` to transmit anything at all).

## Files in this folder

| File | Flash offset |
|---|---|
| `bootloader.bin` | `0x0` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |

Same offsets as `working-green-version-fallback/RESTORE.md` — they come from
this project's own web-flasher manifest generator (`web/src/build.py`,
`BOOTLOADER_OFFSET["ESP32-S3"] = 0x0`) and are identical for every ESP32-S3
board in this repo.

## Restore command (copy-paste, no rebuild)

From this folder (`working-orange-6screen-fallback/`), with the board plugged
in over USB-C and its COM port identified (Device Manager on Windows, or
`pio device list`):

```
esptool.py --chip esp32s3 --port COM6 --baud 460800 write_flash ^
  0x0     bootloader.bin ^
  0x8000  partitions.bin ^
  0xE000  boot_app0.bin ^
  0x10000 firmware.bin
```

(On macOS/Linux, replace `^` line continuations with `\` and `COM6` with the
board's `/dev/tty*` / `/dev/cu.*` path.)

If `esptool.py` isn't on PATH, PlatformIO ships its own copy — run instead from
the repo root:

```
"%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py" --chip esp32s3 --port COM6 --baud 460800 write_flash ^
  0x0     working-orange-6screen-fallback\bootloader.bin ^
  0x8000  working-orange-6screen-fallback\partitions.bin ^
  0xE000  working-orange-6screen-fallback\boot_app0.bin ^
  0x10000 working-orange-6screen-fallback\firmware.bin
```

## NVS is untouched by this restore

None of the four files above touch the `nvs` partition (`0x9000`–`0xDFFF` per
`default_16MB.csv`). Reflashing with this command does **not** erase the stored
Claude Code OAuth token, PIN, or Wi-Fi credentials — the device boots straight
back into normal operation with existing settings intact.

The `spiffs` partition (`0xc90000`–`0xFEFFFF`, holding `/hist_mini.bin`, the
30-day usage ring buffer) is also untouched by this restore — history keeps
accumulating across a restore rather than resetting to empty.

(A full-chip erase, e.g. `esptool.py erase_flash`, *would* wipe both NVS and
the history partition along with everything else — this restore command does
not do that and there is no reason to run one.)
