# How To Build (Draft)

Status: work in progress.

This document tracks the practical hardware build process for a simpler-to-assemble NextCab controller.
Primary goal: reduce soldering and make assembly repeatable for other builders.

## 1. Build Goals

- Minimize solder joints.
- Prefer JST SH / STEMMA QT / Qwiic cable interconnects where possible.
- Keep wiring modular so parts can be swapped during testing.
- Keep firmware setup documented per hardware variant.

## 2. Current Component Set

### Confirmed parts

- Adafruit ESP32-S3 Feather
- Adafruit rotary encoder (STEMMA QT, seesaw)
- Qwiic keypad (exact model TBD)
- Display: TBD (currently testing color TFT)

### Notes

- The Adafruit STEMMA QT encoder defaults to I2C address 0x36.
- Feather ESP32-S3 onboard MAX17048 battery gauge also uses 0x36.
- If MAX17048 support is enabled in firmware, set the encoder to 0x37 by bridging A0 on the encoder board.

## 3. Wiring Strategy (Low-Solder Direction)

## 3.1 I2C bus devices

- Put encoder and keypad on the same I2C bus using STEMMA QT / Qwiic cabling.
- Use daisy-chain or a passive I2C splitter hub.
- Keep cable runs short and mechanically strain-relieved.

## 3.2 Display options under evaluation

- Monochrome OLED (simple, low power, easy to read).
- Color TFT (richer UI, but more wiring and often higher power draw).

Decision pending: validate whether TFT provides enough operational benefit over monochrome for this handheld use case.

## 4. Firmware Configuration Notes

Current Feather build environment in this repo:

- Environment: feather_s3_ili9341
- Uses Qwiic keypad input path.
- Uses STEMMA seesaw encoder input path.
- Uses MAX17048 battery monitor.
- Encoder address configured to 0x37 to avoid MAX17048 conflict.

Before first power-on test:

1. Confirm encoder A0 bridge is applied (for address 0x37).
2. Flash feather_s3_ili9341 firmware.
3. Open serial monitor at 115200 and confirm encoder init messages.

## 5. Known Risks / Gotchas

- I2C address conflict (0x36) if encoder A0 is not bridged.
- USB port re-enumeration during upload can cause temporary COM port failures.
- Unknown keypad model may require driver or keymap adjustments.

### Upload recovery when COM upload fails

If upload fails with "Could not open COMx" after the 1200 bps reset step, use
the manual-upload environment and enter bootloader mode yourself.

1. Hold BOOT on the Feather.
2. Tap RESET.
3. Release BOOT.
4. Upload with `feather_s3_ili9341_manual`.

This environment disables auto-reset hooks (`--before no_reset --after no_reset`)
to avoid the fragile USB reconnect timing path.

## 6. Open Questions

- Exact Qwiic keypad model and dimensions?
- Final display choice: monochrome OLED vs color TFT?
- Battery strategy for the low-solder version?
- Preferred enclosure and connector access layout?

## 7. Next Updates To Add

- Exact part numbers and purchase links.
- Photos of wiring topology.
- Pinout table for each supported display option.
- Step-by-step assembly checklist.
- Bring-up checklist (power, serial logs, input tests).
