# Fallback snapshot — mini-s3-35 (working, green mascot)

This is a known-good, flashable snapshot of the `mini-s3-35` firmware as it stood
**before** any of the color-fix / dashboard-rework changes described in the plan
`spicy-singing-engelbart.md`. Use it to recover the board to this exact state if
something later in that work goes wrong.

## What state this is

- Git commit: `32720a0` (`Add mini-s3-35 board: Guition JC3248W535C_I (ESP32-S3, AXS15231B QSPI)`)
- Plus the uncommitted working-tree changes to `platformio.ini` and `src/ui.cpp`
  that were already in progress at that commit (the `-DMANGO_UI` / `SX()`/`SY()`/`TS()`
  scaling port). A copy of that diff is saved alongside this file as
  `uncommitted-changes.diff` — apply it with `git apply working-green-version-fallback/uncommitted-changes.diff`
  from the repo root if you ever need to reconstruct the exact source tree this
  binary was built from.
- This is the version where the Clawd mascot and orange UI elements render **green**
  (the `halFlush()` byte-order bug the plan's Step 1 fixes) — hence the folder name.

## Files in this folder

| File | Flash offset |
|---|---|
| `bootloader.bin` | `0x0` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |

These offsets come from this project's own web-flasher manifest generator
(`web/src/build.py`, `BOOTLOADER_OFFSET["ESP32-S3"] = 0x0`) and match every other
ESP32-S3 board in this repo (e.g. `web/manifests/crowpanel-adv-35.json`).

## Restore command (copy-paste, no rebuild)

From this folder (`working-green-version-fallback/`), with the board plugged in
over USB-C and its COM port identified (Device Manager on Windows, or
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
  0x0     working-green-version-fallback\bootloader.bin ^
  0x8000  working-green-version-fallback\partitions.bin ^
  0xE000  working-green-version-fallback\boot_app0.bin ^
  0x10000 working-green-version-fallback\firmware.bin
```

## NVS is untouched by this restore

None of the four files above touch the `nvs` partition (`0x9000`–`0xDFFF` per
`default_16MB.csv`). Reflashing with this command does **not** erase the stored
Claude Code OAuth token, PIN, or Wi-Fi credentials — the device will boot straight
back into normal operation with existing settings intact, not into first-time setup.

(A full-chip erase, e.g. `esptool.py erase_flash`, *would* wipe NVS along with
everything else — this restore command does not do that and there is no reason to
run one.)
